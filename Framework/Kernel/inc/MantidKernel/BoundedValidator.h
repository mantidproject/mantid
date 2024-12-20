// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/TypedValidator.h"
#include <memory>
#include <sstream>
#include <string>

namespace Mantid {
namespace Kernel {
/** @class BoundedValidator BoundedValidator.h Kernel/BoundedValidator.h

    BoundedValidator is a validator that requires the values to be between upper
   or lower bounds, or both.
    It offers both inclusive or exclusive bounds. By default the validator uses
   inclusive bounds.

    @author Nick Draper, Tessella Support Services plc
    @date 28/11/2007
*/

namespace {
constexpr int LOWER_BOUND = -1;
constexpr int UPPER_BOUND = 1;
} // namespace

template <class TYPE> class MANTID_KERNEL_DLL BoundedValidator final : public TypedValidator<TYPE> {
public:
  /// No-arg Constructor
  BoundedValidator() noexcept
      : TypedValidator<TYPE>(), m_hasLowerBound(false), m_hasUpperBound(false), m_lowerExclusive(false),
        m_upperExclusive(false), m_lowerBound(TYPE()), m_upperBound(TYPE()), m_hasError(false) {}

  /** Constructor
   * @param lowerBound :: The lower bounding value
   * @param upperBound :: The upper bounding value
   * @param exclusive :: make bounds exclusive (default inclusive)
   */
  BoundedValidator(const TYPE &lowerBound, const TYPE &upperBound, bool exclusive = false) noexcept
      : TypedValidator<TYPE>(), m_hasLowerBound(true), m_hasUpperBound(true), m_lowerExclusive(exclusive),
        m_upperExclusive(exclusive), m_lowerBound(lowerBound), m_upperBound(upperBound), m_hasError(false) {}

  /// Return if it has a lower bound
  bool hasLower() const noexcept { return m_hasLowerBound; }
  /// Return if it has a lower bound
  bool hasUpper() const noexcept { return m_hasUpperBound; }
  /// Return the lower bound value
  TYPE lower() const noexcept { return m_lowerBound; }
  /// Return the upper bound value
  TYPE upper() const noexcept { return m_upperBound; }
  /// Check if lower bound is exclusive
  bool isLowerExclusive() const noexcept { return m_lowerExclusive; }
  /// Check if upper bound is exclusive
  bool isUpperExclusive() const noexcept { return m_upperExclusive; }
  /// Set the lower bound to be exclusive
  void setLowerExclusive(const bool exclusive) noexcept { m_lowerExclusive = exclusive; }
  /// Set the upper bound to be exclusive
  void setUpperExclusive(const bool exclusive) noexcept { m_upperExclusive = exclusive; }

  /// Set both the upper and lower bounds to be exclusive
  void setExclusive(const bool exclusive) noexcept {
    setLowerExclusive(exclusive);
    setUpperExclusive(exclusive);
  }

  /// Set lower bound value
  void setLower(const TYPE &value) noexcept {
    m_hasLowerBound = true;
    m_lowerBound = value;
  }

  /// Set upper bound value
  void setUpper(const TYPE &value) noexcept {
    m_hasUpperBound = true;
    m_upperBound = value;
  }

  /// Clear lower bound value
  void clearLower() noexcept {
    m_hasLowerBound = false;
    m_lowerBound = TYPE();
  }
  /// Clear upper bound value
  void clearUpper() noexcept {
    m_hasUpperBound = false;
    m_upperBound = TYPE();
  }

  /// Set both bounds (lower and upper) at the same time
  void setBounds(const TYPE &lower, const TYPE &upper) noexcept {
    setLower(lower);
    setUpper(upper);
  }

  /// Clear both bounds (lower and upper) at the same time
  void clearBounds() noexcept {
    clearLower();
    clearUpper();
  }

  /// Set the allowed error
  void setError(const TYPE &value) {
    m_error = value;
    m_hasError = true;
  }

  /// Clone the current state
  IValidator_sptr clone() const override { return std::make_shared<BoundedValidator>(*this); }

private:
  // Data and Function Members for This Class Implementation.

  /// Has a lower bound set true/false
  bool m_hasLowerBound;
  /// Has a upper bound set true/false
  bool m_hasUpperBound;
  /// Lower bound is exclusive
  bool m_lowerExclusive;
  /// Upper bound is exclusive
  bool m_upperExclusive;
  /// the lower bound
  TYPE m_lowerBound;
  /// the upper bound
  TYPE m_upperBound;
  /// the allowed error
  TYPE m_error;
  /// Has an error set true/false
  bool m_hasError;

  /** Checks that the value is within any upper and lower limits
   *
   *  @param value :: The value to test
   *  @returns An error message to display to users or an empty string on no
   *error
   */
  std::string checkValidity(const TYPE &value) const override {
    // declare a class that can do conversions to string
    std::ostringstream error;
    // load in the "no error" condition
    error << "";
    // it is allowed not to have a lower bound, if not then you don't need to
    // check
    if (m_hasLowerBound && (value < (errorAdjustment(m_lowerBound, LOWER_BOUND)) ||
                            (value == (errorAdjustment(m_lowerBound, LOWER_BOUND)) && m_lowerExclusive))) {
      error << "Selected value " << value << " is ";
      (m_lowerExclusive) ? error << "<=" : error << "<";
      error << " the lower bound (" << m_lowerBound;
      (error.str() != "" && m_hasError) ? error << " +/- " << m_error << ")" : error << ")";
    }

    if (m_hasUpperBound && (value > (errorAdjustment(m_upperBound, UPPER_BOUND)) ||
                            (value == (errorAdjustment(m_upperBound, UPPER_BOUND)) && m_upperExclusive))) {
      error << "Selected value " << value << " is ";
      (m_upperExclusive) ? error << ">=" : error << ">";
      error << " the upper bound (" << m_upperBound;
      (error.str() != "" && m_hasError) ? error << " +/- " << m_error << ")" : error << ")";
    }
    return error.str();
  }

  TYPE errorAdjustment(const TYPE &boundingValue, const int boundID) const {
    (void)boundID; // avoid unused variable warning.
    return boundingValue;
  }
};

template <> inline void BoundedValidator<std::string>::setError(const std::string &value) {
  (void)value; // avoid unused variable warning.
  throw std::invalid_argument("BoundedValidator<std::string> does not support error.");
}
template <>
inline double BoundedValidator<double>::errorAdjustment(const double &boundingValue, const int boundID) const {
  double adjValue;
  m_hasError ? adjValue = boundingValue + m_error *boundID : adjValue = boundingValue;
  return adjValue;
}
template <> inline int BoundedValidator<int>::errorAdjustment(const int &boundingValue, const int boundID) const {
  int adjValue;
  m_hasError ? adjValue = boundingValue + m_error *boundID : adjValue = boundingValue;
  return adjValue;
}

} // namespace Kernel
} // namespace Mantid
