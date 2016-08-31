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
// lifetime policies. Simplified to take advantage of thread-safe static
// initalization in C++11
////////////////////////////////////////////////////////////////////////////////

#include <MantidKernel/System.h>

#include <memory>

namespace Mantid {
namespace Kernel {

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

/// class to manage an instance of an object as a singleton
template <typename T> class SingletonHolder {
public:
  /// Allow users to access to the type returned by Instance()
  typedef T HeldType;
  static T &Instance();

private:
};

/// Return a reference to the Singleton instance, creating it if it does not
/// already exist
/// Creation is done using the CreateUsingNew policy at the moment
template <typename T> inline T &SingletonHolder<T>::Instance() {
  static std::unique_ptr<T, void (*)(T *)> instance{CreateUsingNew<T>::Create(),
                                                    CreateUsingNew<T>::Destroy};
  return *instance;
}

}
}

#endif // SINGLETON_HOLDER_H
