#pragma once

#include "IModSDK.hpp"

#include "spdlog/spdlog.h"

namespace Logger {
    namespace detail {
        inline void Dispatch(spdlog::level::level_enum p_Level, std::string_view p_Msg) {
            if (auto* s_Sdk = SDK(); s_Sdk) {
                s_Sdk->Log(p_Level, p_Msg);
            }
        }
    }

    template<typename... Args> void Error(spdlog::format_string_t<Args...> p_Format, const Args&... p_Args) {
        detail::Dispatch(spdlog::level::err, fmt::format(fmt::runtime(p_Format), p_Args...));
    }

    template<typename... Args> void Warn(spdlog::format_string_t<Args...> p_Format, const Args&... p_Args) {
        detail::Dispatch(spdlog::level::warn, fmt::format(fmt::runtime(p_Format), p_Args...));
    }

    template<typename... Args> void Info(spdlog::format_string_t<Args...> p_Format, const Args&... p_Args) {
        detail::Dispatch(spdlog::level::info, fmt::format(fmt::runtime(p_Format), p_Args...));
    }

    template<typename... Args> void Debug(spdlog::format_string_t<Args...> p_Format, const Args&... p_Args) {
        detail::Dispatch(spdlog::level::debug, fmt::format(fmt::runtime(p_Format), p_Args...));
    }

    template<typename... Args> void Trace(spdlog::format_string_t<Args...> p_Format, const Args&... p_Args) {
        detail::Dispatch(spdlog::level::trace, fmt::format(fmt::runtime(p_Format), p_Args...));
    }
}
