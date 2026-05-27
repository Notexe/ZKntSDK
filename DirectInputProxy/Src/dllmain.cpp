#include <windows.h>

#include "loader.hpp"
#include "Rendering/D3D12Hooks.hpp"
#include "Rendering/RenderingHost.hpp"
#include "Rendering/WndProcSubclass.hpp"

#include <string>

namespace {
    // Real dinput8.dll handle.
    HMODULE g_RealDinput = nullptr;

    void LoadRealDinput() {
        wchar_t s_SysDir[65535];
        const auto s_SysDirLen = GetSystemDirectoryW(s_SysDir, 65535);
        std::wstring s_SysDirPath(s_SysDir, s_SysDirLen);
        s_SysDirPath += L"\\dinput8.dll";
        g_RealDinput = LoadLibraryW(s_SysDirPath.c_str());
        OutputDebugStringA(g_RealDinput ? "[dinput8 proxy] loaded genuine dinput8.dll\n" : "[dinput8 proxy] FAILED to load genuine dinput8.dll\n");
    }
}

extern "C" HRESULT WINAPI DirectInput8Create(HINSTANCE hinst, DWORD version, const IID& iid, void** out, void* outer) {
    using fn_t = HRESULT(WINAPI*)(HINSTANCE, DWORD, const IID&, void**, void*);
    static auto fn = reinterpret_cast<fn_t>(g_RealDinput ? GetProcAddress(g_RealDinput, "DirectInput8Create") : nullptr);
    return fn ? fn(hinst, version, iid, out, outer) : E_FAIL;
}

BOOL WINAPI DllMain(HINSTANCE self, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(self);
        LoadRealDinput();
        knt::rendering::RenderingHost::Instance().Startup();
        knt::rendering::D3D12Hooks::Instance().Startup();
        zknt::loader::Start(self);
    }
    else if (reason == DLL_PROCESS_DETACH) {
        zknt::loader::Stop();
        knt::rendering::WndProcSubclass::Instance().Uninstall();
        knt::rendering::D3D12Hooks::Instance().Shutdown();
        knt::rendering::RenderingHost::Instance().Shutdown();
    }

    return TRUE;
}
