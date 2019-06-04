// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef Mantid_std::make_unique_h
#define Mantid_std::make_unique_h

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

// implementation of memory::std::make_unique for platforms that don't currently have
// it.
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3656.htm

namespace Mantid {

namespace Kernel {

#if __cplusplus >= 201402L ||                                                  \
    (defined(_MSC_VER) && (_MSC_VER > 1700)) // C++14 or MSVC 2013+

using std::make_unique;

#else  // C++11

template <class T> struct _Unique_if {
  using _Single_object = std::unique_ptr<T>;
};

template <class T> struct _Unique_if<T[]> {
  using _Unknown_bound = std::unique_ptr<T[]>;
};

template <class T, size_t N> struct _Unique_if<T[N]> {
  struct __invalid_type {};
};

template <class T>
typename _Unique_if<T>::_Unknown_bound std::make_unique(size_t n) {
  using U = typename std::remove_extent<T>::type;
  return std::unique_ptr<T>(new U[n]());
}

template <class T, class... Args>
inline typename _Unique_if<T>::_Single_object std::make_unique(Args &&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template <class T, class... Args>
inline typename _Unique_if<T>::_Known_bound std::make_unique(Args &&...) = delete;
#endif // __cplusplus == 201402L
} // namespace Kernel
} // namespace Mantid
#endif // Mantid_std::make_unique_h
