// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/FormattingHelpers.h"

#include <cmath>
#include <iomanip>
#include <sstream>

namespace Mantid::DataHandling {

std::string formatDouble(double const value) {
  std::stringstream ss;
  if (std::fabs(value - std::round(value)) < 1e-12) {
    ss << std::fixed << std::setprecision(1) << value;
  } else {
    ss << value;
  }
  return ss.str();
}

} // namespace Mantid::DataHandling
