// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/Smoothing.h"
#include "MantidKernel/DllConfig.h"

#include <gsl/gsl_errno.h>
#include <gsl/gsl_fft_halfcomplex.h>
#include <gsl/gsl_fft_real.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <vector>

namespace Mantid::Kernel::Smoothing {

// BOXCAR SMOOTHING METHODS

namespace boxcar {
template <typename T> struct Adder {
  Adder() : total(0), npts(0) {}
  virtual ~Adder() = default;
  virtual T term(T const &) const = 0;
  virtual T assign() const = 0;
  virtual void accumulate(T const &x) {
    if (!std::isnan(x)) {
      total += term(x);
      npts++;
    }
  }
  virtual void separate(T const &x) {
    if (!std::isnan(x)) {
      total -= term(x);
      npts--;
    }
  }

protected:
  T total;
  unsigned npts;
};

template <typename T> struct SimpleAdder : public Adder<T> {
  T term(T const &x) const override { return x; }
  T assign() const override { return this->total / this->npts; }
};

template <typename T> struct ErrorPropagationAdder : public Adder<T> {
  T term(T const &x) const override { return x * x; }
  T assign() const override { return std::sqrt(std::abs(this->total)) / this->npts; }
};

template <typename T> struct SumSquareAdder : public Adder<T> {
  T term(T const &x) const override { return x * x; }
  T assign() const override { return std::sqrt(std::abs(this->total) / this->npts); }
};

template <typename T> struct GeometricAdder : public Adder<T> {
  GeometricAdder() { this->total = T(1); }
  void accumulate(T const &x) override {
    if (x != T(0) && !std::isnan(x)) {
      this->total *= term(x);
      this->npts++;
    }
  }
  void separate(T const &x) override {
    if (x != T(0) && !std::isnan(x)) {
      this->total /= term(x);
      this->npts--;
    }
  }
  T term(T const &x) const override { return x; }
  T assign() const override { return std::pow(std::abs(this->total), 1. / this->npts); }
};

template <typename T>
std::vector<T> boxcarSmoothWithFunction(std::vector<T> const &input, unsigned const numPoints, Adder<T> &adder) {
  if (numPoints < 3) {
    throw std::invalid_argument("Boxcar Smoothing requires at least 3 points in the moving average");
  }
  if (input.size() < numPoints) {
    throw std::invalid_argument("Boxcar Smoothing requires the vector size to be greater than the smoothing window");
  }
  // copy the input
  std::size_t const vecSize = input.size();
  std::vector<T> output(vecSize);
  unsigned const halfWidth = (numPoints - 1U) / 2U;

  // First push the values ahead of the current point onto total
  for (unsigned k = 0; k < halfWidth; k++) {
    adder.accumulate(input[k]);
  }
  // smoothed values at the beginning, where fewer than numPoints
  for (unsigned k = 0; k <= halfWidth; k++) {
    unsigned const kp = k + halfWidth;
    // add the leading edge
    adder.accumulate(input[kp]);
    output[k] = adder.assign();
  }
  // main part, each average has numPoints
  for (std::size_t k = halfWidth + 1; k < vecSize - halfWidth; k++) {
    std::size_t const kp = k + halfWidth;
    std::size_t const km = k - halfWidth - 1;
    // remove the previous trailing edge
    adder.separate(input[km]);
    // add the new leading edge
    adder.accumulate(input[kp]);
    output[k] = adder.assign();
  }
  // smoothed values at very end, where fewer than numPoints
  for (std::size_t k = vecSize - halfWidth; k < vecSize; k++) {
    std::size_t const km = k - halfWidth;
    // remove previous trailing edge
    adder.separate(input[km - 1]);
    output[k] = adder.assign();
  }
  return output;
}
} // namespace boxcar

template <typename T> std::vector<T> boxcarSmooth(std::vector<T> const &input, unsigned const numPoints) {
  boxcar::SimpleAdder<T> adder;
  return boxcar::boxcarSmoothWithFunction(input, numPoints, adder);
}

template <typename T> std::vector<T> boxcarErrorSmooth(std::vector<T> const &input, unsigned const numPoints) {
  boxcar::ErrorPropagationAdder<T> adder;
  return boxcar::boxcarSmoothWithFunction(input, numPoints, adder);
}

template <typename T> std::vector<T> boxcarRMSESmooth(std::vector<T> const &input, unsigned const numPoints) {
  boxcar::SumSquareAdder<T> adder;
  return boxcar::boxcarSmoothWithFunction(input, numPoints, adder);
}

template MANTID_KERNEL_DLL std::vector<double> boxcarSmooth(std::vector<double> const &, unsigned const);
template MANTID_KERNEL_DLL std::vector<double> boxcarRMSESmooth(std::vector<double> const &, unsigned const);
template MANTID_KERNEL_DLL std::vector<double> boxcarErrorSmooth(std::vector<double> const &, unsigned const);

// FFT SMOOTHING METHODS

namespace fft {

// filters
template <typename Y> struct FFTFilter {
  virtual Y operator()(std::size_t index) const = 0;
  virtual ~FFTFilter() = default;
};

template <typename Y> struct ZeroFilter : public FFTFilter<Y> {
  // remove the higher frequencies by setting to zero
  // REF: see example code at
  // - https://www.gnu.org/software/gsl/doc/html/fft.html#overview-of-real-data-f
  ZeroFilter(std::size_t n) : cutoff(n) {}
  Y operator()(std::size_t index) const override { return (index >= cutoff ? 0 : 1); }

private:
  std::size_t cutoff;
};

template <typename Y> struct ButterworthFilter : public FFTFilter<Y> {
  // remove the higher frequencies by tapering with a Butterworth filter
  // SOME REFS:
  // - https://scikit-image.org/docs/0.25.x/api/skimage.filters.html#skimage.filters.butterworth
  // - https://isbweb.org/software/sigproc/bogert/filter.pdf
  // - https://users.cs.cf.ac.uk/dave/Vision_lecture/node22.html
  ButterworthFilter(unsigned n, unsigned o) : two_order(2U * o), invcutoff(1.0 / static_cast<Y>(n)) {}
  Y operator()(std::size_t index) const override {
    return 1.0 / (1.0 + std::pow(invcutoff * static_cast<Y>(index), two_order));
  }

private:
  unsigned two_order;
  Y invcutoff;
};

// wrap GSL points in unique pointers with deleters for memory leak safety in case of failures
constexpr auto real_wt_deleter = [](gsl_fft_real_wavetable *p) { gsl_fft_real_wavetable_free(p); };
constexpr auto real_ws_deleter = [](gsl_fft_real_workspace *p) { gsl_fft_real_workspace_free(p); };
constexpr auto hc_wt_deleter = [](gsl_fft_halfcomplex_wavetable *p) { gsl_fft_halfcomplex_wavetable_free(p); };

using real_wt_uptr = std::unique_ptr<gsl_fft_real_wavetable, decltype(real_wt_deleter)>;
using real_ws_uptr = std::unique_ptr<gsl_fft_real_workspace, decltype(real_ws_deleter)>;
using hc_wt_uptr = std::unique_ptr<gsl_fft_halfcomplex_wavetable, decltype(hc_wt_deleter)>;

real_wt_uptr make_gsl_real_wavetable(std::size_t dn) { return real_wt_uptr(gsl_fft_real_wavetable_alloc(dn)); }
real_ws_uptr make_gsl_real_workspace(std::size_t dn) { return real_ws_uptr(gsl_fft_real_workspace_alloc(dn)); }
hc_wt_uptr make_gsl_hc_wavetable(std::size_t dn) { return hc_wt_uptr(gsl_fft_halfcomplex_wavetable_alloc(dn)); }

template <typename Y> std::vector<Y> fftSmoothWithFilter(std::vector<Y> const &input, FFTFilter<Y> const &filter) {
  std::size_t dn = input.size();
  std::vector<Y> output(input.cbegin(), input.cend());

  // obtain the FFT
  fft::real_ws_uptr real_ws = fft::make_gsl_real_workspace(dn);
  fft::real_wt_uptr real_wt = fft::make_gsl_real_wavetable(dn);
  gsl_fft_real_transform(output.data(), 1, dn, real_wt.get(), real_ws.get());
  real_wt.reset();

  // NOTE: the halfcomplex storage requires special treatment of even/odd
  bool even = (dn % 2 == 0);
  std::size_t complex_size = (even ? dn / 2 : (dn - 1) / 2);
  for (std::size_t fn = 0; fn < complex_size - 1; fn++) {
    output[2 * fn - 1] *= filter(fn); // real parts
    output[2 * fn] *= filter(fn);     // imaginary parts
  }
  // handle the last points, which may differ based on even/odd
  output[dn - 2] *= filter(2 * complex_size - 2);
  output[dn - 1] *= filter(2 * complex_size - 2);

  // transform back
  fft::hc_wt_uptr hc_wt = fft::make_gsl_hc_wavetable(dn);
  gsl_fft_halfcomplex_inverse(output.data(), 1, dn, hc_wt.get(), real_ws.get());
  hc_wt.reset();
  real_ws.reset();

  // return the smoothed result
  return output;
}

} // namespace fft

template <typename Y> std::vector<Y> fftSmooth(std::vector<Y> const &input, unsigned const cutoff) {
  if (cutoff == 0) {
    throw std::invalid_argument("The cutoff frequency must be greater than zero");
  } else if (cutoff > input.size()) {
    throw std::invalid_argument("The cutoff frequency must be less than the array size");
  }

  fft::ZeroFilter<Y> filter(cutoff);
  return fftSmoothWithFilter(input, filter);
}

template <typename Y>
std::vector<Y> fftButterworthSmooth(std::vector<Y> const &input, unsigned const cutoff, unsigned const order) {
  if (cutoff == 0) {
    throw std::invalid_argument("The cutoff frequency must be greater than zero");
  }
  if (cutoff > input.size()) {
    throw std::invalid_argument("The Butterworth cutoff frequency must be less than the array size");
  }

  fft::ButterworthFilter<Y> filter(cutoff, order);
  return fft::fftSmoothWithFilter(input, filter);
}

template MANTID_KERNEL_DLL std::vector<double> fftSmooth(std::vector<double> const &, unsigned const);
template MANTID_KERNEL_DLL std::vector<double> fftButterworthSmooth(std::vector<double> const &, unsigned const,
                                                                    unsigned const);

} // namespace Mantid::Kernel::Smoothing
