#pragma once

#include "Common.hpp"
#include "ZScene.hpp"

class IModule : public IComponentInterface {};

class ZSimpleModuleBase : public IModule {};

class ZGameSceneflowModule : public ZSimpleModuleBase {
  public:
    [[nodiscard]]
    bool IsEngineInitialized() const {
        if (!m_pEntitySceneContext) {
            return false;
        }

        return m_pEntitySceneContext->m_SceneResource.size() != 0;
    }

    PAD(0x58);
    ZEntitySceneContext* m_pEntitySceneContext; // 0x60
};
