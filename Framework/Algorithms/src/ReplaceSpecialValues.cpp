//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ReplaceSpecialValues.h"
#include "MantidKernel/Exception.h"
#include <limits>
#include <cmath>

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid {
namespace Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(ReplaceSpecialValues)

ReplaceSpecialValues::ReplaceSpecialValues()
    : UnaryOperation(), m_NaNValue(0.), m_NaNError(0.), m_InfiniteValue(0.),
      m_InfiniteError(0.), m_bigThreshold(0.), m_bigValue(0.), m_bigError(0.),
      m_performNaNCheck(false), m_performInfiniteCheck(false),
      m_performBigCheck(false) {}

void ReplaceSpecialValues::defineProperties() {
  declareProperty("NaNValue", Mantid::EMPTY_DBL(),
                  "The value used to replace occurrances of NaN "
                  "(default: do not check).");
  declareProperty("NaNError", 0.0,
                  "The error value used when replacing a value of NaN ");
  declareProperty(
      "InfinityValue", Mantid::EMPTY_DBL(),
      "The value used to replace occurrances of positive or negative infinity "
      "(default: do not check).");
  declareProperty("InfinityError", 0.0,
                  "The error value used when replacing a value of infinity ");
  declareProperty("BigNumberThreshold", Mantid::EMPTY_DBL(),
                  "The threshold above which a number (positive or negative) "
                  "should be replaced. "
                  "(default: do not check)");
  declareProperty(
      "BigNumberValue", 0.0,
      "The value with which to replace occurrances of 'big' numbers.");
  declareProperty("BigNumberError", 0.0,
                  "The error value used when replacing a 'big' number");
}

void ReplaceSpecialValues::retrieveProperties() {
  m_NaNValue = getProperty("NaNValue");
  m_NaNError = getProperty("NaNError");
  m_InfiniteValue = getProperty("InfinityValue");
  m_InfiniteError = getProperty("InfinityError");
  m_bigThreshold = getProperty("BigNumberThreshold");
  m_bigValue = getProperty("BigNumberValue");
  m_bigError = getProperty("BigNumberError");

  m_performNaNCheck = !checkifPropertyEmpty(m_NaNValue);
  m_performInfiniteCheck = !checkifPropertyEmpty(m_InfiniteValue);
  m_performBigCheck = !checkifPropertyEmpty(m_bigThreshold);
  if (!(m_performNaNCheck || m_performInfiniteCheck || m_performBigCheck)) {
    throw std::invalid_argument(
        "No value was defined for NaN, infinity or BigValueThreshold");
  }
}

void ReplaceSpecialValues::performUnaryOperation(const double XIn,
                                                 const double YIn,
                                                 const double EIn, double &YOut,
                                                 double &EOut) {
  (void)XIn; // Avoid compiler warning
  YOut = YIn;
  EOut = EIn;

  if (m_performNaNCheck && checkIfNan(YIn)) {
    YOut = m_NaNValue;
    EOut = m_NaNError;
  } else if (m_performInfiniteCheck && checkIfInfinite(YIn)) {
    YOut = m_InfiniteValue;
    EOut = m_InfiniteError;
  } else if (m_performBigCheck && checkIfBig(YIn)) {
    YOut = m_bigValue;
    EOut = m_bigError;
  }
}

bool ReplaceSpecialValues::checkIfNan(const double &value) const {
  return (std::isnan(value));
}

bool ReplaceSpecialValues::checkIfInfinite(const double &value) const {
  return (std::isinf(value));
}

bool ReplaceSpecialValues::checkIfBig(const double &value) const {
  return (std::abs(value) > m_bigThreshold);
}

bool ReplaceSpecialValues::checkifPropertyEmpty(const double &value) const {
  return (std::abs(value - Mantid::EMPTY_DBL()) < 1e-08);
}

} // namespace Algorithms
} // namespace Mantid
