// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/Smoothing.h"
#include "MantidKernel/DllConfig.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace Mantid::Kernel::Smoothing {

namespace detail {
template <typename T> struct Averager {
  Averager() : total(0), npts(0) {}
  virtual T term(T const &) const = 0;
  virtual T getAverage() const = 0;
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

template <typename T> struct ArithmeticAverager : public Averager<T> {
  T term(T const &x) const override { return x; }
  T getAverage() const override { return this->total / this->npts; }
};

template <typename T> struct ErrorPropagationAverager : public Averager<T> {
  T term(T const &x) const override { return x * x; }
  T getAverage() const override { return std::sqrt(std::abs(this->total)) / this->npts; }
};

template <typename T> struct SumSquareAverager : public Averager<T> {
  T term(T const &x) const override { return x * x; }
  T getAverage() const override { return std::sqrt(std::abs(this->total) / this->npts); }
};

template <typename T> struct GeometricAverager : public Averager<T> {
  GeometricAverager() { this->total = T(1); }
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
  T getAverage() const override { return std::pow(std::abs(this->total), 1. / this->npts); }
};

template <typename T>
std::vector<T> boxcarSmoothWithFunction(std::vector<T> const &input, unsigned const numPoints, Averager<T> &averager) {
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
  unsigned const halfWidth = (numPoints - 1U) / 2U;

  // First push the values ahead of the current point onto total
  for (unsigned k = 0; k < halfWidth; k++) {
    averager.accumulate(input[k]);
  }
  // smoothed values at the beginning, where fewer than numPoints
  for (unsigned k = 0; k <= halfWidth; k++) {
    unsigned const kp = k + halfWidth;
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

template <typename T> std::vector<T> boxcarSmooth(std::vector<T> const &input, unsigned const numPoints) {
  detail::ArithmeticAverager<T> averager;
  return detail::boxcarSmoothWithFunction(input, numPoints, averager);
}

template <typename T> std::vector<T> boxcarErrorSmooth(std::vector<T> const &input, unsigned const numPoints) {
  detail::ErrorPropagationAverager<T> averager;
  return detail::boxcarSmoothWithFunction(input, numPoints, averager);
}

template <typename T> std::vector<T> boxcarRMSESmooth(std::vector<T> const &input, unsigned const numPoints) {
  detail::SumSquareAverager<T> averager;
  return detail::boxcarSmoothWithFunction(input, numPoints, averager);
}

template MANTID_KERNEL_DLL std::vector<double> boxcarSmooth(std::vector<double> const &, unsigned const);
template MANTID_KERNEL_DLL std::vector<double> boxcarRMSESmooth(std::vector<double> const &, unsigned const);
template MANTID_KERNEL_DLL std::vector<double> boxcarErrorSmooth(std::vector<double> const &, unsigned const);

} // namespace Mantid::Kernel::Smoothing
