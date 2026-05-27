#pragma once

#include <atomic>
#include <cstdint>
#include <dxgi1_6.h>

namespace knt::rendering {
    // Passthrough wrapper for IDXGIFactory4..7. Lives in the proxy DLL so its
    // vtable survives SDK hot-reloads. Intercepts every CreateSwapChain*
    // variant so we can wrap whatever the game asks for.
    class D3D12DXGIFactory final : public IDXGIFactory7 {
      public:
        explicit D3D12DXGIFactory(IDXGIFactory4* p_Target);
        ~D3D12DXGIFactory();

        D3D12DXGIFactory(const D3D12DXGIFactory&) = delete;
        D3D12DXGIFactory& operator=(const D3D12DXGIFactory&) = delete;

        // IUnknown
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID p_Riid, void** p_OutObject) override;
        ULONG STDMETHODCALLTYPE AddRef() override;
        ULONG STDMETHODCALLTYPE Release() override;

        // IDXGIObject
        HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID p_Name, UINT p_DataSize, const void* p_Data) override {
            return m_Target->SetPrivateData(p_Name, p_DataSize, p_Data);
        }
        HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID p_Name, const IUnknown* p_Unknown) override {
            return m_Target->SetPrivateDataInterface(p_Name, p_Unknown);
        }
        HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID p_Name, UINT* p_DataSize, void* p_Data) override {
            return m_Target->GetPrivateData(p_Name, p_DataSize, p_Data);
        }
        HRESULT STDMETHODCALLTYPE GetParent(REFIID p_Riid, void** p_OutParent) override {
            return m_Target->GetParent(p_Riid, p_OutParent);
        }

        // IDXGIFactory
        HRESULT STDMETHODCALLTYPE EnumAdapters(UINT p_Adapter, IDXGIAdapter** p_OutAdapter) override {
            return m_Target->EnumAdapters(p_Adapter, p_OutAdapter);
        }
        HRESULT STDMETHODCALLTYPE MakeWindowAssociation(HWND p_Window, UINT p_Flags) override {
            return m_Target->MakeWindowAssociation(p_Window, p_Flags);
        }
        HRESULT STDMETHODCALLTYPE GetWindowAssociation(HWND* p_OutWindow) override {
            return m_Target->GetWindowAssociation(p_OutWindow);
        }
        HRESULT STDMETHODCALLTYPE CreateSwapChain(IUnknown* p_Device, DXGI_SWAP_CHAIN_DESC* p_Desc, IDXGISwapChain** p_OutSwapChain) override;
        HRESULT STDMETHODCALLTYPE CreateSoftwareAdapter(HMODULE p_Module, IDXGIAdapter** p_OutAdapter) override {
            return m_Target->CreateSoftwareAdapter(p_Module, p_OutAdapter);
        }

        // IDXGIFactory1
        HRESULT STDMETHODCALLTYPE EnumAdapters1(UINT p_Adapter, IDXGIAdapter1** p_OutAdapter) override {
            return m_Target->EnumAdapters1(p_Adapter, p_OutAdapter);
        }
        BOOL STDMETHODCALLTYPE IsCurrent() override {
            return m_Target->IsCurrent();
        }

        // IDXGIFactory2
        BOOL STDMETHODCALLTYPE IsWindowedStereoEnabled() override {
            return m_Target->IsWindowedStereoEnabled();
        }
        HRESULT STDMETHODCALLTYPE CreateSwapChainForHwnd(
            IUnknown* p_Device, HWND p_Hwnd, const DXGI_SWAP_CHAIN_DESC1* p_Desc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* p_FullscreenDesc,
            IDXGIOutput* p_RestrictToOutput, IDXGISwapChain1** p_OutSwapChain
        ) override;
        HRESULT STDMETHODCALLTYPE CreateSwapChainForCoreWindow(
            IUnknown* p_Device, IUnknown* p_Window, const DXGI_SWAP_CHAIN_DESC1* p_Desc, IDXGIOutput* p_RestrictToOutput,
            IDXGISwapChain1** p_OutSwapChain
        ) override;
        HRESULT STDMETHODCALLTYPE GetSharedResourceAdapterLuid(HANDLE p_Resource, LUID* p_OutLuid) override {
            return m_Target->GetSharedResourceAdapterLuid(p_Resource, p_OutLuid);
        }
        HRESULT STDMETHODCALLTYPE RegisterStereoStatusWindow(HWND p_Window, UINT p_Msg, DWORD* p_OutCookie) override {
            return m_Target->RegisterStereoStatusWindow(p_Window, p_Msg, p_OutCookie);
        }
        HRESULT STDMETHODCALLTYPE RegisterStereoStatusEvent(HANDLE p_Event, DWORD* p_OutCookie) override {
            return m_Target->RegisterStereoStatusEvent(p_Event, p_OutCookie);
        }
        void STDMETHODCALLTYPE UnregisterStereoStatus(DWORD p_Cookie) override {
            m_Target->UnregisterStereoStatus(p_Cookie);
        }
        HRESULT STDMETHODCALLTYPE RegisterOcclusionStatusWindow(HWND p_Window, UINT p_Msg, DWORD* p_OutCookie) override {
            return m_Target->RegisterOcclusionStatusWindow(p_Window, p_Msg, p_OutCookie);
        }
        HRESULT STDMETHODCALLTYPE RegisterOcclusionStatusEvent(HANDLE p_Event, DWORD* p_OutCookie) override {
            return m_Target->RegisterOcclusionStatusEvent(p_Event, p_OutCookie);
        }
        void STDMETHODCALLTYPE UnregisterOcclusionStatus(DWORD p_Cookie) override {
            m_Target->UnregisterOcclusionStatus(p_Cookie);
        }
        HRESULT STDMETHODCALLTYPE CreateSwapChainForComposition(
            IUnknown* p_Device, const DXGI_SWAP_CHAIN_DESC1* p_Desc, IDXGIOutput* p_RestrictToOutput, IDXGISwapChain1** p_OutSwapChain
        ) override;

        // IDXGIFactory3
        UINT STDMETHODCALLTYPE GetCreationFlags() override {
            return m_Target->GetCreationFlags();
        }

        // IDXGIFactory4
        HRESULT STDMETHODCALLTYPE EnumAdapterByLuid(LUID p_AdapterLuid, REFIID p_Riid, void** p_OutAdapter) override {
            return m_Target->EnumAdapterByLuid(p_AdapterLuid, p_Riid, p_OutAdapter);
        }
        HRESULT STDMETHODCALLTYPE EnumWarpAdapter(REFIID p_Riid, void** p_OutAdapter) override {
            return m_Target->EnumWarpAdapter(p_Riid, p_OutAdapter);
        }

        // IDXGIFactory5
        HRESULT STDMETHODCALLTYPE CheckFeatureSupport(DXGI_FEATURE p_Feature, void* p_FeatureSupportData, UINT p_FeatureSupportDataSize) override;

        // IDXGIFactory6
        HRESULT STDMETHODCALLTYPE
        EnumAdapterByGpuPreference(UINT p_Adapter, DXGI_GPU_PREFERENCE p_GpuPreference, REFIID p_Riid, void** p_OutAdapter) override;

        // IDXGIFactory7
        HRESULT STDMETHODCALLTYPE RegisterAdaptersChangedEvent(HANDLE p_Event, DWORD* p_OutCookie) override;
        HRESULT STDMETHODCALLTYPE UnregisterAdaptersChangedEvent(DWORD p_Cookie) override;

      private:
        // Called by every CreateSwapChain* variant. Wraps the raw swap chain
        // and notifies registered callbacks via the RenderingHost.
        void WrapAndPublishSwapChain(IUnknown* p_Device, IDXGISwapChain* p_RawSwapChain, IDXGISwapChain** p_OutSwapChain);

        IDXGIFactory4* m_Target;
        std::atomic<ULONG> m_RefCount{0};
    };
}
