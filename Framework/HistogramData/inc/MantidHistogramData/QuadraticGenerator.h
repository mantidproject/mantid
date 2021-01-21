// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidHistogramData/DllConfig.h"

namespace Mantid {
namespace HistogramData {

/** QuadraticGenerator : Makes quadratics
 */
class QuadraticGenerator {
public:
  QuadraticGenerator(double a0, double a1, double a2) : a0(a0), a1(a1), a2(a2) {}

  double operator()() {
    const double x = static_cast<double>(count++);
    return a0 + a1 * x + a2 * x * x;
  }

private:
  double a0;
  double a1;
  double a2;
  size_t count{0};
};

} // namespace HistogramData
} // namespace Mantid
