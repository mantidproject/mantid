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
  Adder() : total(0), npts(0) {}
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
} // namespace detail

template <typename T> std::vector<T> boxcarSmooth(std::vector<T> const &input, unsigned const numPoints) {
  detail::SimpleAdder<T> adder;
  return detail::boxcarSmoothWithFunction(input, numPoints, adder);
}

template <typename T> std::vector<T> boxcarErrorSmooth(std::vector<T> const &input, unsigned const numPoints) {
  detail::ErrorPropagationAdder<T> adder;
  return detail::boxcarSmoothWithFunction(input, numPoints, adder);
}

template <typename T> std::vector<T> boxcarRMSESmooth(std::vector<T> const &input, unsigned const numPoints) {
  detail::SumSquareAdder<T> adder;
  return detail::boxcarSmoothWithFunction(input, numPoints, adder);
}

template MANTID_KERNEL_DLL std::vector<double> boxcarSmooth(std::vector<double> const &, unsigned const);
template MANTID_KERNEL_DLL std::vector<double> boxcarRMSESmooth(std::vector<double> const &, unsigned const);
template MANTID_KERNEL_DLL std::vector<double> boxcarErrorSmooth(std::vector<double> const &, unsigned const);

} // namespace Mantid::Kernel::Smoothing
