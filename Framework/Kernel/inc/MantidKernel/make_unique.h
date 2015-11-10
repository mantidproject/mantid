#ifndef Mantid_make_unique_h
#define Mantid_make_unique_h

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

// implementation of memory::make_unique for platforms that don't currently have
// it.
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3656.htm

namespace Mantid {

namespace Kernel {

#if __cplusplus > 201103L // C++14

using std::make_unique;

#else // C++11

template <class T> struct _Unique_if {
  typedef std::unique_ptr<T> _Single_object;
};

template <class T> struct _Unique_if<T[]> {
  typedef std::unique_ptr<T[]> _Unknown_bound;
};

template <class T, size_t N> struct _Unique_if<T[N]> {
  struct __invalid_type {};
};

template <class T>
typename _Unique_if<T>::_Unknown_bound make_unique(size_t n) {
  typedef typename std::remove_extent<T>::type U;
  return std::unique_ptr<T>(new U[n]());
}

#if !defined(_MSC_VER) || _MSC_VER > 1700

template <class T, class... Args>
inline typename _Unique_if<T>::_Single_object make_unique(Args &&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template <class T, class... Args>
inline typename _Unique_if<T>::_Known_bound make_unique(Args &&...) = delete;
#else
// workaround for MSVC 2012

template <class T> inline typename _Unique_if<T>::_Single_object make_unique() {
  return std::unique_ptr<T>(new T());
}

template <class T, class Arg1>
inline typename _Unique_if<T>::_Single_object make_unique(Arg1 param1) {
  return std::unique_ptr<T>(new T(std::forward<Arg1>(param1)));
}

template <class T, class Arg1, class Arg2>
inline typename _Unique_if<T>::_Single_object make_unique(Arg1 param1,
                                                          Arg2 param2) {
  return std::unique_ptr<T>(
      new T(std::forward<Arg1>(param1), std::forward<Arg2>(param2)));
}

template <class T, class Arg1, class Arg2, class Arg3>
inline typename _Unique_if<T>::_Single_object
make_unique(Arg1 param1, Arg2 param2, Arg3 param3) {
  return std::unique_ptr<T>(new T(std::forward<Arg1>(param1),
                                  std::forward<Arg2>(param2),
                                  std::forward<Arg3>(param3)));
}

template <class T, class Arg1, class Arg2, class Arg3, class Arg4>
inline typename _Unique_if<T>::_Single_object
make_unique(Arg1 param1, Arg2 param2, Arg3 param3, Arg4 param4) {
  return std::unique_ptr<T>(
      new T(std::forward<Arg1>(param1), std::forward<Arg2>(param2),
            std::forward<Arg3>(param3), std::forward<Arg4>(param4)));
}

template <class T, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5>
inline typename _Unique_if<T>::_Single_object
make_unique(Arg1 param1, Arg2 param2, Arg3 param3, Arg4 param4, Arg5 param5) {
  return std::unique_ptr<T>(
      new T(std::forward<Arg1>(param1), std::forward<Arg2>(param2),
            std::forward<Arg3>(param3), std::forward<Arg4>(param4),
            std::forward<Arg5>(param5)));
}

template <class T>
inline typename _Unique_if<T>::_Known_bound make_unique() = delete;
template <class T, class Arg1>
inline typename _Unique_if<T>::_Known_bound make_unique(Arg1 param1) = delete;
template <class T, class Arg1, class Arg2>
inline typename _Unique_if<T>::_Known_bound make_unique(Arg1 param1,
                                                        Arg2 param2) = delete;
template <class T, class Arg1, class Arg2, class Arg3>
inline typename _Unique_if<T>::_Known_bound
make_unique(Arg1 param1, Arg2 param2, Arg3 param3) = delete;
template <class T, class Arg1, class Arg2, class Arg3, class Arg4>
inline typename _Unique_if<T>::_Known_bound
make_unique(Arg1 param1, Arg2 param2, Arg3 param3, Arg4 param4) = delete;
template <class T, class Arg1, class Arg2, class Arg3, class Arg4, class Arg5>
inline typename _Unique_if<T>::_Known_bound
make_unique(Arg1 param1, Arg2 param2, Arg3 param3, Arg4 param4,
            Arg5 param5) = delete;
#endif // MSVC
#endif // __cplusplus == 201402L
}
}
#endif
