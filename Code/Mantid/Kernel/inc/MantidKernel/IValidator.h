#ifndef MANTID_KERNEL_IVALIDATOR_H_
#define MANTID_KERNEL_IVALIDATOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include "MantidKernel/Logger.h"
#include <string>

namespace Mantid
{
namespace Kernel
{
/** @class IValidator IValidator.h Kernel/IValidator.h

    IValidator is the basic interface for all validators for properties

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
template <typename TYPE>
class DLLExport IValidator
{
public:
  ///virtual Destructor
  virtual ~IValidator() {}

  /** Calls the validator, logs any messages and then passes them on
   *  
   *  @param value The value to be checked
   *  @returns An error message to display to users or an empty string on no error
   */
  std::string isValid(const TYPE &value)
  {
    std::string failure = checkValidity(value);
    if ( failure != "" )
    {//log any problems, problems in the children of composite validators get logged twice, first as themselves then as a composite validator
      g_log.debug() << this->getType() << " validator check failed: " << failure << std::endl;
      //this error message may be shown to users, it is briefer than in the log because it's in context, users wont see composite validator messages because I think they don't care if the validator is a member of a composite or not
      return failure;
    }
    else return "";
  }

  /** Gets the type of the validator as a string
   * 
   *  @returns String describing the type
   */	  
  virtual const std::string getType() const = 0;

  /// Make a copy of the present type of validator
  virtual IValidator* clone() = 0;

protected:
  /** Checks the value based on the validator's rules but does no logging
   * 
   *  @returns An error message to display to users or an empty string on no error
   */
  virtual std::string checkValidity(const TYPE &) const = 0;

private:
  /// Contains the debugging logging stream that validators write errors to
  static Logger& g_log;

};

template <typename TYPE>
Logger& IValidator<TYPE>::g_log = Logger::get("IValidator");

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_IVALIDATOR_H_*/
