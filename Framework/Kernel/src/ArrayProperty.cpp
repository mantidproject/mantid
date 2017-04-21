#include "MantidKernel/ArrayProperty.h"

// PropertyWithValue implementation
#include "MantidKernel/PropertyWithValue.tcc"

namespace Mantid {
namespace Kernel {

/** Constructor
 *  @param name ::      The name to assign to the property
 *  @param vec ::       The initial vector of values to assign to the
 * property.
 *  @param validator :: The validator to use for this property, if required.
 *  @param direction :: The direction (Input/Output/InOut) of this property
 */
template <typename T>
ArrayProperty<T>::ArrayProperty(const std::string &name,
                                const std::vector<T> &vec,
                                IValidator_sptr validator,
                                const unsigned int direction)
    : PropertyWithValue<std::vector<T>>(name, vec, validator, direction) {}

/** Constructor
 *  Will lead to the property having a default-constructed (i.e. empty) vector
 *  as its initial (default) value
 *  @param name ::      The name to assign to the property
 *  @param validator :: The validator to use for this property, if required
 *  @param direction :: The direction (Input/Output/InOut) of this property
 */

template <typename T>
ArrayProperty<T>::ArrayProperty(const std::string &name,
                                IValidator_sptr validator,
                                const unsigned int direction)
    : PropertyWithValue<std::vector<T>>(name, std::vector<T>(), validator,
                                        direction) {}

/** Constructor that's useful for output properties or inputs with an empty
 * default and no validator.
 *  Will lead to the property having a default-constructed (i.e. empty) vector
 *  as its initial (default) value and no validator
 *  @param name ::      The name to assign to the property
 *  @param direction :: The direction (Input/Output/InOut) of this property
 */
template <typename T>
ArrayProperty<T>::ArrayProperty(const std::string &name,
                                const unsigned int direction)
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
template <typename T>
ArrayProperty<T>::ArrayProperty(const std::string &name,
                                const std::string &values,
                                IValidator_sptr validator,
                                const unsigned int direction)
    : PropertyWithValue<std::vector<T>>(name, std::vector<T>(), values,
                                        validator, direction) {}

/// 'Virtual copy constructor'
template <typename T> ArrayProperty<T> *ArrayProperty<T>::clone() const {
  return new ArrayProperty<T>(*this);
}

/** Returns the values stored in the ArrayProperty
 *  @return The stored values as a comma-separated list
 */
template <typename T> std::string ArrayProperty<T>::value() const {
  // Implemented this method for documentation reasons. Just calls base class
  // method.
  return PropertyWithValue<std::vector<T>>::value();
}

/** Sets the values stored in the ArrayProperty from a string representation
 *  @param value :: The values to assign to the property, given as a
 * comma-separated list
 *  @return True if the assignment was successful
 */
template <typename T>
std::string ArrayProperty<T>::setValue(const std::string &value) {
  // Implemented this method for documentation reasons. Just calls base class
  // method.
  return PropertyWithValue<std::vector<T>>::setValue(value);
}

/// @cond

template class DLLExport ArrayProperty<int32_t>;
template class DLLExport ArrayProperty<uint32_t>;
template class DLLExport ArrayProperty<int64_t>;
template class DLLExport ArrayProperty<uint64_t>;
template class DLLExport ArrayProperty<float>;
template class DLLExport ArrayProperty<double>;
template class DLLExport ArrayProperty<std::string>;

template class DLLExport ArrayProperty<std::vector<int32_t>>;
template class DLLExport ArrayProperty<std::vector<uint32_t>>;
template class DLLExport ArrayProperty<std::vector<int64_t>>;
template class DLLExport ArrayProperty<std::vector<uint64_t>>;
template class DLLExport ArrayProperty<std::vector<float>>;
template class DLLExport ArrayProperty<std::vector<double>>;
template class DLLExport ArrayProperty<std::vector<std::string>>;

#if defined(_WIN32) || defined(__clang__) && defined(__APPLE__)
template class DLLExport ArrayProperty<long>;
template class DLLExport ArrayProperty<unsigned long>;
template class DLLExport ArrayProperty<std::vector<long>>;
template class DLLExport ArrayProperty<std::vector<unsigned long>>;
#endif

/// @endcond

} // namespace Kernel
} // namespace Mantid
