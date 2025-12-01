// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <gsl/gsl_errno.h>
#include <gsl/gsl_fft_halfcomplex.h>
#include <gsl/gsl_fft_real.h>
#include <gsl/gsl_spline.h>

#include "MantidKernel/Strings.h"

#include <memory>
#include <span>
#include <vector>

namespace Mantid::Kernel::fft {

/// Functor to free a GSL objects in a shared pointer
struct GSLFree {
  void operator()(gsl_fft_real_wavetable *p) { gsl_fft_real_wavetable_free(p); }
  void operator()(gsl_fft_real_workspace *p) { gsl_fft_real_workspace_free(p); }
  void operator()(gsl_fft_halfcomplex_wavetable *p) { gsl_fft_halfcomplex_wavetable_free(p); }
};

using real_wt_uptr = std::unique_ptr<gsl_fft_real_wavetable, GSLFree>;
using real_ws_uptr = std::unique_ptr<gsl_fft_real_workspace, GSLFree>;
using hc_wt_uptr = std::unique_ptr<gsl_fft_halfcomplex_wavetable, GSLFree>;

inline real_wt_uptr make_gsl_real_wavetable(std::size_t dn) { return real_wt_uptr(gsl_fft_real_wavetable_alloc(dn)); }
inline real_ws_uptr make_gsl_real_workspace(std::size_t dn) { return real_ws_uptr(gsl_fft_real_workspace_alloc(dn)); }
inline hc_wt_uptr make_gsl_hc_wavetable(std::size_t dn) { return hc_wt_uptr(gsl_fft_halfcomplex_wavetable_alloc(dn)); }
} // namespace Mantid::Kernel::fft

namespace Mantid::Kernel::spline {

/// Minimum number of points needed to fit a cubic spline in GSL
constexpr unsigned int MIN_CSPLINE_POINTS{3};

/// Functor to free a GSL objects in a shared pointer
struct GSLFree {
  void operator()(gsl_spline *spline) { gsl_spline_free(spline); }
  void operator()(gsl_interp_accel *acc) { gsl_interp_accel_free(acc); }
};

using accel_uptr = std::unique_ptr<gsl_interp_accel, GSLFree>;
using spline_uptr = std::unique_ptr<gsl_spline, GSLFree>;

inline accel_uptr make_interp_accel() {
  accel_uptr acc(gsl_interp_accel_alloc());
  if (!acc) {
    throw std::runtime_error("Failed to allocate a GSL interpolation accelerator");
  }
  return acc;
}

template <typename X, typename Y>
spline_uptr make_spline(std::span<X const> x, std::span<Y const> y, gsl_interp_type const *type) {
  std::size_t N = x.size();
  if (N != y.size()) {
    throw std::runtime_error(Strings::strmakef("x and y lengths for spline don't match: %zu vs %zu", N, y.size()));
  }
  if (N == 0) {
    throw std::runtime_error("A spline requires non-empty vectors");
  }
  spline_uptr spline(gsl_spline_alloc(type, N));
  if (!spline) {
    throw std::runtime_error(Strings::strmakef("Failed to allocate a GSL spline with %zu points", N));
  }
  int status = gsl_spline_init(spline.get(), x.data(), y.data(), N);
  if (status != GSL_SUCCESS) {
    throw std::runtime_error("Failed to initialize GSL spline: " + std::string(gsl_strerror(status)));
  }
  return spline;
}

template <typename X, typename Y> spline_uptr make_cubic_spline(std::span<X const> x, std::span<Y const> y) {
  std::size_t N = x.size();
  if (N < MIN_CSPLINE_POINTS) {
    throw std::runtime_error(
        Strings::strmakef("A cubic spline requires %u points, given vector with %zu points", MIN_CSPLINE_POINTS, N));
  }
  return make_spline(x, y, gsl_interp_cspline);
}
} // namespace Mantid::Kernel::spline
