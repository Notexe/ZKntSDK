#pragma once

#include "ZPrimitives.hpp"

class ZMutex {
  public:
    void Lock() {
        EnterCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(this));
    }

    void Unlock() {
        LeaveCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(this));
    }

    uint64_t m_impl[5];
};
