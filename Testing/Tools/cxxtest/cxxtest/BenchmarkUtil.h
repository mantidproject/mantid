// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#if !defined(__GNUC__) && !defined(__clang__)
#include <stdexcept>
#endif

namespace CxxTest {
/**
 * Use doNotOptimize for variables that are computed during
 * benchmarking but otherwise are useless. The compiler tends to do a
 * good job at eliminating unused variables, and this function fools it
 * into thinking var is in fact needed.
 *
 * Definition extracted from Google Benchmark:
 * https://github.com/google/benchmark/blob/eacce0b503a81a2910cc1ea0439cf7bc39e3377d/include/benchmark/benchmark.h#L441
 *
 * We only run benchmarks on GCC/Clang currently so this is not defined for other compilers
 */

#if defined(__GNUC__) || defined(__clang__)
#define CXXTEST_ALWAYS_INLINE __attribute__((always_inline))

template <class Tp> inline CXXTEST_ALWAYS_INLINE void doNotOptimize(Tp const &value) {
  asm volatile("" : : "r,m"(value) : "memory");
}

template <class Tp> inline CXXTEST_ALWAYS_INLINE void doNotOptimize(Tp &value) {
#if defined(__clang__)
  asm volatile("" : "+r,m"(value) : : "memory");
#else
  asm volatile("" : "+m,r"(value) : : "memory");
#endif
}
#else

#define CXXTEST_ALWAYS_INLINE __forceinline

template <class Tp> inline CXXTEST_ALWAYS_INLINE void doNotOptimize(Tp const &value) {
  throw std::runtime_error("DoNotOptimize only implemented for gcc/clang");
}
#endif

} // namespace CxxTest
