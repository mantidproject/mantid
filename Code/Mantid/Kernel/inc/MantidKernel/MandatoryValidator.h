#ifndef MANTID_KERNEL_MANDATORYVALIDATOR_H_
#define MANTID_KERNEL_MANDATORYVALIDATOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/IValidator.h"
#include <string>

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

    Copyright &copy; 2007-8 STFC Rutherford Appleton Laboratory

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
template< class TYPE >
class DLLExport MandatoryValidator : public IValidator<TYPE>
{
public:

   /** Gets the type name of the validator
   *
   *  @return validator type
   */
  const std::string getType() const
  {
	  return "mandatory";
  }

  IValidator<TYPE>* clone() { return new MandatoryValidator(*this); }

private:
  /** Checks if the string is empty
  *
  *  @param value the string to test
  *  @return "A value must be entered for this parameter" if empty or ""
  */
  std::string checkValidity(const TYPE& value) const
  {
	  if ( value.empty() ) return "A value must be entered for this parameter";
	  else return "";
  }

};


// Member function specializations for ints and doubles. The defintions are in the
// cpp file so that multiple symbol errors do not occur in the linking stage.
// 

/** Checks if the integer it is passed equals the flag value 
*  Mantid::EMPTY_DBL(), which implies that it wasn't set by the user
*  @param value the value to test
*  @return "A value must be entered for this parameter" if empty or ""
*/
template<>
std::string MandatoryValidator<int>::checkValidity(const int& value) const;

/** Checks if the double it is passed is within 10 parts per billon of flag
*  value Mantid::EMPTY_DBL(), which implies that it wasn't set by the user
*  @param value the value to test
*  @return "A value must be entered for this parameter" if empty or ""
*/
template<>
std::string MandatoryValidator<double>::checkValidity(const double& value) const;


} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_MANDATORYVALIDATOR_H_*/
