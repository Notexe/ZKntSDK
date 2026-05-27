#pragma once

#include <Windows.h>

namespace knt::rendering {
    // Installs MinHook detours on the dxgi.dll CreateDXGIFactory* entry
    // points. Lives in the proxy DLL (never unloads).
    class D3D12Hooks {
      public:
        static D3D12Hooks& Instance();

        bool Startup();
        void Shutdown();

      private:
        D3D12Hooks() = default;

        bool m_Installed = false;
    };
}
