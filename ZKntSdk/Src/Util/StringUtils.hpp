#pragma once

#include <algorithm>
#include <string>
#include <vector>

namespace knt::util {
    inline std::string ToLowerCase(const std::string& p_String) {
        std::string s_Result = p_String;
        std::transform(s_Result.begin(), s_Result.end(), s_Result.begin(), [](unsigned char c) { return static_cast<char>(::tolower(c)); });
        return s_Result;
    }

    inline std::string ToUpperCase(const std::string& p_String) {
        std::string s_Result = p_String;
        std::transform(s_Result.begin(), s_Result.end(), s_Result.begin(), [](unsigned char c) { return static_cast<char>(::toupper(c)); });
        return s_Result;
    }

    inline std::vector<std::string> Split(const std::string& p_String, const std::string& p_Delim) {
        std::vector<std::string> s_Parts;
        std::size_t s_Start = p_String.find_first_not_of(p_Delim);
        std::size_t s_End;
        while ((s_End = p_String.find_first_of(p_Delim, s_Start)) != std::string::npos) {
            s_Parts.push_back(p_String.substr(s_Start, s_End - s_Start));
            s_Start = p_String.find_first_not_of(p_Delim, s_End);
        }
        if (s_Start != std::string::npos) {
            s_Parts.push_back(p_String.substr(s_Start));
        }
        return s_Parts;
    }
}
