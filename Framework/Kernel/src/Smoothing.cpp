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
template <typename T> struct Adder {
  virtual T term(T const &) const = 0;
  virtual T assign(T const &) const = 0;
  void accumulate(T &r, T const &x) const { r += term(x); }
  void separate(T &r, T const &x) const { r -= term(x); };
};

template <typename T> struct SimpleAdder : public Adder<T> {
  T term(T const &x) const override { return x; }
  T assign(T const &x) const override { return x; }
};

template <typename T> struct SumSquareAdder : public Adder<T> {
  T term(T const &x) const override { return x * x; }
  T assign(T const &x) const override { return std::sqrt(std::abs(x)); }
};

template <typename T>
std::vector<T> boxcarSmoothWithFunction(std::vector<T> const &input, unsigned const numPoints, Adder<T> const &adder) {
  if (numPoints < 3) {
    throw std::invalid_argument("Boxcar Smoothing requires at least 3 points in the moving average");
  }
  // copy the input
  std::size_t const vecSize = input.size();
  std::vector<T> output(vecSize);
  unsigned const halfWidth = (numPoints - 1U) / 2U;
  T const width = T(2U * halfWidth + 1U);

  // holds the moving average
  T total(0);
  // First push the values ahead of the current point onto total
  for (unsigned k = 0; k < halfWidth; k++) {
    if (!std::isnan(input[k])) { // exclude NaN
      adder.accumulate(total, input[k]);
    }
  }
  // smoothed values at the beginning, where ewer than numPoints
  for (unsigned k = 0; k <= halfWidth; k++) {
    unsigned const kp = k + halfWidth;
    // add the leading edge
    if (!std::isnan(input[kp])) { // exclude NaN
      adder.accumulate(total, input[kp]);
    }
    output[k] = adder.assign(total / T(kp + 1));
  }
  // main part, each average has numPoints
  for (std::size_t k = halfWidth + 1; k < vecSize - halfWidth; k++) {
    std::size_t const kp = k + halfWidth;
    std::size_t const km = k - halfWidth - 1;
    // remove the previous trailing edge
    if (!std::isnan(input[km])) { // exclude NaN
      adder.separate(total, input[km]);
    }
    // add the new leading edge
    if (!std::isnan(input[kp])) { // exclude NaN
      adder.accumulate(total, input[kp]);
    }
    output[k] = adder.assign(total / width);
  }
  // smoothed values at very end, where fewer than numPoints
  for (std::size_t k = vecSize - halfWidth; k < vecSize; k++) {
    std::size_t const km = k - halfWidth;
    // remove previous trailing edge
    if (!std::isnan(input[km - 1])) { // exclude NaN
      adder.separate(total, input[km - 1]);
    }
    output[k] = adder.assign(total / T(vecSize - km));
  }
  return output;
}
} // namespace detail

template <typename T> std::vector<T> boxcarSmooth(std::vector<T> const &input, unsigned const numPoints) {
  return detail::boxcarSmoothWithFunction(input, numPoints, detail::SimpleAdder<T>());
}

template <typename T> std::vector<T> boxcarSumSquareSmooth(std::vector<T> const &input, unsigned const numPoints) {
  return detail::boxcarSmoothWithFunction(input, numPoints, detail::SumSquareAdder<T>());
}

template MANTID_KERNEL_DLL std::vector<double> boxcarSmooth(std::vector<double> const &, unsigned const);
template MANTID_KERNEL_DLL std::vector<double> boxcarSumSquareSmooth(std::vector<double> const &, unsigned const);

} // namespace Mantid::Kernel::Smoothing
