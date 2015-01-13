//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/OneMinusExponentialCor.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid {
namespace Algorithms {
// Register the class into the algorithm factory
DECLARE_ALGORITHM(OneMinusExponentialCor)

void OneMinusExponentialCor::defineProperties() {
  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty("C", 1.0, mustBePositive, "The positive value by which the "
                                            "entire exponent calculation is "
                                            "multiplied (see formula below).");

  declareProperty("C1", 1.0, "The value by which the entire calculation is "
                             "multiplied (see formula below).");

  std::vector<std::string> operations(2);
  operations[0] = "Multiply";
  operations[1] = "Divide";
  declareProperty("Operation", "Divide",
                  boost::make_shared<Kernel::StringListValidator>(operations),
                  "Whether to divide (the default) or multiply the data by the "
                  "correction function.");
}

void OneMinusExponentialCor::retrieveProperties() {
  m_c = getProperty("C");
  m_c1 = getProperty("C1");
  std::string op = getProperty("Operation");
  m_divide = (op == "Divide") ? true : false;
}

void OneMinusExponentialCor::performUnaryOperation(const double XIn,
                                                   const double YIn,
                                                   const double EIn,
                                                   double &YOut, double &EOut) {
  double factor = m_c1 * (1.0 - exp(-1.0 * m_c * XIn));
  if (m_divide)
    factor = 1.0 / factor;

  // Multiply the data and error by the correction factor
  YOut = YIn * factor;
  EOut = EIn * factor;
}

} // namespace Algorithms
} // namespace Mantid
