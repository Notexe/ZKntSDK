#pragma once

#include "ZObjectPool.hpp"

template<class T> class TObjectPool {
  public:
    ZObjectPool m_Pool;
    T* m_pStart;
    T* m_pEnd;

    [[nodiscard]] size_t IndexOf(T* p_Object) {
        return (reinterpret_cast<uintptr_t>(p_Object) - reinterpret_cast<uintptr_t>(m_pStart)) / sizeof(T);
    }

    [[nodiscard]] bool BelongsToPool(T* p_Object) const {
        return p_Object >= m_pStart && p_Object < m_pEnd;
    }
};
