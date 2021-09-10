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

/** LogarithmicGenerator : TODO: DESCRIPTION
 */
class LogarithmicGenerator {
public:
  LogarithmicGenerator(double start, double increment) : current(start), increment(1.0 + increment) {}

  double operator()() {
    double old = current;
    current *= increment;
    return old;
  }

private:
  double current;
  double increment;
};

} // namespace HistogramData
} // namespace Mantid
