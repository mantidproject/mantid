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
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"
#include "MantidPythonInterface/core/IsNone.h"

#include <boost/python/call_method.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/errors.hpp>
#include <boost/python/exec.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/import.hpp>
#include <boost/python/list.hpp>

#include <iostream>

namespace Mantid::Kernel {

/**
 * Creates a string representation of the object
 */
template <> std::string toString(PythonObject const &obj) {
  Mantid::PythonInterface::GlobalInterpreterLock gil;

  if (Mantid::PythonInterface::isNone(obj)) {
    throw Exception::NullPointerException("toString", boost::python::call_method<std::string>(obj.ptr(), "name"));
  } else {
    std::string ret;
    try {
      boost::python::object json = boost::python::import("json");
      ret = boost::python::extract<std::string>(json.attr("dumps")(obj));
    } catch (boost::python::error_already_set const &) {
      ret = boost::python::extract<std::string>(obj.attr("__repr__")());
    }
    return ret;
  }
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
template <> Json::Value encodeAsJson(PythonObject const &) {
  throw Exception::NotImplementedError("encodeAsJson(const boost::python::object &value)");
}

#ifdef __APPLE__
// NOTE for mac builds, it is necessary the DLL export occur here
// Instantiate a copy of the class with our template type so we generate the symbols for the methods in the hxx header.
template class MANTID_PYTHONINTERFACE_CORE_DLL PropertyWithValue<PythonObject>;
#endif

} // namespace Mantid::Kernel

namespace Mantid::PythonInterface {

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
std::string PythonObjectProperty::setValue(std::string const &val) {
  Mantid::PythonInterface::GlobalInterpreterLock gil;
  try {
    boost::python::object json = boost::python::import("json");
    boost::python::str strval(val);
    m_value = boost::python::extract<PythonObject>(json.attr("loads")(strval));
  } catch (boost::python::error_already_set const &) {
    boost::python::str strval(val);
    try {
      m_value = boost::python::eval(strval);
    } catch (boost::python::error_already_set const &) {
      m_value = strval;
    }
    // m_value = PythonObject(val);
  }
  return "";
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

std::string PythonObjectProperty::setDataItem(const std::shared_ptr<Kernel::DataItem> &) {
  throw NotImplementedError("PythonObjectProperty::setDataItem(const std::shared_ptr<Kernel::DataItem> &)");
}

/** Indicates if the value matches the default, in this case if the value is None
 *  @return true if the value is None
 */
bool PythonObjectProperty::isDefault() const { return m_value.is_none(); }

} // namespace Mantid::PythonInterface
