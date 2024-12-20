// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/WarningSuppressions.h"
#include <ios>
#include <random>

// Implementation of normal_distribution taken from llvm/libcxx at
// https://github.com/llvm-mirror/libcxx/blob/661dff0e60093266e7833cbbf5d3809a4f950378/include/random#L497
//
// Each of our supported platforms has the header but gcc
// (https://github.com/gcc-mirror/gcc/blob/da8dff89fa9398f04b107e388cb706517ced9505/libstdc%2B%2B-v3/include/bits/random.tcc#L1783)
// differs to msvc/clang in which value it caches/returns. This makes writing
// tests against the implementations much harder. This simply choose the libcxx
// header as our "standard".
//
// Modifications to enable compilation:
//  * define required macros just for this header
//  * prefix some structures with std:: now they are not in std:: namespace
//  * include __save_flags helper class for io formatting
//  * disabled a maybe-uninitialized warning that would be disabled in the
//    system header anyway

#ifdef _MSC_VER
#define INLINE_VISIBILITY __forceinline
#else
#define INLINE_VISIBILITY __attribute__((__always_inline__))
#endif

namespace Mantid {
namespace Kernel {

template <class _CharT, class _Traits> class __save_flags {
  using __stream_type = std::basic_ios<_CharT, _Traits>;
  using fmtflags = typename __stream_type::fmtflags;

  __stream_type &__stream_;
  fmtflags __fmtflags_;
  _CharT __fill_;

  __save_flags(const __save_flags &);
  __save_flags &operator=(const __save_flags &);

public:
  INLINE_VISIBILITY
  explicit __save_flags(__stream_type &__stream)
      : __stream_(__stream), __fmtflags_(__stream.flags()), __fill_(__stream.fill()) {}
  INLINE_VISIBILITY
  ~__save_flags() {
    __stream_.flags(__fmtflags_);
    __stream_.fill(__fill_);
  }
};

template <class _RealType = double> class MANTID_KERNEL_DLL normal_distribution {
public:
  // types
  using result_type = _RealType;

  class MANTID_KERNEL_DLL param_type {
    result_type __mean_;
    result_type __stddev_;

  public:
    using distribution_type = normal_distribution;

    INLINE_VISIBILITY
    explicit param_type(result_type __mean = 0, result_type __stddev = 1) : __mean_(__mean), __stddev_(__stddev) {}

    INLINE_VISIBILITY
    result_type mean() const { return __mean_; }
    INLINE_VISIBILITY
    result_type stddev() const { return __stddev_; }

    friend INLINE_VISIBILITY bool operator==(const param_type &__x, const param_type &__y) {
      return __x.__mean_ == __y.__mean_ && __x.__stddev_ == __y.__stddev_;
    }
    friend INLINE_VISIBILITY bool operator!=(const param_type &__x, const param_type &__y) { return !(__x == __y); }
  };

private:
  param_type __p_;
  result_type _V_;
  bool _V_hot_;

public:
  // constructors and reset functions
  INLINE_VISIBILITY
  explicit normal_distribution(result_type __mean = 0, result_type __stddev = 1)
      : __p_(param_type(__mean, __stddev)), _V_hot_(false) {}
  INLINE_VISIBILITY
  explicit normal_distribution(const param_type &__p) : __p_(__p), _V_hot_(false) {}
  INLINE_VISIBILITY
  void reset() { _V_hot_ = false; }

  // generating functions
  template <class _URNG> INLINE_VISIBILITY result_type operator()(_URNG &__g) { return (*this)(__g, __p_); }
  template <class _URNG> result_type operator()(_URNG &__g, const param_type &__p);

  // property functions
  INLINE_VISIBILITY
  result_type mean() const { return __p_.mean(); }
  INLINE_VISIBILITY
  result_type stddev() const { return __p_.stddev(); }

  INLINE_VISIBILITY
  param_type param() const { return __p_; }
  INLINE_VISIBILITY
  void param(const param_type &__p) { __p_ = __p; }

  INLINE_VISIBILITY
  result_type min() const { return -std::numeric_limits<result_type>::infinity(); }
  INLINE_VISIBILITY
  result_type max() const { return std::numeric_limits<result_type>::infinity(); }

  friend INLINE_VISIBILITY bool operator==(const normal_distribution &__x, const normal_distribution &__y) {
    return __x.__p_ == __y.__p_ && __x._V_hot_ == __y._V_hot_ && (!__x._V_hot_ || __x._V_ == __y._V_);
  }
  friend INLINE_VISIBILITY bool operator!=(const normal_distribution &__x, const normal_distribution &__y) {
    return !(__x == __y);
  }

  template <class _CharT, class _Traits, class _RT>
  friend std::basic_ostream<_CharT, _Traits> &operator<<(std::basic_ostream<_CharT, _Traits> &__os,
                                                         const normal_distribution<_RT> &__x);

  template <class _CharT, class _Traits, class _RT>
  friend std::basic_istream<_CharT, _Traits> &operator>>(std::basic_istream<_CharT, _Traits> &__is,
                                                         normal_distribution<_RT> &__x);
};

GNU_DIAG_OFF("maybe-uninitialized")
template <class _RealType>
template <class _URNG>
_RealType normal_distribution<_RealType>::operator()(_URNG &__g, const param_type &__p) {
  result_type _Up;
  if (_V_hot_) {
    _V_hot_ = false;
    _Up = _V_;
  } else {
    std::uniform_real_distribution<result_type> _Uni(-1, 1);
    result_type __u;
    result_type __v;
    result_type __s;
    do {
      __u = _Uni(__g);
      __v = _Uni(__g);
      __s = __u * __u + __v * __v;
    } while (__s > 1 || __s == 0);
    result_type _Fp = std::sqrt(-2 * std::log(__s) / __s);
    _V_ = __v * _Fp;
    _V_hot_ = true;
    _Up = __u * _Fp;
  }
  return _Up * __p.stddev() + __p.mean();
}
GNU_DIAG_ON("maybe-uninitialized")

template <class _CharT, class _Traits, class _RT>
std::basic_ostream<_CharT, _Traits> &operator<<(std::basic_ostream<_CharT, _Traits> &__os,
                                                const normal_distribution<_RT> &__x) {
  __save_flags<_CharT, _Traits> __lx(__os);
  __os.flags(std::ios_base::dec | std::ios_base::left | std::ios_base::fixed | std::ios_base::scientific);
  _CharT __sp = __os.widen(' ');
  __os.fill(__sp);
  __os << __x.mean() << __sp << __x.stddev() << __sp << __x._V_hot_;
  if (__x._V_hot_)
    __os << __sp << __x._V_;
  return __os;
}

template <class _CharT, class _Traits, class _RT>
std::basic_istream<_CharT, _Traits> &operator>>(std::basic_istream<_CharT, _Traits> &__is,
                                                normal_distribution<_RT> &__x) {
  using _Eng = normal_distribution<_RT>;
  using result_type = typename _Eng::result_type;
  using param_type = typename _Eng::param_type;
  __save_flags<_CharT, _Traits> __lx(__is);
  __is.flags(std::ios_base::dec | std::ios_base::skipws);
  result_type __mean;
  result_type __stddev;
  result_type _Vp = 0;
  bool _V_hot = false;
  __is >> __mean >> __stddev >> _V_hot;
  if (_V_hot)
    __is >> _Vp;
  if (!__is.fail()) {
    __x.param(param_type(__mean, __stddev));
    __x._V_hot_ = _V_hot;
    __x._V_ = _Vp;
  }
  return __is;
}

// Clean up macros
#undef INLINE_VISIBILITY

} // namespace Kernel
} // namespace Mantid
