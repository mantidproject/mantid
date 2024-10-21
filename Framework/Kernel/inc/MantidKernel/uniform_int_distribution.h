// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <limits>
#include <random>
#include <type_traits>

// Implementation of uniform_int_distribution taken from llvm/libcxx at
// https://github.com/llvm-mirror/libcxx/blob/master/include/algorithm#L2965
//
// Each of our supported platforms has the header but minor differences in the
// results yield problems consistency of our tests across platorms.
//
// Modifications to enable compilation:
//  * define required macros just for this header
//  * prefix some structures with std:: now they are not in std:: namespace
//  * applied clang format

#ifdef _MSC_VER
#define __CHAR_BIT__ CHAR_BIT
#define INLINE_VISIBILITY __forceinline
#else
#define INLINE_VISIBILITY __attribute__((__always_inline__))
#endif

namespace Mantid {
namespace Kernel {

//@cond

// Precondition:  __x != 0
inline INLINE_VISIBILITY unsigned __clz(unsigned __x) {
#ifndef _MSC_VER
  return static_cast<unsigned>(__builtin_clz(__x));
#else
  static_assert(sizeof(unsigned) == sizeof(unsigned long), "");
  static_assert(sizeof(unsigned long) == 4, "");
  unsigned long where;
  // Search from LSB to MSB for first set bit.
  // Returns zero if no set bit is found.
  if (_BitScanReverse(&where, __x))
    return 31 - where;
  return 32; // Undefined Behavior.
#endif
}

inline INLINE_VISIBILITY unsigned long __clz(unsigned long __x) {
#ifndef _MSC_VER
  return static_cast<unsigned long>(__builtin_clzl(__x));
#else
  static_assert(sizeof(unsigned) == sizeof(unsigned long), "");
  return __clz(static_cast<unsigned>(__x));
#endif
}

inline INLINE_VISIBILITY unsigned long long __clz(unsigned long long __x) {
#ifndef _MSC_VER
  return static_cast<unsigned long long>(__builtin_clzll(__x));
#else
  unsigned long where;
// BitScanReverse scans from MSB to LSB for first set bit.
// Returns 0 if no set bit is found.
#if defined(_LIBCPP_HAS_BITSCAN64)
  if (_BitScanReverse64(&where, __x))
    return static_cast<int>(63 - where);
#else
  // Scan the high 32 bits.
  if (_BitScanReverse(&where, static_cast<unsigned long>(__x >> 32)))
    return 63 - (where + 32); // Create a bit offset from the MSB.
  // Scan the low 32 bits.
  if (_BitScanReverse(&where, static_cast<unsigned long>(__x)))
    return 63 - where;
#endif
  return 64; // Undefined Behavior.
#endif // _MSC_VER
}

// __independent_bits_engine

template <unsigned long long _Xp, size_t _Rp> struct __log2_imp {
  static const size_t value = _Xp & ((unsigned long long)(1) << _Rp) ? _Rp : __log2_imp<_Xp, _Rp - 1>::value;
};

template <unsigned long long _Xp> struct __log2_imp<_Xp, 0> {
  static const size_t value = 0;
};

template <size_t _Rp> struct __log2_imp<0, _Rp> {
  static const size_t value = _Rp + 1;
};

template <class _UIntType, _UIntType _Xp> struct __log2 {
  static const size_t value = __log2_imp<_Xp, sizeof(_UIntType) * __CHAR_BIT__ - 1>::value;
};

template <class _Engine, class _UIntType> class __independent_bits_engine {
public:
  // types
  using result_type = _UIntType;

private:
  using _Engine_result_type = typename _Engine::result_type;
  typedef typename std::conditional<sizeof(_Engine_result_type) <= sizeof(result_type), result_type,
                                    _Engine_result_type>::type _Working_result_type;

  _Engine &__e_;
  size_t __w_;
  size_t __w0_;
  size_t __n_;
  size_t __n0_;
  _Working_result_type __y0_;
  _Working_result_type __y1_;
  _Engine_result_type __mask0_;
  _Engine_result_type __mask1_;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4307)
#endif
  static constexpr const _Working_result_type _Rp = _Engine::max() - _Engine::min() + _Working_result_type(1);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
  static constexpr const size_t __m = __log2<_Working_result_type, _Rp>::value;
  static constexpr const size_t _WDt = std::numeric_limits<_Working_result_type>::digits;
  static constexpr const size_t _EDt = std::numeric_limits<_Engine_result_type>::digits;

public:
  // constructors and seeding functions
  __independent_bits_engine(_Engine &__e, size_t __w);

  // generating functions
  result_type operator()() { return __eval(std::integral_constant<bool, _Rp != 0>()); }

private:
  result_type __eval(std::false_type);
  result_type __eval(std::true_type);
};

template <class _Engine, class _UIntType>
__independent_bits_engine<_Engine, _UIntType>::__independent_bits_engine(_Engine &__e, size_t __w)
    : __e_(__e), __w_(__w) {
  __n_ = __w_ / __m + (__w_ % __m != 0);
  __w0_ = __w_ / __n_;
  if (_Rp == 0)
    __y0_ = _Rp;
  else if (__w0_ < _WDt)
    __y0_ = (_Rp >> __w0_) << __w0_;
  else
    __y0_ = 0;
  if (_Rp - __y0_ > __y0_ / __n_) {
    ++__n_;
    __w0_ = __w_ / __n_;
    if (__w0_ < _WDt)
      __y0_ = (_Rp >> __w0_) << __w0_;
    else
      __y0_ = 0;
  }
  __n0_ = __n_ - __w_ % __n_;
  if (__w0_ < _WDt - 1)
    __y1_ = (_Rp >> (__w0_ + 1)) << (__w0_ + 1);
  else
    __y1_ = 0;
  __mask0_ = __w0_ > 0 ? _Engine_result_type(~0) >> (_EDt - __w0_) : _Engine_result_type(0);
  __mask1_ = __w0_ < _EDt - 1 ? _Engine_result_type(~0) >> (_EDt - (__w0_ + 1)) : _Engine_result_type(~0);
}

template <class _Engine, class _UIntType>
inline _UIntType __independent_bits_engine<_Engine, _UIntType>::__eval(std::false_type) {
  return static_cast<result_type>(__e_() & __mask0_);
}

template <class _Engine, class _UIntType>
_UIntType __independent_bits_engine<_Engine, _UIntType>::__eval(std::true_type) {
  const size_t _WRt = std::numeric_limits<result_type>::digits;
  result_type _Sp = 0;
  for (size_t __k = 0; __k < __n0_; ++__k) {
    _Engine_result_type __u;
    do {
      __u = __e_() - _Engine::min();
    } while (__u >= __y0_);
    if (__w0_ < _WRt)
      _Sp <<= __w0_;
    else
      _Sp = 0;
    _Sp += static_cast<result_type>(__u & __mask0_);
  }
  for (size_t __k = __n0_; __k < __n_; ++__k) {
    _Engine_result_type __u;
    do {
      __u = __e_() - _Engine::min();
    } while (__u >= __y1_);
    if (__w0_ < _WRt - 1)
      _Sp <<= __w0_ + 1;
    else
      _Sp = 0;
    _Sp += static_cast<result_type>(__u & __mask1_);
  }
  return _Sp;
}

//@endcond

// uniform_int_distribution

template <class _IntType = int> class uniform_int_distribution {
public:
  // types
  using result_type = _IntType;

  class param_type {
    result_type __a_;
    result_type __b_;

  public:
    using distribution_type = uniform_int_distribution;

    explicit param_type(result_type __a = 0, result_type __b = std::numeric_limits<result_type>::max())
        : __a_(__a), __b_(__b) {}

    result_type a() const { return __a_; }
    result_type b() const { return __b_; }

    friend bool operator==(const param_type &__x, const param_type &__y) {
      return __x.__a_ == __y.__a_ && __x.__b_ == __y.__b_;
    }
    friend bool operator!=(const param_type &__x, const param_type &__y) { return !(__x == __y); }
  };

private:
  param_type __p_;

public:
  // constructors and reset functions
  explicit uniform_int_distribution(result_type __a = 0, result_type __b = std::numeric_limits<result_type>::max())
      : __p_(param_type(__a, __b)) {}
  explicit uniform_int_distribution(const param_type &__p) : __p_(__p) {}
  void reset() {}

  // generating functions
  template <class _URNG> result_type operator()(_URNG &__g) { return (*this)(__g, __p_); }
  template <class _URNG> result_type operator()(_URNG &__g, const param_type &__p);

  // property functions
  result_type a() const { return __p_.a(); }
  result_type b() const { return __p_.b(); }

  param_type param() const { return __p_; }
  void param(const param_type &__p) { __p_ = __p; }

  result_type min() const { return a(); }
  result_type max() const { return b(); }

  friend bool operator==(const uniform_int_distribution &__x, const uniform_int_distribution &__y) {
    return __x.__p_ == __y.__p_;
  }
  friend bool operator!=(const uniform_int_distribution &__x, const uniform_int_distribution &__y) {
    return !(__x == __y);
  }
};

template <class _IntType>
template <class _URNG>
typename uniform_int_distribution<_IntType>::result_type
uniform_int_distribution<_IntType>::operator()(_URNG &__g, const param_type &__p) {
  using _UIntType = typename std::conditional<sizeof(result_type) <= sizeof(uint32_t), uint32_t, uint64_t>::type;
  const _UIntType _Rp = __p.b() - __p.a() + _UIntType(1);
  if (_Rp == 1)
    return __p.a();
  const size_t _Dt = std::numeric_limits<_UIntType>::digits;
  using _Eng = __independent_bits_engine<_URNG, _UIntType>;
  if (_Rp == 0)
    return static_cast<result_type>(_Eng(__g, _Dt)());
  size_t __w = _Dt - __clz(_Rp) - 1;
  if ((_Rp & (std::numeric_limits<_UIntType>::max() >> (_Dt - __w))) != 0)
    ++__w;
  _Eng __e(__g, __w);
  _UIntType __u;
  do {
    __u = __e();
  } while (__u >= _Rp);
  return static_cast<result_type>(__u + __p.a());
}

} // namespace Kernel
} // namespace Mantid

// Clean up macros
#undef INLINE_VISIBILITY
#ifdef _MANTID_CHAR_BIT_DEFINED_HERE
#undef __CHAR_BIT__
#undef _MANTID_CHAR_BIT_DEFINED_HERE
#endif
