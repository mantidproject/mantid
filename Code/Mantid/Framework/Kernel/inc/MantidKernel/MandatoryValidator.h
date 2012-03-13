#ifndef MANTID_KERNEL_MANDATORYVALIDATOR_H_
#define MANTID_KERNEL_MANDATORYVALIDATOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/TypedValidator.h"
#include "MantidKernel/EmptyValues.h"

namespace Mantid
{
namespace Kernel
{
/** @class MandatoryValidator MandatoryValidator.h Kernel/MandatoryValidator.h

    Validator to check that a property is not left empty.
    MandatoryValidator is a validator that requires a string to be set to a non-blank value
    or a vector (i.e. ArrayProperty) is not empty.

    @author Nick Draper, Tessella Support Services plc
    @date 28/11/2007

    Copyright &copy; 2007-9 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
template<typename TYPE>
class DLLExport MandatoryValidator : public TypedValidator<TYPE>
{
public:
  IValidator_sptr clone() const { return boost::make_shared<MandatoryValidator>(); }

private:
  /**
   * Check if a value has been provided
   *  @param value :: the string to test
   *  @return "A value must be entered for this parameter" if empty or ""
   */
  std::string checkValidity(const TYPE& value) const
  {
    if ( isEmpty(value) ) return "A value must be entered for this parameter";
    else return "";
  }

  /**
   * Checks if a vector is empty
   * @param value :: A vector of possible values
   * @returns True if the vector is empty
   */
  template <typename T>
  bool isEmpty(const std::vector<T> & values) const
  {
    return values.empty();
  }

  /**
   * Checks if a string is empty
   * @param value :: A test string
   * @returns True if the vector is empty
   */
  bool isEmpty(const std::string & value) const
  {
    return value.empty();
  }

  /**
   * Checks if an integer is empty
   * @param value :: A value to test
   * @returns True if the value is considered empty
   */
  bool isEmpty(const int & value) const
  {
    return (value == Mantid::EMPTY_INT());
  }
  /**
   * Checks if a double is empty
   * @param value :: A vector of possible values
   * @returns True if the value is considered empty
   */
  bool isEmpty(const double & value) const
  {
    if( std::abs(value - Mantid::EMPTY_DBL()) < 1e-08 ) return true;
    else return false;
  }

};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_MANDATORYVALIDATOR_H_*/
