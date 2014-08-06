#ifndef MANTID_KERNEL_IVALIDATOR_H_
#define MANTID_KERNEL_IVALIDATOR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/DataItem.h"
#include "MantidKernel/Logger.h"
#ifndef Q_MOC_RUN
# include <boost/any.hpp>
# include <boost/shared_ptr.hpp>
# include <boost/make_shared.hpp>
# include <boost/type_traits/is_convertible.hpp>
# include <boost/type_traits/is_pointer.hpp>
#endif
#include <vector>
#include <string>
#include <sstream>
#include <stdexcept>

namespace Mantid
{
namespace Kernel
{
// Forward declaration so that the typedef boost::shared_ptr<Validator> understand it
class IValidator;

/// A shared_ptr to an IValidator
typedef boost::shared_ptr<IValidator> IValidator_sptr;

namespace
{
  /// Helper object to determine if a type is either a pointer/shared_ptr
  /// Generic implementation says it is not
  template<class T>
  struct IsPtrType : public boost::is_pointer<T>
  {};
  /// Helper object to determine if a type is either a pointer/shared_ptr
  /// Specialized implementation for shared_ptr
  template<class T>
  struct IsPtrType<boost::shared_ptr<T> > : public boost::true_type
  {};

}

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
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
    return runCheck(value, IsPtrType<TYPE>());
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
  virtual std::vector<std::string> allowedValues() const { return std::vector<std::string>(); }

  /**
   * Implement this method for validators which wish to support aliasing for alloeed values.
   * @param alias :: A string representation of an alias.
   * @return :: A string representation of an aliased value. Should throw std::invalid_argument
   *    is the given alias is invalid.
   */
  virtual std::string getValueForAlias(const std::string& alias) const { UNUSED_ARG(alias); throw std::invalid_argument("Validator does'n support value aliasing.") ;}
  
  /// Make a copy of the present type of validator
  virtual IValidator_sptr clone() const = 0;

  /** Checks the value based on the validator's rules
   * 
   *  @returns An error message to display to users or an empty string on no error
   */
  virtual std::string check(const boost::any&) const = 0;

private:
  /** Calls the validator for a type that is not a pointer type
   * @param value :: The value to be checked
   * @returns An error message to display to users or an empty string on no error
   */
  template<typename T>
  std::string runCheck(const T & value, const boost::false_type &) const
  {
    const T *valuePtr = &value; // Avoid a copy by storing the pointer in the any holder
    return check(boost::any(valuePtr));
  }

  /** Calls the validator for a type that is either a raw pointer or a boost::shared pointer
   * @returns An error message to display to users or an empty string on no error
   */
  template<typename T>
  std::string runCheck(const T & value, const boost::true_type &) const
  {
    return runCheckWithDataItemPtr(value, boost::is_convertible<T, DataItem_sptr>());
  }
  /** Calls the validator for a pointer type that is NOT convertible to DataItem_sptr
   * @param value :: The value to be checked
   * @returns An error message to display to users or an empty string on no error
   */
  template<typename T>
  std::string runCheckWithDataItemPtr(const T & value, const boost::false_type &) const
  {
    return check(boost::any(value));
  }
  /** Calls the validator for a pointer type that IS convertible to DataItem_sptr
   * @param value :: The value to be checked
   * @returns An error message to display to users or an empty string on no error
   */
  template<typename T>
  std::string runCheckWithDataItemPtr(const T & value, const boost::true_type &) const
  {
    return check(boost::any(boost::static_pointer_cast<DataItem>(value)));
  }
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_IVALIDATOR_H_*/
