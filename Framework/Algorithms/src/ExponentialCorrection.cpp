// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ExponentialCorrection.h"
#include "MantidKernel/ListValidator.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid {
namespace Algorithms {
// Register the class into the algorithm factory
DECLARE_ALGORITHM(ExponentialCorrection)

ExponentialCorrection::ExponentialCorrection() : UnaryOperation(), m_c0(0.), m_c1(0.), m_divide(false) {}

void ExponentialCorrection::defineProperties() {
  declareProperty("C0", 1.0, "The value by which the entire exponent calculation is multiplied.");
  declareProperty("C1", 0.0, "The value by which the x value is multiplied prior to exponentiation.");
  getPointerToProperty("InputWorkspace")->setDocumentation("The name of the workspace to apply the correction to.");
  getPointerToProperty("OutputWorkspace")
      ->setDocumentation("The name to use for the corrected workspace (can be "
                         "the same as the input one).");

  std::vector<std::string> operations(2);
  operations[0] = "Multiply";
  operations[1] = "Divide";
  declareProperty("Operation", "Divide", std::make_shared<Kernel::StringListValidator>(operations),
                  "Whether to divide (the default) or multiply the data by the "
                  "correction function.");
}

void ExponentialCorrection::retrieveProperties() {
  m_c0 = getProperty("C0");
  m_c1 = getProperty("C1");
  std::string op = getProperty("Operation");
  m_divide = op == "Divide";
}

void ExponentialCorrection::performUnaryOperation(const double XIn, const double YIn, const double EIn, double &YOut,
                                                  double &EOut) {
  double factor = m_c0 * exp(-1.0 * m_c1 * XIn);
  if (m_divide)
    factor = 1.0 / factor;

  // Multiply the data and error by the correction factor
  YOut = YIn * factor;
  EOut = EIn * factor;
}

} // namespace Algorithms
} // namespace Mantid
