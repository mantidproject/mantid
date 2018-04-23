#ifndef SINGLETON_HOLDER_H
#define SINGLETON_HOLDER_H

////////////////////////////////////////////////////////////////////////////////
// The Loki Library
// Copyright (c) 2001 by Andrei Alexandrescu
// This code accompanies the book:
// Alexandrescu, Andrei. "Modern C++ Design: Generic Programming and Design
//     Patterns Applied". Copyright (c) 2001. Addison-Wesley.
// Permission to use, copy, modify, distribute and sell this software for any
//     purpose is hereby granted without fee, provided that the above copyright
//     notice appear in all copies and that both that copyright notice and this
//     permission notice appear in supporting documentation.
// The author or Addison-Wesley Longman make no representations about the
//     suitability of this software for any purpose. It is provided "as is"
//     without express or implied warranty.
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Simplified from the original code, to just work for simple singletons
// Removed all the code relating to the creation/destruction, threading and
// lifetime policies.
////////////////////////////////////////////////////////////////////////////////

#include <MantidKernel/DllConfig.h>
#include <cassert>
#include <cstdlib>
#include <mutex>
#include <stdexcept>
#include <string>
#include <typeinfo>

namespace Mantid {
namespace Kernel {

/// prototype for function passed to atexit()
using atexit_func_t = void (*)();

extern MANTID_KERNEL_DLL void CleanupSingletons();
extern MANTID_KERNEL_DLL void AddSingleton(atexit_func_t func);

/// class to manage an instance of an object as a singleton
template <typename T> class SingletonHolder {
public:
  /// Allow users to access to the type returned by Instance()
  using HeldType = T;

  static T &Instance();

private:
  static void DestroySingleton();
  /// default constructor marked private so only access is via the Instance()
  /// method
  SingletonHolder();

  static T *pInstance;
  static std::once_flag flag;
  static bool destroyed;
};

/// Implementation of the SingletonHolder create policy using the new and delete
/// operators
template <typename T> struct CreateUsingNew {
  /// create an object using the new operator
  /// @returns New instance
  static T *Create() { return new T; }
  /// delete an object instantiated using Create
  /// @param p :: pointer to instance to destroy
  static void Destroy(T *p) { delete p; }
};

/// Return a reference to the Singleton instance, creating it if it does not
/// already exist
/// Creation is done using the CreateUsingNew policy at the moment
template <typename T> inline T &SingletonHolder<T>::Instance() {
  if (destroyed) {
    std::string s("Attempt to use destroyed singleton ");
    s += typeid(T).name();
    throw std::runtime_error(s.c_str());
  }
  std::call_once(flag, [] {
    pInstance = CreateUsingNew<T>::Create();
    AddSingleton(&DestroySingleton);
  });
  return *pInstance;
}

/// Destroy the singleton
template <typename T> void SingletonHolder<T>::DestroySingleton() {
  // std::cerr << "destroying singleton " << typeid(T).name() << '\n';
  assert(!destroyed);
  CreateUsingNew<T>::Destroy(pInstance);
  pInstance = nullptr;
  destroyed = true;
}

/// global variable holding pointer to singleton instance
template <typename T> T *SingletonHolder<T>::pInstance = nullptr;

template <typename T> std::once_flag SingletonHolder<T>::flag;

/// variable to allow trapping of attempts to destroy a singleton more than once
template <typename T> bool SingletonHolder<T>::destroyed = false;
}
}

#endif // SINGLETON_HOLDER_H
