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

#include <memory>

namespace Mantid::Kernel::fft {
// wrap GSL points in unique pointers with deleters for memory leak safety in case of failures
constexpr auto real_wt_deleter = [](gsl_fft_real_wavetable *p) { gsl_fft_real_wavetable_free(p); };
constexpr auto real_ws_deleter = [](gsl_fft_real_workspace *p) { gsl_fft_real_workspace_free(p); };
constexpr auto hc_wt_deleter = [](gsl_fft_halfcomplex_wavetable *p) { gsl_fft_halfcomplex_wavetable_free(p); };

using real_wt_uptr = std::unique_ptr<gsl_fft_real_wavetable, decltype(real_wt_deleter)>;
using real_ws_uptr = std::unique_ptr<gsl_fft_real_workspace, decltype(real_ws_deleter)>;
using hc_wt_uptr = std::unique_ptr<gsl_fft_halfcomplex_wavetable, decltype(hc_wt_deleter)>;

inline real_wt_uptr make_gsl_real_wavetable(std::size_t dn) { return real_wt_uptr(gsl_fft_real_wavetable_alloc(dn)); }
inline real_ws_uptr make_gsl_real_workspace(std::size_t dn) { return real_ws_uptr(gsl_fft_real_workspace_alloc(dn)); }
inline hc_wt_uptr make_gsl_hc_wavetable(std::size_t dn) { return hc_wt_uptr(gsl_fft_halfcomplex_wavetable_alloc(dn)); }
} // namespace Mantid::Kernel::fft

namespace Mantid::Kernel::spline {
// wrap GSL pointers in unique pointers with deleters for memory leak safety in case of failures
constexpr auto interp_accel_deleter = [](gsl_interp_accel *p) { gsl_interp_accel_free(p); };
constexpr auto spline_deleter = [](gsl_spline *p) { gsl_spline_free(p); };

using accel_uptr = std::unique_ptr<gsl_interp_accel, decltype(interp_accel_deleter)>;
using spline_uptr = std::unique_ptr<gsl_spline, decltype(spline_deleter)>;

inline accel_uptr make_interp_accel() { return accel_uptr(gsl_interp_accel_alloc()); }

template <typename X, typename Y> spline_uptr make_cubic_spline(std::vector<X> const &x, std::vector<Y> const &y) {
  spline_uptr spline(gsl_spline_alloc(gsl_interp_cspline, x.size()));
  gsl_spline_init(spline.get(), x.data(), y.data(), x.size());
  return spline;
}
} // namespace Mantid::Kernel::spline
