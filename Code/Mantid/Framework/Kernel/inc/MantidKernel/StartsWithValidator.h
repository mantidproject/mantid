#ifndef MANTID_KERNEL_STARTSWITHVALIDATOR_H_
#define MANTID_KERNEL_STARTSWITHVALIDATOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/ListValidator.h"
#ifndef Q_MOC_RUN
# include <boost/lexical_cast.hpp>
#endif
#include <vector>

namespace Mantid
{
namespace Kernel
{
/** StartsWithValidator is a validator that requires the value of a property to start with one 
    of the strings in a defined list of possibilities.

    Copyright &copy; 2008-9 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
class DLLExport StartsWithValidator : public Kernel::StringListValidator
{
public:
  /**
   * Default constructor.
   * 
   */
  StartsWithValidator():Kernel::StringListValidator(){}
  /**
   * Constructor.
   * @param values :: A vector with the allowed values.
   */
  StartsWithValidator(const std::vector<std::string>& values):Kernel::StringListValidator(values){}
  /**
   * Constructor.
   * @param values :: A set with the allowed values.
   */
  StartsWithValidator(const std::set<std::string>& values):Kernel::StringListValidator(values){}
  /// Clone the validator
  IValidator_sptr clone() const{ return boost::make_shared<StartsWithValidator>(*this); }
protected:
  /** Checks if the string passed starts with one from the list
   *  @param value :: The value to test
   *  @return "" if the value is on the list, or "The value does not start with any of the allowed values"
   */
  std::string checkValidity(const std::string & value) const;

};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_STARTSWITHVALIDATOR_H_*/
