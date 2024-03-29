// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/Power.h"
#include "MantidKernel/Exception.h"
#include <cmath>

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid::Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(Power)

Power::Power() : UnaryOperation(), m_exponent(0.) { this->useHistogram = true; }

///////////////////////////////////

void Power::defineProperties() {
  declareProperty("Exponent", 1.0, "The exponent with which to raise base values in the base workspace to.");
}

void Power::retrieveProperties() { m_exponent = getProperty("Exponent"); }

void Power::performUnaryOperation(const double XIn, const double YIn, const double EIn, double &YOut, double &EOut) {
  (void)XIn; // Avoid compiler warning
  YOut = calculatePower(YIn, m_exponent);
  EOut = std::fabs(m_exponent * YOut * (EIn / YIn));
}

inline double Power::calculatePower(const double base, const double exponent) { return std::pow(base, exponent); }
} // namespace Mantid::Algorithms
