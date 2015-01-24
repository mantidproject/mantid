#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidKernel/PropertyHistory.h"

namespace Mantid {
namespace API {
/** Constructor.
*  Sets the property names but initialises the function pointer to null.
*  @param name :: The name to assign to the property
*/
FunctionProperty::FunctionProperty(const std::string &name)
    : Kernel::PropertyWithValue<boost::shared_ptr<IFunction>>(
          name, boost::shared_ptr<IFunction>(),
          Kernel::IValidator_sptr(new Kernel::NullValidator()),
          Kernel::Direction::InOut) {}

/// Copy constructor
FunctionProperty::FunctionProperty(const FunctionProperty &right)
    : Kernel::PropertyWithValue<boost::shared_ptr<IFunction>>(right) {}

/// Copy assignment operator. Copies the pointer to the function.
FunctionProperty &FunctionProperty::operator=(const FunctionProperty &right) {
  if (&right == this)
    return *this;
  Kernel::PropertyWithValue<boost::shared_ptr<IFunction>>::operator=(right);
  return *this;
}

/** Bring in the PropertyWithValue assignment operator explicitly (avoids VSC++
 * warning)
  * @param value :: The value to set to
  * @return assigned PropertyWithValue
  */
boost::shared_ptr<IFunction> &FunctionProperty::
operator=(const boost::shared_ptr<IFunction> &value) {
  return Kernel::PropertyWithValue<boost::shared_ptr<IFunction>>::operator=(
      value);
}

//--------------------------------------------------------------------------------------
/// Add the value of another property
FunctionProperty &FunctionProperty::operator+=(Kernel::Property const *) {
  throw Kernel::Exception::NotImplementedError(
      "+= operator is not implemented for FunctionProperty.");
  return *this;
}

/// Virtual destructor
FunctionProperty::~FunctionProperty() {}

/** Get the function definition
*  @return The function definition
*/
std::string FunctionProperty::value() const {
  if (m_value)
    return m_value->asString();
  else
    return getDefault();
}

/** Get the value the property was initialised with -its default value
*  @return The default value
*/
std::string FunctionProperty::getDefault() const { return ""; }

/** Set the function definition.
*  Also tries to create the function with FunctionFactory.
*  @param value :: The function definition string.
*  @return Error message from FunctionFactory or "" on success.
*/
std::string FunctionProperty::setValue(const std::string &value) {
  std::string error = "";
  try {
    m_value = boost::shared_ptr<IFunction>(
        FunctionFactory::Instance().createInitialized(value));
    m_definition = value;
  } catch (std::exception &e) {
    error = e.what();
  }
  return error;
}

/** Checks whether the entered function is valid.
*  To be valid it has to be other then default which is no function defined.
*  @returns A user level description of the problem or "" if it is valid.
*/
std::string FunctionProperty::isValid() const {
  return isDefault() ? "Function is empty." : "";
}

/** Indicates if the function has not been created yet.
*  @return true if the function has not been created yet.
*/
bool FunctionProperty::isDefault() const {
  return m_value == boost::shared_ptr<IFunction>();
}

/// Create a history record
/// @return A populated PropertyHistory for this class
const Kernel::PropertyHistory FunctionProperty::createHistory() const {
  return Kernel::PropertyHistory(this);
}

} // namespace API
} // namespace Mantid
