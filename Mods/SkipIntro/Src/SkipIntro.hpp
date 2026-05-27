#pragma once

#include <IPluginInterface.hpp>

class ZString;
class ZEngineAppCommon;

class SkipIntro : public zknt::IPluginInterface {
  public:
    void Init() override;

  private:
    DECLARE_PLUGIN_DETOUR(SkipIntro, ZString*, ZEngineAppCommon_GetBootScene, ZEngineAppCommon* th, ZString& result);
};

DECLARE_ZKNT_PLUGIN(SkipIntro)
