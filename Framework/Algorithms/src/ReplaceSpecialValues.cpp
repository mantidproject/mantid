// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ReplaceSpecialValues.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/Exception.h"
#include <cmath>
#include <limits>

using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace Mantid::Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(ReplaceSpecialValues)

ReplaceSpecialValues::ReplaceSpecialValues()
    : UnaryOperation(), m_NaNValue(0.), m_NaNError(0.), m_InfiniteValue(0.), m_InfiniteError(0.), m_bigThreshold(0.),
      m_bigValue(0.), m_bigError(0.), m_performNaNCheck(false), m_performInfiniteCheck(false), m_performBigCheck(false),
      m_checkErrors(false) {}

void ReplaceSpecialValues::defineProperties() {
  // NaN properties
  declareProperty("NaNValue", Mantid::EMPTY_DBL(),
                  "The value used to replace occurrences of NaN "
                  "(default: do not check).");
  declareProperty("NaNError", 0.0, "The error value used when replacing a value of NaN ");
  // Infinity properties
  declareProperty("InfinityValue", Mantid::EMPTY_DBL(),
                  "The value used to replace occurrences of positive or negative infinity "
                  "(default: do not check).");
  declareProperty("InfinityError", 0.0, "The error value used when replacing a value of infinity ");
  // Big number properties
  declareProperty("BigNumberThreshold", Mantid::EMPTY_DBL(),
                  "The threshold above which a number (positive or negative) "
                  "should be replaced. "
                  "(default: do not check)");
  declareProperty("BigNumberValue", 0.0, "The value with which to replace occurrences of 'big' numbers.");
  declareProperty("BigNumberError", 0.0, "The error value used when replacing a 'big' number");
  // Small number properties
  declareProperty("SmallNumberThreshold", Mantid::EMPTY_DBL(),
                  "The threshold below which a number (positive or negative) "
                  "should be replaced. (default: do not check)");
  declareProperty("SmallNumberValue", 0.0, "The value with which to replace occurrences of 'small' numbers.");
  declareProperty("SmallNumberError", 0.0, "The error value used when replacing a 'small' number");
  declareProperty("CheckErrorAxis", false, "Whether or not to also check the error axis values.");
  declareProperty("UseAbsolute", true, "Whether large and small comparisons should be done on absolute values.");
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
  m_checkErrors = getProperty("CheckErrorAxis");
  m_useAbsolute = getProperty("UseAbsolute");

  m_performNaNCheck = !checkifPropertyEmpty(m_NaNValue);
  m_performInfiniteCheck = !checkifPropertyEmpty(m_InfiniteValue);
  m_performBigCheck = !checkifPropertyEmpty(m_bigThreshold);
  m_performSmallCheck = !checkifPropertyEmpty(m_smallThreshold);

  const bool replacement_set = m_performNaNCheck || m_performInfiniteCheck || m_performBigCheck || m_performSmallCheck;

  if (!replacement_set) {
    throw std::invalid_argument("No value was defined for NaN, infinity, "
                                "BigValueThreshold or SmallValueThreshold");
  }
}

void ReplaceSpecialValues::performUnaryOperation(const double XIn, const double YIn, const double EIn, double &YOut,
                                                 double &EOut) {
  (void)XIn; // Avoid compiler warning
  YOut = YIn;
  EOut = EIn;

  if (m_performNaNCheck && (checkIfNan(YIn) || (m_checkErrors && checkIfNan(EIn)))) {
    YOut = m_NaNValue;
    EOut = m_NaNError;
  } else if (m_performInfiniteCheck && (checkIfInfinite(YIn) || (m_checkErrors && checkIfInfinite(EIn)))) {
    YOut = m_InfiniteValue;
    EOut = m_InfiniteError;
  } else if (m_performBigCheck && (checkIfBig(YIn) || (m_checkErrors && checkIfBig(EIn)))) {
    YOut = m_bigValue;
    EOut = m_bigError;
  } else if (m_performSmallCheck && (checkIfSmall(YIn) || (m_checkErrors && checkIfSmall(EIn)))) {
    YOut = m_smallValue;
    EOut = m_smallError;
  }
}

bool ReplaceSpecialValues::checkIfNan(const double value) const { return (std::isnan(value)); }

bool ReplaceSpecialValues::checkIfInfinite(const double value) const { return (std::isinf(value)); }

bool ReplaceSpecialValues::checkIfBig(const double value) const {
  return m_useAbsolute ? std::abs(value) > m_bigThreshold : value > m_bigThreshold;
}

bool ReplaceSpecialValues::checkIfSmall(const double value) const {
  return m_useAbsolute ? std::abs(value) < m_smallThreshold : value < m_smallThreshold;
}

bool ReplaceSpecialValues::checkifPropertyEmpty(const double value) const {
  return (std::abs(value - Mantid::EMPTY_DBL()) < 1e-08);
}

} // namespace Mantid::Algorithms
