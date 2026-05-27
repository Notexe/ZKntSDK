#include "D3D12DXGIFactory.hpp"

#include "D3D12SwapChain.hpp"
#include "RenderingHost.hpp"

#include "../../Include/D3DUtils.hpp"

namespace knt::rendering {
    D3D12DXGIFactory::D3D12DXGIFactory(IDXGIFactory4* p_Target) : m_Target(p_Target) {
        m_Target->AddRef();
    }

    D3D12DXGIFactory::~D3D12DXGIFactory() {
        m_Target->Release();
    }

    ULONG STDMETHODCALLTYPE D3D12DXGIFactory::AddRef() {
        return ++m_RefCount;
    }

    ULONG STDMETHODCALLTYPE D3D12DXGIFactory::Release() {
        const ULONG s_NewRefCount = --m_RefCount;
        if (s_NewRefCount == 0) {
            delete this;
        }
        return s_NewRefCount;
    }

    HRESULT STDMETHODCALLTYPE D3D12DXGIFactory::QueryInterface(REFIID p_Riid, void** p_OutObject) {
        if (!p_OutObject) {
            return E_INVALIDARG;
        }
        *p_OutObject = nullptr;
        if (p_Riid == IID_IUnknown || p_Riid == __uuidof(IDXGIObject) || p_Riid == __uuidof(IDXGIFactory) || p_Riid == __uuidof(IDXGIFactory1)
            || p_Riid == __uuidof(IDXGIFactory2) || p_Riid == __uuidof(IDXGIFactory3) || p_Riid == __uuidof(IDXGIFactory4)
            || p_Riid == __uuidof(IDXGIFactory5) || p_Riid == __uuidof(IDXGIFactory6) || p_Riid == __uuidof(IDXGIFactory7)) {
            *p_OutObject = this;
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE D3D12DXGIFactory::CheckFeatureSupport(DXGI_FEATURE p_Feature, void* p_Data, UINT p_DataSize) {
        IDXGIFactory5* s_Factory5 = nullptr;
        if (m_Target->QueryInterface(IID_PPV_ARGS(&s_Factory5)) != S_OK) {
            return E_NOTIMPL;
        }
        const HRESULT s_Result = s_Factory5->CheckFeatureSupport(p_Feature, p_Data, p_DataSize);
        s_Factory5->Release();
        return s_Result;
    }

    HRESULT STDMETHODCALLTYPE
    D3D12DXGIFactory::EnumAdapterByGpuPreference(UINT p_Adapter, DXGI_GPU_PREFERENCE p_GpuPreference, REFIID p_Riid, void** p_OutAdapter) {
        IDXGIFactory6* s_Factory6 = nullptr;
        if (m_Target->QueryInterface(IID_PPV_ARGS(&s_Factory6)) != S_OK) {
            return E_NOTIMPL;
        }
        const HRESULT s_Result = s_Factory6->EnumAdapterByGpuPreference(p_Adapter, p_GpuPreference, p_Riid, p_OutAdapter);
        s_Factory6->Release();
        return s_Result;
    }

    HRESULT STDMETHODCALLTYPE D3D12DXGIFactory::RegisterAdaptersChangedEvent(HANDLE p_Event, DWORD* p_OutCookie) {
        IDXGIFactory7* s_Factory7 = nullptr;
        if (m_Target->QueryInterface(IID_PPV_ARGS(&s_Factory7)) != S_OK) {
            return E_NOTIMPL;
        }
        const HRESULT s_Result = s_Factory7->RegisterAdaptersChangedEvent(p_Event, p_OutCookie);
        s_Factory7->Release();
        return s_Result;
    }

    HRESULT STDMETHODCALLTYPE D3D12DXGIFactory::UnregisterAdaptersChangedEvent(DWORD p_Cookie) {
        IDXGIFactory7* s_Factory7 = nullptr;
        if (m_Target->QueryInterface(IID_PPV_ARGS(&s_Factory7)) != S_OK) {
            return E_NOTIMPL;
        }
        const HRESULT s_Result = s_Factory7->UnregisterAdaptersChangedEvent(p_Cookie);
        s_Factory7->Release();
        return s_Result;
    }

    void D3D12DXGIFactory::WrapAndPublishSwapChain(IUnknown* p_Device, IDXGISwapChain* p_RawSwapChain, IDXGISwapChain** p_OutSwapChain) {
        ScopedD3DRef<IDXGISwapChain3> s_SwapChain3;
        if (p_RawSwapChain->QueryInterface(REF_IID_PPV_ARGS(s_SwapChain3)) != S_OK) {
            // Not a SwapChain3 (D3D11 or older). Hand the raw object back.
            *p_OutSwapChain = p_RawSwapChain;
            return;
        }

        // The wrapper owns the QI'd IDXGISwapChain3 ref; drop the raw one.
        p_RawSwapChain->Release();

        auto* s_Wrapped = new D3D12SwapChain(s_SwapChain3.m_Ref);
        s_Wrapped->AddRef();
        *p_OutSwapChain = s_Wrapped;

        RenderingHost::Instance().NotifyNewSwapChain(s_SwapChain3.m_Ref, p_Device);
    }

    HRESULT STDMETHODCALLTYPE D3D12DXGIFactory::CreateSwapChain(IUnknown* p_Device, DXGI_SWAP_CHAIN_DESC* p_Desc, IDXGISwapChain** p_OutSwapChain) {
        IDXGISwapChain* s_Raw = nullptr;
        const HRESULT s_Result = m_Target->CreateSwapChain(p_Device, p_Desc, &s_Raw);
        if (FAILED(s_Result) || !s_Raw) {
            *p_OutSwapChain = s_Raw;
            return s_Result;
        }
        WrapAndPublishSwapChain(p_Device, s_Raw, p_OutSwapChain);
        return s_Result;
    }

    HRESULT STDMETHODCALLTYPE D3D12DXGIFactory::CreateSwapChainForHwnd(
        IUnknown* p_Device, HWND p_Hwnd, const DXGI_SWAP_CHAIN_DESC1* p_Desc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* p_FullscreenDesc,
        IDXGIOutput* p_RestrictToOutput, IDXGISwapChain1** p_OutSwapChain
    ) {
        IDXGISwapChain1* s_Raw = nullptr;
        const HRESULT s_Result = m_Target->CreateSwapChainForHwnd(p_Device, p_Hwnd, p_Desc, p_FullscreenDesc, p_RestrictToOutput, &s_Raw);
        if (FAILED(s_Result) || !s_Raw) {
            *p_OutSwapChain = s_Raw;
            return s_Result;
        }

        IDXGISwapChain* s_RawBase = nullptr;
        WrapAndPublishSwapChain(p_Device, s_Raw, &s_RawBase);
        // The wrapper also implements IDXGISwapChain1; on the fallback path
        // WrapAndPublishSwapChain returns the raw object unchanged.
        *p_OutSwapChain = static_cast<IDXGISwapChain1*>(s_RawBase);
        return s_Result;
    }

    HRESULT STDMETHODCALLTYPE D3D12DXGIFactory::CreateSwapChainForCoreWindow(
        IUnknown* p_Device, IUnknown* p_Window, const DXGI_SWAP_CHAIN_DESC1* p_Desc, IDXGIOutput* p_RestrictToOutput, IDXGISwapChain1** p_OutSwapChain
    ) {
        IDXGISwapChain1* s_Raw = nullptr;
        const HRESULT s_Result = m_Target->CreateSwapChainForCoreWindow(p_Device, p_Window, p_Desc, p_RestrictToOutput, &s_Raw);
        if (FAILED(s_Result) || !s_Raw) {
            *p_OutSwapChain = s_Raw;
            return s_Result;
        }
        IDXGISwapChain* s_RawBase = nullptr;
        WrapAndPublishSwapChain(p_Device, s_Raw, &s_RawBase);
        *p_OutSwapChain = static_cast<IDXGISwapChain1*>(s_RawBase);
        return s_Result;
    }

    HRESULT STDMETHODCALLTYPE D3D12DXGIFactory::CreateSwapChainForComposition(
        IUnknown* p_Device, const DXGI_SWAP_CHAIN_DESC1* p_Desc, IDXGIOutput* p_RestrictToOutput, IDXGISwapChain1** p_OutSwapChain
    ) {
        IDXGISwapChain1* s_Raw = nullptr;
        const HRESULT s_Result = m_Target->CreateSwapChainForComposition(p_Device, p_Desc, p_RestrictToOutput, &s_Raw);
        if (FAILED(s_Result) || !s_Raw) {
            *p_OutSwapChain = s_Raw;
            return s_Result;
        }
        IDXGISwapChain* s_RawBase = nullptr;
        WrapAndPublishSwapChain(p_Device, s_Raw, &s_RawBase);
        *p_OutSwapChain = static_cast<IDXGISwapChain1*>(s_RawBase);
        return s_Result;
    }
}
