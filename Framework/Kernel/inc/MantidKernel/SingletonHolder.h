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
#include <mutex>

namespace Mantid {
namespace Kernel {

/// Type of deleter function
using SingletonDeleterFn = std::function<void()>;

/// Register the given deleter function to be called
/// at exit
MANTID_KERNEL_DLL void deleteOnExit(SingletonDeleterFn func);

/// Manage the lifetime of a class intended to be a singleton
template <typename T> class SingletonHolder {
public:
  using HeldType = T;

  SingletonHolder() = delete;

  static T &Instance();

private:
  static T *instance;
  static std::once_flag once;
#ifndef NDEBUG
  static bool destroyed;
#endif
};

// Static field initializers
template <typename T> T *SingletonHolder<T>::instance = nullptr;
template <typename T> std::once_flag SingletonHolder<T>::once;
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
/// Creation is done using the CreateUsingNew policy at the moment
template <typename T> inline T &SingletonHolder<T>::Instance() {
  std::call_once(once, []() {
    instance = CreateUsingNew<T>::create();
    deleteOnExit(SingletonDeleterFn([]() {
#ifndef NDEBUG
      destroyed = true;
#endif
      CreateUsingNew<T>::destroy(instance);
    }));
  });
#ifndef NDEBUG
  assert(!destroyed);
#endif
  return *instance;
}

} // namespace Kernel
} // namespace Mantid

#endif // SINGLETON_HOLDER_H
