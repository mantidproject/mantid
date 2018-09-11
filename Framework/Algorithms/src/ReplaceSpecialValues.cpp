//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ReplaceSpecialValues.h"
#include "MantidKernel/Exception.h"
#include <cmath>
#include <limits>

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
  // NaN properties
  declareProperty("NaNValue", Mantid::EMPTY_DBL(),
                  "The value used to replace occurrences of NaN "
                  "(default: do not check).");
  declareProperty("NaNError", 0.0,
                  "The error value used when replacing a value of NaN ");
  // Infinity properties
  declareProperty(
      "InfinityValue", Mantid::EMPTY_DBL(),
      "The value used to replace occurrences of positive or negative infinity "
      "(default: do not check).");
  declareProperty("InfinityError", 0.0,
                  "The error value used when replacing a value of infinity ");
  // Big number properties
  declareProperty("BigNumberThreshold", Mantid::EMPTY_DBL(),
                  "The threshold above which a number (positive or negative) "
                  "should be replaced. "
                  "(default: do not check)");
  declareProperty(
      "BigNumberValue", 0.0,
      "The value with which to replace occurrences of 'big' numbers.");
  declareProperty("BigNumberError", 0.0,
                  "The error value used when replacing a 'big' number");
  // Small number properties
  declareProperty("SmallNumberThreshold", Mantid::EMPTY_DBL(),
                  "The threshold below which a number (positive or negative) "
                  "should be replaced. (default: do not check)");
  declareProperty(
      "SmallNumberValue", 0.0,
      "The value with which to replace occurrences of 'small' numbers.");
  declareProperty("SmallNumberError", 0.0,
                  "The error value used when replacing a 'small' number");
}

void ReplaceSpecialValues::retrieveProperties() {
  m_NaNValue = getProperty("NaNValue");
  m_NaNError = getProperty("NaNError");
  m_InfiniteValue = getProperty("InfinityValue");
  m_InfiniteError = getProperty("InfinityError");
  m_bigThreshold = getProperty("BigNumberThreshold");
  m_bigValue = getProperty("BigNumberValue");
  m_bigError = getProperty("BigNumberError");
  m_smallThreshold = getProperty("SmallNumberThreshold");
  m_smallValue = getProperty("SmallNumberValue");
  m_smallError = getProperty("SmallNumberError");

  m_performNaNCheck = !checkifPropertyEmpty(m_NaNValue);
  m_performInfiniteCheck = !checkifPropertyEmpty(m_InfiniteValue);
  m_performBigCheck = !checkifPropertyEmpty(m_bigThreshold);
  m_performSmallCheck = !checkifPropertyEmpty(m_smallThreshold);

  const bool replacement_set = m_performNaNCheck || m_performInfiniteCheck ||
                               m_performBigCheck || m_performSmallCheck;

  if (!replacement_set) {
    throw std::invalid_argument("No value was defined for NaN, infinity, "
                                "BigValueThreshold or SmallValueThreshold");
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
  } else if (m_performSmallCheck && checkIfSmall(YIn)) {
    YOut = m_smallValue;
    EOut = m_smallError;
  }
}

bool ReplaceSpecialValues::checkIfNan(const double value) const {
  return (std::isnan(value));
}

bool ReplaceSpecialValues::checkIfInfinite(const double value) const {
  return (std::isinf(value));
}

bool ReplaceSpecialValues::checkIfBig(const double value) const {
  return (std::abs(value) > m_bigThreshold);
}

bool ReplaceSpecialValues::checkIfSmall(const double value) const {
  return (std::abs(value) < m_smallThreshold);
}

bool ReplaceSpecialValues::checkifPropertyEmpty(const double value) const {
  return (std::abs(value - Mantid::EMPTY_DBL()) < 1e-08);
}

} // namespace Algorithms
} // namespace Mantid
