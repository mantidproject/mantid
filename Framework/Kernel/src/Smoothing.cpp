// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/Smoothing.h"
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/GSL_FFT_Helpers.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace Mantid::Kernel::Smoothing {

namespace detail {
template <typename T> struct Averager {
  /** A small ABC to represent taking an average over a few values */
  Averager() : m_total(0), m_npts(0) {}
  /** A function returning a "term" in the average
   * @param x the input value, from which the term is computed
   * @returns the term, calculated from x
   */
  virtual T term(T const &x) const = 0;
  /**  Retrieve the average of all included values
   * @returns the average of all included terms
   */
  virtual T getAverage() const = 0;
  /** Include values in the average
   * @param x the value to be included in the averaged
   */
  virtual void accumulate(T const &x) {
    if (!std::isnan(x)) {
      m_total += term(x);
      m_npts++;
    }
  }
  /** Remove values from the average
   * @param x the value to be removed from the averaged
   */
  virtual void separate(T const &x) {
    if (!std::isnan(x)) {
      m_total -= term(x);
      m_npts--;
    }
  }

protected:
  T m_total;
  unsigned int m_npts;
};

/** Represents taking the arithmetic mean */
template <typename T> struct ArithmeticAverager : public Averager<T> {
  T term(T const &x) const override { return x; }
  T getAverage() const override { return this->m_total / this->m_npts; }
};

/** Represents propagating errors for values which had been arithmetically averaged */
template <typename T> struct ErrorPropagationAverager : public Averager<T> {
  T term(T const &x) const override { return x * x; }
  T getAverage() const override { return std::sqrt(std::abs(this->m_total)) / this->m_npts; }
};

/** Represents taking the root-mean-square averaege */
template <typename T> struct SumSquareAverager : public Averager<T> {
  T term(T const &x) const override { return x * x; }
  T getAverage() const override { return std::sqrt(std::abs(this->m_total) / this->m_npts); }
};

/** Represents taking the geometric mean */
template <typename T> struct GeometricAverager : public Averager<T> {
  GeometricAverager() { this->m_total = T(1); }
  void accumulate(T const &x) override {
    if (x != T(0) && !std::isnan(x)) {
      this->m_total *= term(x);
      this->m_npts++;
    }
  }
  void separate(T const &x) override {
    if (x != T(0) && !std::isnan(x)) {
      this->m_total /= term(x);
      this->m_npts--;
    }
  }
  T term(T const &x) const override { return x; }
  T getAverage() const override { return std::pow(std::abs(this->total), 1. / this->npts); }
};

template <typename T>
std::vector<T> boxcarSmoothWithFunction(std::vector<T> const &input, unsigned int const numPoints,
                                        Averager<T> &averager) {
  if (numPoints < 3) {
    throw std::invalid_argument("Boxcar Smoothing requires at least 3 points in the moving average");
  }
  if (numPoints % 2 == 0) {
    throw std::invalid_argument("Boxcar Smoothing requires an odd number of points in the moving average");
  }
  if (input.size() < numPoints) {
    throw std::invalid_argument("Boxcar Smoothing requires the vector size to be greater than the smoothing window");
  }
  // copy the input
  std::size_t const vecSize = input.size();
  std::vector<T> output(vecSize);
  unsigned int const halfWidth = (numPoints - 1U) / 2U;

  // First push the values ahead of the current point onto total
  for (unsigned int k = 0; k < halfWidth; k++) {
    averager.accumulate(input[k]);
  }
  // smoothed values at the beginning, where fewer than numPoints
  for (unsigned int k = 0; k <= halfWidth; k++) {
    unsigned int const kp = k + halfWidth;
    // add the leading edge
    averager.accumulate(input[kp]);
    output[k] = averager.getAverage();
  }
  // main part, each average has numPoints
  for (std::size_t k = halfWidth + 1; k < vecSize - halfWidth; k++) {
    std::size_t const kp = k + halfWidth;
    std::size_t const km = k - halfWidth - 1;
    // remove the previous trailing edge
    averager.separate(input[km]);
    // add the new leading edge
    averager.accumulate(input[kp]);
    output[k] = averager.getAverage();
  }
  // smoothed values at very end, where fewer than numPoints
  for (std::size_t k = vecSize - halfWidth; k < vecSize; k++) {
    std::size_t const km = k - halfWidth;
    // remove previous trailing edge
    averager.separate(input[km - 1]);
    output[k] = averager.getAverage();
  }
  return output;
}
} // namespace detail

template <typename T> std::vector<T> boxcarSmooth(std::vector<T> const &input, unsigned int const numPoints) {
  detail::ArithmeticAverager<T> averager;
  return detail::boxcarSmoothWithFunction(input, numPoints, averager);
}

template <typename T> std::vector<T> boxcarErrorSmooth(std::vector<T> const &input, unsigned int const numPoints) {
  detail::ErrorPropagationAverager<T> averager;
  return detail::boxcarSmoothWithFunction(input, numPoints, averager);
}

template <typename T> std::vector<T> boxcarRMSESmooth(std::vector<T> const &input, unsigned int const numPoints) {
  detail::SumSquareAverager<T> averager;
  return detail::boxcarSmoothWithFunction(input, numPoints, averager);
}

template MANTID_KERNEL_DLL std::vector<double> boxcarSmooth(std::vector<double> const &, unsigned int const);
template MANTID_KERNEL_DLL std::vector<double> boxcarRMSESmooth(std::vector<double> const &, unsigned int const);
template MANTID_KERNEL_DLL std::vector<double> boxcarErrorSmooth(std::vector<double> const &, unsigned int const);

// FFT SMOOTHING METHODS

namespace fft {

// filters
template <typename Y> struct FFTFilter {
  virtual Y operator()(std::size_t const index) const = 0;
  virtual ~FFTFilter() = default;
};

template <typename Y> struct ZeroFilter : public FFTFilter<Y> {
  // remove the higher frequencies by setting to zero
  // REF: see example code at
  // - https://www.gnu.org/software/gsl/doc/html/fft.html#overview-of-real-data-f
  ZeroFilter(std::size_t const n) : m_cutoff(n) {}
  Y operator()(std::size_t const index) const override { return (index >= m_cutoff ? 0 : 1); }

private:
  std::size_t m_cutoff;
};

template <typename Y> struct ButterworthFilter : public FFTFilter<Y> {
  // remove the higher frequencies by tapering with a Butterworth filter
  // SOME REFS:
  // - https://scikit-image.org/docs/0.25.x/api/skimage.filters.html#skimage.filters.butterworth
  // - https://isbweb.org/software/sigproc/bogert/filter.pdf
  // - https://users.cs.cf.ac.uk/dave/Vision_lecture/node22.html
  ButterworthFilter(unsigned int const n, unsigned int const o)
      : m_two_order(2U * o), m_invcutoff(1.0 / static_cast<Y>(n)) {}
  Y operator()(std::size_t const index) const override {
    return 1.0 / (1.0 + std::pow(m_invcutoff * static_cast<Y>(index), m_two_order));
  }

private:
  unsigned m_two_order;
  Y m_invcutoff;
};

template <typename Y> std::vector<Y> fftSmoothWithFilter(std::vector<Y> const &input, FFTFilter<Y> const &filter) {
  std::size_t const N = input.size();
  std::vector<Y> output(input.cbegin(), input.cend());

  // obtain the FFT
  Kernel::fft::real_ws_uptr real_ws = Kernel::fft::make_gsl_real_workspace(N);
  Kernel::fft::real_wt_uptr real_wt = Kernel::fft::make_gsl_real_wavetable(N);
  gsl_fft_real_transform(output.data(), 1, N, real_wt.get(), real_ws.get());
  real_wt.reset(); // wavetable no longer needed

  // NOTE: the halfcomplex storage requires special treatment of even/odd
  // while we could use GSL's unpack, it is easier to alter the values in place
  bool const even = (N % 2 == 0);
  std::size_t const complex_size = (even ? N / 2 : (N - 1) / 2);
  output[0] *= filter(0); // x[0] = z[0].real; z[0].imag = 0 and is not stored anywhere
  for (std::size_t fn = 1; fn < complex_size + (even ? 0UL : 1UL); fn++) {
    output[2 * fn - 1] *= filter(fn); // real parts
    output[2 * fn] *= filter(fn);     // imaginary parts
  }
  // for even data, last point is an unmatched real value
  if (even) {
    output[N - 1] *= filter(complex_size);
  }

  // transform back
  Kernel::fft::hc_wt_uptr hc_wt = Kernel::fft::make_gsl_hc_wavetable(N);
  gsl_fft_halfcomplex_inverse(output.data(), 1, N, hc_wt.get(), real_ws.get());
  hc_wt.reset();   // wavetable no longer needed
  real_ws.reset(); // workspace no longer needed

  // return the smoothed result
  return output;
}

} // namespace fft

template <typename Y> std::vector<Y> fftSmooth(std::vector<Y> const &input, unsigned const cutoff) {
  if (cutoff == 0) {
    throw std::invalid_argument("The cutoff frequency must be greater than zero (" + std::to_string(cutoff) + " <= 0)");
  } else if (cutoff > input.size()) {
    throw std::invalid_argument("The cutoff frequency must be less than the array size( " + std::to_string(cutoff) +
                                " > " + std::to_string(input.size()) + ")");
  }

  fft::ZeroFilter<Y> filter(cutoff);
  return fftSmoothWithFilter(input, filter);
}

template <typename Y>
std::vector<Y> fftButterworthSmooth(std::vector<Y> const &input, unsigned const cutoff, unsigned const order) {
  if (cutoff == 0) {
    throw std::invalid_argument("The Butterworth cutoff frequency must be greater than zero (" +
                                std::to_string(cutoff) + " <= 0)");
  }
  if (cutoff > input.size()) {
    throw std::invalid_argument("The Butterworth cutoff frequency must be less than the array size (" +
                                std::to_string(cutoff) + " > " + std::to_string(input.size()) + ")");
  }
  if (order < 1) {
    throw std::invalid_argument("The Butterworth order must be nonzero (" + std::to_string(order) + " <= 0)");
  }

  fft::ButterworthFilter<Y> filter(cutoff, order);
  return fft::fftSmoothWithFilter(input, filter);
}

template MANTID_KERNEL_DLL std::vector<double> fftSmooth(std::vector<double> const &, unsigned const);
template MANTID_KERNEL_DLL std::vector<double> fftButterworthSmooth(std::vector<double> const &, unsigned const,
                                                                    unsigned const);

} // namespace Mantid::Kernel::Smoothing
