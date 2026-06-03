#define CR_HOST CR_SAFEST
#include "ModLoader.hpp"

#include "HookImpl.hpp"
#include "Logging.hpp"
#include "ModSDK.hpp"
#include "Util/StringUtils.hpp"

#include <Windows.h>
#include <atomic>
#include <format>
#include <fstream>

namespace fs = std::filesystem;

namespace zknt {
    namespace {
        // Set immediately before invoking cr (cr_plugin_open / cr_plugin_update / cr_plugin_close)
        // so the Cb_* trampolines can route back to the mod currently being driven.
        thread_local ModLoader::LoadedMod* tl_PendingMod = nullptr;
        thread_local ModLoader* tl_PendingLoader = nullptr;

        struct ScopedPendingMod {
            ScopedPendingMod(ModLoader* p_Loader, ModLoader::LoadedMod* p_Mod) {
                tl_PendingLoader = p_Loader;
                tl_PendingMod = p_Mod;
            }
            ~ScopedPendingMod() {
                tl_PendingMod = nullptr;
                tl_PendingLoader = nullptr;
            }
        };

        fs::path GameExeDir() {
            char s_Path[MAX_PATH];
            const auto s_Size = GetModuleFileNameA(nullptr, s_Path, MAX_PATH);
            if (s_Size == 0) {
                return {};
            }
            return fs::path(s_Path).parent_path();
        }

        fs::path ModsDir() {
            return GameExeDir() / "mods";
        }

        fs::path ModsIniPath() {
            return GameExeDir() / "mods.ini";
        }

        // Minimal INI reader: one line per `[section]`, blank lines / # / ; comments ignored.
        // We only care about top-level section names (= active mods).
        std::vector<std::string> ReadEnabledModsFromIni(const fs::path& p_Path) {
            std::vector<std::string> s_Mods;
            std::ifstream s_File(p_Path);
            if (!s_File) {
                return s_Mods;
            }
            std::string s_Line;
            while (std::getline(s_File, s_Line)) {
                // Trim leading whitespace.
                std::size_t s_Begin = s_Line.find_first_not_of(" \t\r");
                if (s_Begin == std::string::npos) {
                    continue;
                }
                if (s_Line[s_Begin] != '[') {
                    continue;
                }
                const std::size_t s_End = s_Line.find(']', s_Begin + 1);
                if (s_End == std::string::npos) {
                    continue;
                }
                std::string s_Name = s_Line.substr(s_Begin + 1, s_End - s_Begin - 1);
                if (!s_Name.empty()) {
                    s_Mods.push_back(s_Name);
                }
            }
            return s_Mods;
        }

        void WriteEnabledModsToIni(const fs::path& p_Path, const std::unordered_set<std::string>& p_Mods) {
            std::ofstream s_File(p_Path, std::ios::trunc);
            if (!s_File) {
                return;
            }
            for (const auto& s_Mod : p_Mods) {
                s_File << '[' << s_Mod << "]\n";
            }
        }
    }

    ModLoader::ModLoader() = default;

    ModLoader::~ModLoader() {
        // Stop the watcher first so it doesn't try to cr_plugin_update mods
        // that we're about to unload.
        m_HotReloadStop.store(true, std::memory_order_release);
        if (m_HotReloadThread.joinable()) {
            m_HotReloadThread.join();
        }

        UnloadAllMods();

        // Best-effort cleanup of cr's working-copy directory.
        if (!m_CrTempDir.empty()) {
            std::error_code s_Ec;
            fs::remove_all(m_CrTempDir, s_Ec);
        }
    }

    void ModLoader::Startup() {
        wchar_t s_TempBuf[MAX_PATH];
        const auto s_TempLen = GetTempPathW(_countof(s_TempBuf), s_TempBuf);
        if (s_TempLen > 0 && s_TempLen < _countof(s_TempBuf)) {
            m_CrTempDir = fs::path(s_TempBuf) / std::format(L"ZKntSdk-Mods-{}", GetCurrentProcessId());
            std::error_code s_Ec;
            fs::create_directories(m_CrTempDir, s_Ec);
            if (s_Ec) {
                Logger::Warn("Failed to create cr temp dir '{}': {}", m_CrTempDir.string(), s_Ec.message());
                m_CrTempDir.clear();
            }
        }

        ScanAvailableMods();
        LoadActiveModsFromIni();

        // Watch mod DLL files for changes and hot-reload them via cr.
        m_HotReloadStop.store(false, std::memory_order_release);
        m_HotReloadThread = std::thread(&ModLoader::HotReloadThreadProc, this);
    }

    void ModLoader::ScanAvailableMods() {
        std::unique_lock s_Lock(m_Mutex);

        m_AvailableMods.clear();
        m_AvailableModsLower.clear();
        m_IncompatibleMods.clear();

        const auto s_ModsDir = ModsDir();
        if (!fs::exists(s_ModsDir) || !fs::is_directory(s_ModsDir)) {
            Logger::Warn("Mods directory '{}' not found.", s_ModsDir.string());
            return;
        }

        for (const auto& s_Entry : fs::directory_iterator(s_ModsDir)) {
            if (!s_Entry.is_regular_file()) {
                continue;
            }
            if (s_Entry.path().extension() != ".dll") {
                continue;
            }
            auto s_Name = s_Entry.path().filename().stem().string();
            m_AvailableMods.insert(s_Name);
            m_AvailableModsLower.insert(knt::util::ToLowerCase(s_Name));
        }
    }

    std::unordered_set<std::string> ModLoader::GetAvailableMods() {
        std::shared_lock s_Lock(m_Mutex);
        return m_AvailableMods;
    }

    std::unordered_set<std::string> ModLoader::GetActiveMods() {
        std::shared_lock s_Lock(m_Mutex);
        std::unordered_set<std::string> s_Set;
        for (const auto& [s_Key, _] : m_LoadedMods) {
            s_Set.insert(s_Key);
        }
        return s_Set;
    }

    void ModLoader::SetActiveMods(const std::unordered_set<std::string>& p_Mods) {
        std::unordered_set<std::string> s_DesiredLower;
        for (const auto& s_Mod : p_Mods) {
            s_DesiredLower.insert(knt::util::ToLowerCase(s_Mod));
        }

        std::vector<std::string> s_ToUnload;
        std::vector<std::string> s_ToLoad;
        {
            std::shared_lock s_Lock(m_Mutex);
            for (const auto& [s_Key, _] : m_LoadedMods) {
                if (!s_DesiredLower.contains(s_Key)) {
                    s_ToUnload.push_back(s_Key);
                }
            }
            for (const auto& s_Lower : s_DesiredLower) {
                if (!m_LoadedMods.contains(s_Lower)) {
                    s_ToLoad.push_back(s_Lower);
                }
            }
        }

        for (const auto& s_Mod : s_ToUnload) {
            UnloadMod(s_Mod);
        }
        for (const auto& s_Mod : s_ToLoad) {
            LoadMod(s_Mod, true);
        }

        PersistActiveModsToIni(p_Mods);
    }

    void ModLoader::LoadActiveModsFromIni() {
        const auto s_Enabled = ReadEnabledModsFromIni(ModsIniPath());
        for (const auto& s_Mod : s_Enabled) {
            const auto s_Lower = knt::util::ToLowerCase(s_Mod);
            if (s_Lower == "sdk") {
                continue;
            }
            {
                std::shared_lock s_Lock(m_Mutex);
                if (!m_AvailableModsLower.contains(s_Lower)) {
                    Logger::Warn("Mod '{}' listed in mods.ini but not found in mods/.", s_Mod);
                    continue;
                }
            }
            LoadMod(s_Mod, false);
        }
    }

    void ModLoader::PersistActiveModsToIni(const std::unordered_set<std::string>& p_Mods) {
        WriteEnabledModsToIni(ModsIniPath(), p_Mods);
    }

    void ModLoader::LoadMod(const std::string& p_Name, bool /*p_LiveLoad*/) {
        const std::string s_Lower = knt::util::ToLowerCase(p_Name);

        // Locate the on-disk DLL using the canonical-cased name we stashed in
        // ScanAvailableMods (preserves filesystem casing on case-sensitive FS).
        std::string s_Canonical = p_Name;
        {
            std::shared_lock s_Lock(m_Mutex);
            if (m_LoadedMods.contains(s_Lower)) {
                Logger::Warn("Mod '{}' is already loaded.", p_Name);
                return;
            }
            for (const auto& s_Avail : m_AvailableMods) {
                if (knt::util::ToLowerCase(s_Avail) == s_Lower) {
                    s_Canonical = s_Avail;
                    break;
                }
            }
        }

        const auto s_DllPath = ModsDir() / (s_Canonical + ".dll");
        if (!fs::exists(s_DllPath)) {
            Logger::Warn("Mod DLL not found: {}", s_DllPath.string());
            return;
        }

        Logger::Info("Loading mod '{}' from {}.", p_Name, s_DllPath.string());

        // Construct the LoadedMod up-front so its address is stable; cr keeps
        // a pointer to the embedded cr_plugin and we keep a pointer to the
        // LoadedMod from the trampolines.
        auto s_Owned = std::make_unique<LoadedMod>();
        s_Owned->m_Path = s_DllPath;
        s_Owned->m_Services.m_SDK = SDK();
        s_Owned->m_Services.RegisterPlugin = &ModLoader::Cb_RegisterPlugin;
        s_Owned->m_Services.UnregisterPlugin = &ModLoader::Cb_UnregisterPlugin;
        s_Owned->m_Ctx.userdata = &s_Owned->m_Services;

        {
            ScopedPendingMod s_Pending(this, s_Owned.get());
            if (!cr_plugin_open(s_Owned->m_Ctx, s_DllPath.string().c_str())) {
                Logger::Warn("cr_plugin_open failed for mod '{}'.", p_Name);
                return;
            }
            // Route cr's working copies into a process-private temp dir so
            // they don't sit next to the original DLL (where Windows /
            // other mods could pick them up as duplicate modules).
            if (!m_CrTempDir.empty()) {
                cr_set_temporary_path(s_Owned->m_Ctx, m_CrTempDir.string());
            }
            // Force the initial load.
            const auto s_Rc = cr_plugin_update(s_Owned->m_Ctx, true);
            if (s_Rc < 0 || s_Owned->m_Ctx.failure != CR_NONE) {
                Logger::Warn("cr_plugin_update failed for mod '{}': rc={} failure={}", p_Name, s_Rc, static_cast<int>(s_Owned->m_Ctx.failure));
                cr_plugin_close(s_Owned->m_Ctx);
                return;
            }
        }

        IPluginInterface* s_Plugin = s_Owned->m_Plugin;
        if (!s_Plugin) {
            Logger::Warn("Mod '{}' didn't register a plugin during cr_main.", p_Name);
            cr_plugin_close(s_Owned->m_Ctx);
            return;
        }

        LoadedMod* s_ModPtr = nullptr;
        {
            std::unique_lock s_Lock(m_Mutex);
            s_ModPtr = s_Owned.get();
            m_ModList.push_back(s_ModPtr);
            m_LoadedMods.emplace(s_Lower, std::move(s_Owned));
        }

        // Plugins call Init on the game thread; we don't have a game thread
        // available here, so for now Init and OnEngineInitialized fire from
        // the SDK Engine_Init detour (see ModSDK::Engine_Init).
        s_Plugin->Init();
        s_ModPtr->m_FirstLoadDone = true;

        ModSDK::GetInstance()->OnModLoaded(p_Name, s_Plugin, true);
    }

    void ModLoader::UnloadMod(const std::string& p_Name) {
        const std::string s_Lower = knt::util::ToLowerCase(p_Name);

        std::unique_ptr<LoadedMod> s_Mod;
        {
            std::unique_lock s_Lock(m_Mutex);
            auto s_It = m_LoadedMods.find(s_Lower);
            if (s_It == m_LoadedMods.end()) {
                return;
            }
            s_Mod = std::move(s_It->second);
            m_LoadedMods.erase(s_It);
            std::erase(m_ModList, s_Mod.get());
        }

        Logger::Info("Unloading mod '{}'.", p_Name);

        ModSDK::GetInstance()->OnModUnloading(p_Name, s_Mod->m_Plugin);

        // Detour cleanup happens via Cb_UnregisterPlugin inside cr_plugin_close.

        {
            ScopedPendingMod s_Pending(this, s_Mod.get());
            cr_plugin_close(s_Mod->m_Ctx);
        }
        // cr's CR_UNLOAD/CLOSE handler in the mod deletes the plugin instance.

        ModSDK::GetInstance()->OnModUnloaded(p_Name);
    }

    void ModLoader::ReloadMod(const std::string& p_Name) {
        UnloadMod(p_Name);
        LoadMod(p_Name, true);
    }

    void ModLoader::UnloadAllMods() {
        std::vector<std::string> s_Names;
        {
            std::shared_lock s_Lock(m_Mutex);
            s_Names.reserve(m_LoadedMods.size());
            for (const auto& [s_Key, _] : m_LoadedMods) {
                s_Names.push_back(s_Key);
            }
        }
        for (const auto& s_Name : s_Names) {
            UnloadMod(s_Name);
        }
    }

    void ModLoader::ReloadAllMods() {
        std::vector<std::string> s_Names;
        {
            std::shared_lock s_Lock(m_Mutex);
            s_Names.reserve(m_LoadedMods.size());
            for (const auto& [s_Key, _] : m_LoadedMods) {
                s_Names.push_back(s_Key);
            }
        }
        for (const auto& s_Name : s_Names) {
            ReloadMod(s_Name);
        }
    }

    void ModLoader::TickHotReload() {
        std::shared_lock s_Lock(m_Mutex);
        for (auto& [s_Key, s_Mod] : m_LoadedMods) {
            ScopedPendingMod s_Pending(this, s_Mod.get());
            // cr_plugin_update detects file-mtime changes and, on change,
            // drives cr_main(CR_UNLOAD) on the live DLL followed by
            // cr_main(CR_LOAD) on the rebuilt one. Detour cleanup and the
            // new instance's Init() are dispatched through Cb_UnregisterPlugin
            // / Cb_RegisterPlugin so this loop stays trivial.
            cr_plugin_update(s_Mod->m_Ctx, false);
        }
    }

    void ModLoader::HotReloadThreadProc() {
        using namespace std::chrono_literals;
        // Poll every 500ms. cr_plugin_update is cheap when nothing has
        // changed (just a file stat per mod) so this is a tolerable cost.
        while (!m_HotReloadStop.load(std::memory_order_acquire)) {
            try {
                TickHotReload();
            }
            catch (...) {
                // Swallow exceptions so the watcher thread never tears the
                // process down. A corrupt rebuild would otherwise propagate.
            }
            std::this_thread::sleep_for(500ms);
        }
    }

    IPluginInterface* ModLoader::GetModByName(const std::string& p_Name) {
        std::shared_lock s_Lock(m_Mutex);
        const auto s_It = m_LoadedMods.find(knt::util::ToLowerCase(p_Name));
        return s_It == m_LoadedMods.end() ? nullptr : s_It->second->m_Plugin;
    }

    void ModLoader::Cb_RegisterPlugin(void* p_Token, IPluginInterface* p_Plugin) {
        auto* s_Mod = tl_PendingMod;
        if (!s_Mod) {
            return;
        }
        s_Mod->m_Plugin = p_Plugin;
        s_Mod->m_Token = p_Token;
        // First load is driven by ModLoader::LoadMod which calls Init()
        // explicitly after cr_plugin_open returns. Subsequent CR_LOAD
        // events come from the hot-reload watcher; those need to invoke
        // Init on the recycled instance themselves.
        if (s_Mod->m_FirstLoadDone && p_Plugin) {
            p_Plugin->Init();

            if (ModSDK::GetInstance()->IsEngineInitialized()) {
                p_Plugin->OnEngineInitialized();
            }
        }
    }

    void ModLoader::Cb_UnregisterPlugin(void* p_Token) {
        auto* s_Mod = tl_PendingMod;
        if (!s_Mod || s_Mod->m_Token != p_Token) {
            return;
        }
        // Pull the old instance's detours before cr unmaps the DLL so the
        // captured `Context` and detour function pointers never dangle.
        if (s_Mod->m_Plugin) {
            HookRegistry::ClearDetoursWithContext(s_Mod->m_Plugin);
        }
        s_Mod->m_Plugin = nullptr;
        s_Mod->m_Token = nullptr;
    }
}
