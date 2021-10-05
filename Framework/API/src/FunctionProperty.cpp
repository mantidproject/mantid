// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/FunctionProperty.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidKernel/PropertyHistory.h"

#include <json/value.h>

namespace Mantid::API {
/** Constructor.
 *  Sets the property names but initialises the function pointer to null.
 *  @param name :: The name to assign to the property
 *  @param direction :: The direction of the function (i.e. input or output)
 *  @param optional :: A flag indicating whether the property is optional or mandatory.
 */
FunctionProperty::FunctionProperty(const std::string &name, const unsigned int direction,
                                   const PropertyMode::Type optional)
    : Kernel::PropertyWithValue<std::shared_ptr<IFunction>>(name, std::shared_ptr<IFunction>(),
                                                            std::make_shared<Kernel::NullValidator>(), direction),
      m_optional(optional) {}

/** Bring in the PropertyWithValue assignment operator explicitly (avoids VSC++
 * warning)
 * @param value :: The value to set to
 * @return assigned PropertyWithValue
 */
FunctionProperty &FunctionProperty::operator=(const std::shared_ptr<IFunction> &value) {
  Kernel::PropertyWithValue<std::shared_ptr<IFunction>>::operator=(value);
  return *this;
}

//--------------------------------------------------------------------------------------
/// Add the value of another property
FunctionProperty &FunctionProperty::operator+=(Kernel::Property const * /*right*/) {
  throw Kernel::Exception::NotImplementedError("+= operator is not implemented for FunctionProperty.");
}

/** Get the function definition
 *  @return The function definition
 */
std::string FunctionProperty::value() const {
  if (m_value)
    return m_value->asString();
  else
    return getDefault();
}

/**
 * @return A Json::Value object encoding the string representation of the
 * function
 */
Json::Value FunctionProperty::valueAsJson() const { return Json::Value(value()); }

/** Get the value the property was initialised with -its default value
 *  @return The default value
 */
std::string FunctionProperty::getDefault() const { return ""; }

/** Returns true if this property is optional.
 *  @return true if this property is optional.
 */
bool FunctionProperty::isOptional() const { return (m_optional == PropertyMode::Optional); }

/** Set the function definition.
 *  Also tries to create the function with FunctionFactory.
 *  @param value :: The function definition string.
 *  @return Error message from FunctionFactory or "" on success.
 */
std::string FunctionProperty::setValue(const std::string &value) {
  std::string error;

  if (isOptional() && value.empty()) {
    // No error message when the function string is empty and the function is optional
    m_value = std::shared_ptr<IFunction>();
    m_definition = value;
    return error;
  }

  try {
    m_value = std::shared_ptr<IFunction>(FunctionFactory::Instance().createInitialized(value));
    m_definition = value;
  } catch (std::exception &e) {
    error = e.what();
  }
  return error;
}

/**
 * Assumes the Json object is a string and parses it to create the function
 * @param value A Json::Value containing a string
 * @return An empty string indicating success otherwise the string will contain
 * the value of the error.
 */
std::string FunctionProperty::setValueFromJson(const Json::Value &value) {
  try {
    return setValue(value.asString());
  } catch (std::exception &exc) {
    return exc.what();
  }
}

/** Checks whether the entered function is valid.
 *  To be valid it has to be other then default which is no function defined.
 *  @returns A user level description of the problem or "" if it is valid.
 */
std::string FunctionProperty::isValid() const {
  if (isOptional() || direction() == Kernel::Direction::Output) {
    return "";
  } else {
    return isDefault() ? "Function is empty." : "";
  }
}

/** Indicates if the function has not been created yet.
 *  @return true if the function has not been created yet.
 */
bool FunctionProperty::isDefault() const { return m_value == std::shared_ptr<IFunction>(); }

/// Create a history record
/// @return A populated PropertyHistory for this class
const Kernel::PropertyHistory FunctionProperty::createHistory() const { return Kernel::PropertyHistory(this); }

} // namespace Mantid::API
