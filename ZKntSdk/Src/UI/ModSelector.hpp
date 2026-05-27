#pragma once

#include <string>
#include <unordered_set>
#include <vector>
#include <Windows.h>

namespace zknt::ui {
    class ModSelector {
      public:
        ModSelector();

        // Called by ModLoader after every Scan / SetActiveMods so the UI list
        // reflects what's on disk and what's currently active.
        void UpdateAvailableMods(
            const std::unordered_set<std::string>& p_Mods, const std::unordered_set<std::string>& p_IncompatibleMods,
            const std::unordered_set<std::string>& p_ActiveMods
        );

        // Invoked from the SDK's UI callback once per frame.
        void Draw(bool p_HasFocus);

        void Show() {
            m_Open = true;
        }

      private:
        struct AvailableMod {
            std::string m_Name;
            bool m_Enabled;
        };

        void ApplySelectedMods();

        bool m_Open = false;
        bool m_ShouldShow = false;
        SRWLOCK m_Lock{};
        std::vector<AvailableMod> m_AvailableMods;
        std::unordered_set<std::string> m_IncompatibleMods;
    };
}
