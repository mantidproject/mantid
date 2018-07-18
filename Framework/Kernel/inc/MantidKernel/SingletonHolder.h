#ifndef MANTID_KERNEL_SINGLETON_HOLDER_H
#define MANTID_KERNEL_SINGLETON_HOLDER_H

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
#include <functional>

namespace Mantid {
namespace Kernel {

/// Type of deleter function
using deleter_t = std::function<void()>;

/// Register the given deleter function to be called
/// at exit
extern MANTID_KERNEL_DLL void deleteOnExit(deleter_t);

/// Manage the lifetime of a class intended to be a singleton
template <typename T> class SingletonHolder {
public:
  using HeldType = T;

  SingletonHolder() = delete;

  static T &Instance();
};

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
/// Creation is done using the CreateUsingNew policy at the moment
template <typename T> inline T &SingletonHolder<T>::Instance() {
// Local static instance initialized by an immediately invoked lambda.
// A second nested lambda captures the singleton instance pointer
// and forms the deleter function that is registered with
// the atexit deleters.

#ifndef NDEBUG
  static bool destroyed(false);
#endif
  static T *instance = [] {
    auto *singleton = CreateUsingNew<T>::create();
    deleteOnExit(deleter_t([singleton]() {
#ifndef NDEBUG
      destroyed = true;
#endif
      CreateUsingNew<T>::destroy(singleton);
    }));
    return singleton;
  }();
  assert(!destroyed);
  return *instance;
}

} // namespace Kernel
} // namespace Mantid

#endif // SINGLETON_HOLDER_H
