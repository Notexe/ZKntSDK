#pragma once

template<class T> class TRefCountPtrArg {
  public:
    T* m_pObject;
};

template<class T> class TRefCountPtr : public TRefCountPtrArg<T> {};
