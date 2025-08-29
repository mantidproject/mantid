// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidPythonInterface/core/PythonObjectProperty.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/PropertyWithValue.hxx"
#include "MantidKernel/PropertyWithValueJSON.h"

namespace Mantid::Kernel {

/**
 * Creates a string representation of the object
 */
template <> std::string toString(PythonObject const &) {
  throw Exception::NotImplementedError("toString(const boost::python::object &)");
}

/**
 * Creates a pretty string representation of the object. In this case it matches
 * the simple string case.
 */
template <> std::string toPrettyString(PythonObject const &value, size_t /*maxLength*/, bool /*collapseLists*/) {
  return toString(value);
}

/**
 * Creates a Json representation of the object
 */
Json::Value encodeAsJson(PythonObject const &) {
  throw Exception::NotImplementedError("encodeAsJson(const boost::python::object &value)");
}

} // namespace Mantid::Kernel

namespace Mantid::PythonInterface {

using Kernel::DataItem;
using Kernel::PropertyWithValue;
using Kernel::Exception::NotImplementedError;

/**
 *  @return An empty string to indicate a default
 */
std::string PythonObjectProperty::getDefault() const { return ""; }

/** Set the function definition.
 *  Also tries to create the function with FunctionFactory.
 *  @param value :: The function definition string.
 *  @return Error message from FunctionFactory or "" on success.
 */
std::string PythonObjectProperty::setValue(std::string const &) {
  throw NotImplementedError("PythonObjectProperty::setValue(const std::string &value)");
}

/**
 * Assumes the Json object is a string and parses it to create the function
 * @param value A Json::Value containing a string
 * @return An empty string indicating success otherwise the string will contain
 * the value of the error.
 */
std::string PythonObjectProperty::setValueFromJson(const Json::Value &) {
  throw NotImplementedError("PythonObjectProperty::setValueFromJson(const Json::Value &value)");
}

/** Indicates if the value matches the default, in this case if the value is None
 *  @return true if the value is None
 */
bool PythonObjectProperty::isDefault() const { return m_value.is_none(); }

} // namespace Mantid::PythonInterface
