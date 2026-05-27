#pragma once

#include <IPluginInterface.hpp>

class ZApplicationEngineWin32;
class NoPause : public zknt::IPluginInterface {
  public:
    void Init() override;

  private:
    DECLARE_PLUGIN_DETOUR(NoPause, bool, GetOption, const ZString&, bool);
    DECLARE_PLUGIN_DETOUR(NoPause, LRESULT, WndProc, ZApplicationEngineWin32*, HWND, UINT, WPARAM, LPARAM);
};

DECLARE_ZKNT_PLUGIN(NoPause)
