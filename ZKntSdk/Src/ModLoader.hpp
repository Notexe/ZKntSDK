#pragma once

#include "IPluginInterface.hpp"

#include <cr.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace zknt {
    class ModLoader {
      public:
        ModLoader();
        ~ModLoader();

        ModLoader(const ModLoader&) = delete;
        ModLoader& operator=(const ModLoader&) = delete;

        void Startup();

        // Enumerate mods/*.dll into m_AvailableMods. No DLLs are loaded
        // here -- a DLL is only opened when a user enables it.
        void ScanAvailableMods();

        std::unordered_set<std::string> GetAvailableMods();
        std::unordered_set<std::string> GetActiveMods();

        // Diff against the current active set, unload removed mods, load
        // newly-enabled ones, and persist the selection to mods.ini.
        void SetActiveMods(const std::unordered_set<std::string>& p_Mods);

        void LoadMod(const std::string& p_Name, bool p_LiveLoad);
        void UnloadMod(const std::string& p_Name);
        void ReloadMod(const std::string& p_Name);
        void UnloadAllMods();
        void ReloadAllMods();

        // Polled from a background thread; invokes cr_plugin_update on each
        // active mod, picking up rebuilt DLLs on disk.
        void TickHotReload();

        IPluginInterface* GetModByName(const std::string& p_Name);

        std::vector<IPluginInterface*> GetLoadedMods() {
            std::shared_lock s_Lock(m_Mutex);
            std::vector<IPluginInterface*> s_Result;
            s_Result.reserve(m_ModList.size());
            for (auto* s_Mod : m_ModList) {
                if (s_Mod && s_Mod->m_Plugin) {
                    s_Result.push_back(s_Mod->m_Plugin);
                }
            }
            return s_Result;
        }

        struct LoadedMod {
            cr_plugin m_Ctx{};
            knt::mod::HostServices m_Services{};
            std::filesystem::path m_Path;
            // Populated by RegisterPlugin trampoline during cr_main.
            IPluginInterface* m_Plugin = nullptr;
            void* m_Token = nullptr;
            // False until LoadMod has done the initial Init(). Used by
            // Cb_RegisterPlugin to auto-Init the new instance produced by
            // a hot-reload (cr_plugin_update -> CR_UNLOAD + CR_LOAD).
            bool m_FirstLoadDone = false;
        };

      private:
        // cr-userdata trampolines. Each LoadedMod's m_Services points at
        // free functions that route back to the live ModLoader instance.
        static void Cb_RegisterPlugin(void* p_Token, IPluginInterface* p_Plugin);
        static void Cb_UnregisterPlugin(void* p_Token);

        void LoadActiveModsFromIni();
        void PersistActiveModsToIni(const std::unordered_set<std::string>& p_Mods);

        void HotReloadThreadProc();

        std::shared_mutex m_Mutex;
        std::filesystem::path m_CrTempDir;
        std::unordered_set<std::string> m_AvailableMods;      // canonical-case names
        std::unordered_set<std::string> m_AvailableModsLower; // lookup helper
        std::unordered_set<std::string> m_IncompatibleMods;
        std::unordered_map<std::string, std::unique_ptr<LoadedMod>> m_LoadedMods; // lowercase name -> mod
        // Pointers are stable because LoadedMod lives inside the unique_ptr
        // map above. Storing LoadedMod* (rather than IPluginInterface*) here
        // means hot-reloads that recycle the plugin instance don't need to
        // touch this vector.
        std::vector<LoadedMod*> m_ModList;

        std::thread m_HotReloadThread;
        std::atomic<bool> m_HotReloadStop{false};
    };
}
