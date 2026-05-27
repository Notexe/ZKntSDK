#include "SkipIntro.hpp"

#include <Logging.hpp>
#include <Glacier/ZString.h>

void SkipIntro::Init() {
    // SDK()->Hooks()->ZEngineAppCommon_GetBootScene->AddDetour(this, &SkipIntro::ZEngineAppCommon_GetBootScene);
}

DEFINE_PLUGIN_DETOUR(SkipIntro, ZString*, ZEngineAppCommon_GetBootScene, ZEngineAppCommon* th, ZString& result) {
    // TODO: Figure out another way to do this as this doesn't work.
    // Game expects an entity here, not a brick, and this just ends up crashing it.
    result = std::string_view("assembly:/_knt/scenes/frontend/mainmenu.brick");

    // const auto s_Result = p_Hook->CallOriginal(th, result);
    // Logger::Info("Original boot scene: {}", s_Result->c_str());

    return {HookAction::Return(), &result};

    /*result = (*Globals::ComponentManager)->m_pApplication->GetOption("SCENE_FILE");

    if (result == "assembly:/_PRO/Scenes/Frontend/Boot.entity") {
        result = "assembly:/_PRO/Scenes/Frontend/MainMenu.entity";
    }

    return HookResult<ZString*>(HookAction::Return(), &result);*/
}

DEFINE_ZKNT_PLUGIN(SkipIntro)
