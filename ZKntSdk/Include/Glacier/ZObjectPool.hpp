#pragma once

#include "ZInfiniteBuffer.hpp"
#include "ZMutex.hpp"

class ZObjectPool {
  public:
    volatile int64_t m_nFreeListStart;
    uint32_t* m_pData;
    uint32_t m_nObjectSize;
    uint32_t m_nMaxObjectCount;
    uint32_t m_nGrowCount;
    uint32_t m_nObjectDelta;
    uint32_t m_nSize;
    ZInfiniteBuffer<void> m_Buffer;
    ZMutex m_Mutex;

    [[nodiscard]] bool BelongsToPool(void* p_Object) const {
        return reinterpret_cast<uintptr_t>(p_Object) - reinterpret_cast<uintptr_t>(m_Buffer.m_pData) < m_Buffer.m_nActualSize;
    }

    void Free(void* p_Object) {
        if (!p_Object) {
            return;
        }

        const uint32_t v4 = (reinterpret_cast<uintptr_t>(p_Object) - reinterpret_cast<uintptr_t>(m_pData)) / sizeof(uint32_t);

        int64_t v5;
        int64_t v6;

        do {
            v5 = InterlockedExchangeAdd64(&m_nFreeListStart, 0);
            v6 = v4 | (v5 >> 32 << 32) + 0x100000000LL;
        } while (InterlockedCompareExchange64(&m_nFreeListStart, v6, v5) != v5);
    }
};

static_assert(sizeof(ZObjectPool) == 104);
static_assert(offsetof(ZObjectPool, m_pData) == 8);
static_assert(offsetof(ZObjectPool, m_Buffer.m_nActualSize) == 52);
