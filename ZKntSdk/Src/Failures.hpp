#pragma once

// Number of failures encountered while setting up functions, hooks, and globals.
inline int g_Failures = 0;

inline void Fail() {
    ++g_Failures;
}
