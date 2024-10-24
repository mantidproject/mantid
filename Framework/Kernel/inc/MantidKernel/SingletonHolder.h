#pragma once

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

#include "MantidKernel/DllConfig.h"
#include <cassert>
#include <cstdlib>
#include <functional>
#include <mutex>

namespace Mantid {
namespace Kernel {

/// Type of deleter function
using SingletonDeleterFn = std::function<void()>;

/// Register the given deleter function to be called
/// at exit
MANTID_KERNEL_DLL void deleteOnExit(const SingletonDeleterFn &func);

/// Manage the lifetime of a class intended to be a singleton
template <typename T> class SingletonHolder {
public:
  using HeldType = T;

  SingletonHolder() = delete;

  static T &Instance();

private:
#ifndef NDEBUG
  static bool destroyed;
#endif
};

// Static field initializers
#ifndef NDEBUG
template <typename T> bool SingletonHolder<T>::destroyed = false;
#endif

/// Policy class controlling creation of the singleton
/// Implementation classes should mark their default
/// constructors private and insert a friend declaration
/// for this class, e.g.:
///
///   friend struct Mantid::Kernel::CreateUsingNew<SingletonImplClass>;
///
template <typename T> struct CreateUsingNew {
  /// create an object using the new operator
  /// @returns New instance
  static T *create() { return new T; }
  /// delete an object instantiated using Create
  /// @param p :: pointer to instance to destroy
  static void destroy(T *p) { delete p; }
};

/// Return a reference to the Singleton instance, creating it if it does not
/// already exist
/// Creation is done using the CreateUsingNew policy. Held types need
/// to make CreateUsingNew<T> a friend.
/// This method cannot be inlined due to the presence of a local static
/// variable. Inlining causes each call site to receive a different
/// copy of the static instance variable.
template <typename T>
#if defined(_MSC_VER)
__declspec(noinline)
#endif
T &
#if defined(__GNUC__) // covers clang too
    __attribute__((noinline))
#endif
    SingletonHolder<T>::Instance() {
  // Initialiazing a local static is thread-safe in C++11
  // The inline lambda call is used to create the singleton once
  // and register an atexit function to delete it
  static T *instance = []() {
    auto local = CreateUsingNew<T>::create();
    deleteOnExit(SingletonDeleterFn([]() {
#ifndef NDEBUG
      destroyed = true;
#endif
      CreateUsingNew<T>::destroy(instance);
    }));
    return local;
  }();
#ifndef NDEBUG
  assert(!destroyed);
#endif
  return *instance;
}

} // namespace Kernel
} // namespace Mantid
