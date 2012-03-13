#ifndef MANTID_KERNEL_IVALIDATOR_H_
#define MANTID_KERNEL_IVALIDATOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/DataItem.h"
#include <boost/any.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <set>
#include <string>
#include <sstream>

namespace Mantid
{
namespace Kernel
{
// Forward declaration so that the typedef boost::shared_ptr<Validator>
class IValidator;

/// A shared_ptr to an IValidator
typedef boost::shared_ptr<IValidator> IValidator_sptr;

/** IValidator is the basic interface for all validators for properties

    @author Nick Draper, Tessella Support Services plc
    @date 28/11/2007
    
    Copyright &copy; 2007-2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport IValidator
{
public:
  /// Constructor
  IValidator()
  {}

  ///virtual Destructor
  virtual ~IValidator() {}

  //------------------------------------------------------------------------------------------------------------
  /** Calls the validator
   *  
   *  @param value :: The value to be checked
   *  @returns An error message to display to users or an empty string on no error
   */
  template <typename TYPE>
  std::string isValid(const TYPE &value) const
  {
    return isValidImpl(value, boost::is_convertible<TYPE, DataItem_sptr>());
  }

  /**
   * Deal with a C-style string by first coverting it to a std::string
   * so that boost::any can deal with it, calls isValid(std::string)
   * @param value :: The value to be checked
   * @returns An error message to display to users or an empty string on no error
   */
  std::string isValid(const char * value) const
  {
    return isValid(std::string(value));
  }

  //------------------------------------------------------------------------------------------------------------
  /** The set of allowed values that this validator may have, if a discrete set exists.
   *  Overridden in applicable concrete validators; the base class just returns an empty set.
   *  @return The set of allowed values that this validator may have or an empty set
   */
  virtual std::set<std::string> allowedValues() const { return std::set<std::string>(); }
  
  /// Make a copy of the present type of validator
  virtual IValidator_sptr clone() const = 0;

  /** Checks the value based on the validator's rules
   * 
   *  @returns An error message to display to users or an empty string on no error
   */
  virtual std::string check(const boost::any&) const = 0;

private:
  /** Calls the validator for a general type
   * @param value :: The value to be checked
   * @returns An error message to display to users or an empty string on no error
   */
  template<typename T>
  std::string isValidImpl(const T & value, const boost::false_type &) const
  {
    return check(boost::any(value));
  }

  /** Calls the validator. This ensures the wrapped value is a DataItem_sptr
   * @param value :: The value to be checked
   * @returns An error message to display to users or an empty string on no error
   */
  template<typename T>
  std::string isValidImpl(const T & value, const boost::true_type &) const
  {
    return check(boost::any(boost::static_pointer_cast<DataItem>(value)));
  }

};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_IVALIDATOR_H_*/
