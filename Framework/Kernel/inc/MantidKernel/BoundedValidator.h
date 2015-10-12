#ifndef MANTID_KERNEL_BOUNDEDVALIDATOR_H_
#define MANTID_KERNEL_BOUNDEDVALIDATOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/TypedValidator.h"
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

    Copyright &copy; 2007-9 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
template <class TYPE>
class DLLExport BoundedValidator : public TypedValidator<TYPE> {
public:
  /// No-arg Constructor
  BoundedValidator()
      : TypedValidator<TYPE>(), m_hasLowerBound(false), m_hasUpperBound(false),
        m_lowerExclusive(false), m_upperExclusive(false), m_lowerBound(TYPE()),
        m_upperBound(TYPE()) {}

  /** Constructor
   * @param lowerBound :: The lower bounding value
   * @param upperBound :: The upper bounding value
   * @param exclusive :: make bounds exclusive (default inclusive)
   */
  BoundedValidator(const TYPE lowerBound, const TYPE upperBound,
                   bool exclusive = false)
      : TypedValidator<TYPE>(), m_hasLowerBound(true), m_hasUpperBound(true),
        m_lowerExclusive(exclusive), m_upperExclusive(exclusive),
        m_lowerBound(lowerBound), m_upperBound(upperBound) {}

  /// Destructor
  virtual ~BoundedValidator() {}

  /// Return if it has a lower bound
  bool hasLower() const { return m_hasLowerBound; }
  /// Return if it has a lower bound
  bool hasUpper() const { return m_hasUpperBound; }
  /// Return the lower bound value
  const TYPE &lower() const { return m_lowerBound; }
  /// Return the upper bound value
  const TYPE &upper() const { return m_upperBound; }
  /// Check if lower bound is exclusive
  bool isLowerExclusive() const { return m_lowerExclusive; }
  /// Check if upper bound is exclusive
  bool isUpperExclusive() const { return m_upperExclusive; }
  /// Set the lower bound to be exclusive
  void setLowerExclusive(const bool exclusive) { m_lowerExclusive = exclusive; }
  /// Set the upper bound to be exclusive
  void setUpperExclusive(const bool exclusive) { m_upperExclusive = exclusive; }

  /// Set both the upper and lower bounds to be exclusive
  void setExclusive(const bool exclusive) {
    setLowerExclusive(exclusive);
    setUpperExclusive(exclusive);
  }

  /// Set lower bound value
  void setLower(const TYPE &value) {
    m_hasLowerBound = true;
    m_lowerBound = value;
  }

  /// Set upper bound value
  void setUpper(const TYPE &value) {
    m_hasUpperBound = true;
    m_upperBound = value;
  }

  /// Clear lower bound value
  void clearLower() {
    m_hasLowerBound = false;
    m_lowerBound = TYPE();
  }
  /// Clear upper bound value
  void clearUpper() {
    m_hasUpperBound = false;
    m_upperBound = TYPE();
  }

  /// Set both bounds (lower and upper) at the same time
  void setBounds(const TYPE &lower, const TYPE &upper) {
    setLower(lower);
    setUpper(upper);
  }

  /// Clear both bounds (lower and upper) at the same time
  void clearBounds() {
    clearLower();
    clearUpper();
  }

  /// Clone the current state
  IValidator_sptr clone() const {
    return boost::make_shared<BoundedValidator>(*this);
  }

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

  /** Checks that the value is within any upper and lower limits
   *
   *  @param value :: The value to test
   *  @returns An error message to display to users or an empty string on no
   *error
   */
  std::string checkValidity(const TYPE &value) const {
    // declare a class that can do conversions to string
    std::ostringstream error;
    // load in the "no error" condition
    error << "";
    // it is allowed not to have a lower bound, if not then you don't need to
    // check
    if (m_hasLowerBound &&
        (value < m_lowerBound || (value == m_lowerBound && m_lowerExclusive))) {
      error << "Selected value " << value << " is ";
      (m_lowerExclusive) ? error << "<=" : error << "<";
      error << " the lower bound (" << m_lowerBound << ")";
    }
    if (m_hasUpperBound &&
        (value > m_upperBound || (value == m_upperBound && m_upperExclusive))) {
      error << "Selected value " << value << " is ";
      (m_upperExclusive) ? error << ">=" : error << ">";
      error << " the upper bound (" << m_upperBound << ")";
    }
    return error.str();
  }
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_BOUNDEDVALIDATOR_H_*/
