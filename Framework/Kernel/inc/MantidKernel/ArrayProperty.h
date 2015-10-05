#ifndef MANTID_KERNEL_ARRAYPROPERTY_H_
#define MANTID_KERNEL_ARRAYPROPERTY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "PropertyWithValue.h"

namespace Mantid {
namespace Kernel {
/** Support for a property that holds an array of values.
    Implemented as a PropertyWithValue that holds a vector of the desired type.
    This class is really a convenience class to aid in the declaration of the
    property - there's no problem directly using a PropertyWithValue of vector
   type.

    @author Russell Taylor, Tessella Support Services plc
    @date 27/02/2008

    Copyright &copy; 2008-2010 ISIS Rutherford Appleton Laboratory, NScD Oak
   Ridge National Laboratory & European Spallation Source

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
template <typename T>
class DLLExport ArrayProperty : public PropertyWithValue<std::vector<T>> {
public:
  /** Constructor
   *  @param name ::      The name to assign to the property
   *  @param vec ::       The initial vector of values to assign to the
   * property.
   *  @param validator :: The validator to use for this property, if required.
   *  @param direction :: The direction (Input/Output/InOut) of this property
   */
  ArrayProperty(const std::string &name, const std::vector<T> &vec,
                IValidator_sptr validator = IValidator_sptr(new NullValidator),
                const unsigned int direction = Direction::Input)
      : PropertyWithValue<std::vector<T>>(name, vec, validator, direction) {}

  /** Constructor
   *  Will lead to the property having a default-constructed (i.e. empty) vector
   *  as its initial (default) value
   *  @param name ::      The name to assign to the property
   *  @param validator :: The validator to use for this property, if required
   *  @param direction :: The direction (Input/Output/InOut) of this property
   */

  ArrayProperty(const std::string &name, IValidator_sptr validator,
                const unsigned int direction = Direction::Input)
      : PropertyWithValue<std::vector<T>>(name, std::vector<T>(), validator,
                                          direction) {}

  /** Constructor that's useful for output properties or inputs with an empty
   * default and no validator.
   *  Will lead to the property having a default-constructed (i.e. empty) vector
   *  as its initial (default) value and no validator
   *  @param name ::      The name to assign to the property
   *  @param direction :: The direction (Input/Output/InOut) of this property
   */
  ArrayProperty(const std::string &name,
                const unsigned int direction = Direction::Input)
      : PropertyWithValue<std::vector<T>>(name, std::vector<T>(),
                                          IValidator_sptr(new NullValidator),
                                          direction) {}

  /** Constructor from which you can set the property's values through a string:
   *
   * Inherits from the constructor of PropertyWithValue specifically made to
   * handle a list
   * of numeric values in a string format so that initial value is set
   * correctly.
   *
   *  @param name ::      The name to assign to the property
   *  @param values ::    A comma-separated string containing the values to
   * store in the property
   *  @param validator :: The validator to use for this property, if required
   *  @param direction :: The direction (Input/Output/InOut) of this property
   *  @throw std::invalid_argument if the string passed is not compatible with
   * the array type
   */
  ArrayProperty(const std::string &name, const std::string &values,
                IValidator_sptr validator = IValidator_sptr(new NullValidator),
                const unsigned int direction = Direction::Input)
      : PropertyWithValue<std::vector<T>>(name, std::vector<T>(), values,
                                          validator, direction) {}

  /// Copy constructor
  ArrayProperty(const ArrayProperty &right)
      : PropertyWithValue<std::vector<T>>(right) {}

  /// 'Virtual copy constructor'
  ArrayProperty<T> *clone() const { return new ArrayProperty<T>(*this); }

  /// Virtual destructor
  virtual ~ArrayProperty() {}

  // Unhide the base class assignment operator
  using PropertyWithValue<std::vector<T>>::operator=;

  /** Returns the values stored in the ArrayProperty
   *  @return The stored values as a comma-separated list
   */
  std::string value() const {
    // Implemented this method for documentation reasons. Just calls base class
    // method.
    return PropertyWithValue<std::vector<T>>::value();
  }

  /** Sets the values stored in the ArrayProperty from a string representation
   *  @param value :: The values to assign to the property, given as a
   * comma-separated list
   *  @return True if the assignment was successful
   */
  std::string setValue(const std::string &value) {
    // Implemented this method for documentation reasons. Just calls base class
    // method.
    return PropertyWithValue<std::vector<T>>::setValue(value);
  }
  // May want to add specialisation the the class later, e.g. setting just one
  // element of the vector
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_ARRAYPROPERTY_H_*/
