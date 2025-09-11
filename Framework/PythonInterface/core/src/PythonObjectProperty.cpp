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

namespace {
namespace bp = boost::python;

std::set<std::string> jsonAllowedTypes{"int", "float", "str", "dict", "list", "tuple", "NoneType", "bool"};

bp::object iterativeDictDump(bp::object obj) {
  bp::dict d = bp::extract<bp::dict>(obj.attr("__dict__"));
  bp::list keyvals = d.items();
  for (bp::ssize_t i = 0; i < bp::len(keyvals); i++) {
    bp::object key = keyvals[i][0];
    bp::object val = keyvals[i][1];
    std::string valtype = bp::extract<std::string>(val.attr("__class__").attr("__name__"));
    if (!jsonAllowedTypes.count(valtype)) {
      d[key] = iterativeDictDump(val);
    }
  }
  return bp::object(d);
}
} // namespace

namespace Mantid::Kernel {

/**
 * Creates a string representation of the object
 */
template <> std::string toString(PythonObject const &obj) {
  Mantid::PythonInterface::GlobalInterpreterLock gil;

  // std::string ret;
  boost::python::object rep;
  // if the object is None type, return an empty string
  if (Mantid::PythonInterface::isNone(obj)) {
    rep = boost::python::str("");
  }
  // if the object can be read as a string, then return the object itself
  else if (boost::python::extract<std::string>(obj).check()) {
    rep = obj;
  }
  // otherwise, use either json to return a string representation of the class
  else {
    // try loading as a json -- will work for most 'built-in' types
    boost::python::object json = boost::python::import("json");
    try {
      rep = json.attr("dumps")(obj);
    }
    // if json doesn't work, then iteratively dump its dict as a json
    catch (boost::python::error_already_set const &e) {
      PyErr_Clear(); // NOTE must clear error registry, or bizarre errors will occur at unexpected lines
      boost::python::object dict = iterativeDictDump(obj);
      rep = (json.attr("dumps")(dict)).attr("encode")("utf-8");
    }
  }
  return boost::python::extract<std::string>(rep);
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
  throw Exception::NotImplementedError("encodeAsJson(const boost::python::object &)");
}

#ifdef __APPLE__
// NOTE for mac builds, it is necessary the DLL export occur here.
// This declaration normally lives in Framework/Kernel/PropertyWithValue.cpp.  However, because the boost library is
// not linked in Kernel, this declarationcan only occur in a file inside the PythonInterface layer
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

/** Set the property value.
 *  Also tries to create the function with FunctionFactory.
 *  @param value :: The function definition string.
 *  @return Error message, or "" on success.
 */
std::string PythonObjectProperty::setValue(PythonObject const &value) {
  std::string ret;
  try {
    *this = value;
  } catch (std::invalid_argument const &except) {
    ret = except.what();
  }
  return ret;
}

/** Set the propety value..
 *  @param value :: The value of the property as a string.
 *  @return Error message, or "" on success.
 */
std::string PythonObjectProperty::setValue(std::string const &value) {
  Mantid::PythonInterface::GlobalInterpreterLock gil;

  std::string ret;
  boost::python::object newVal;
  // try to load as a json object
  try {
    boost::python::object json = boost::python::import("json");
    newVal = json.attr("loads")(value);
  }
  // if it cannot be loaded as a json then it is probably a string
  catch (boost::python::error_already_set const &) {
    PyErr_Clear(); // NOTE must clear error registry, or bizarre errors will occur
    try {
      newVal = boost::python::str(value.c_str());
    } catch (boost::python::error_already_set const &) {
      PyErr_Clear(); // NOTE must clear error registry, or bizarre errors will occur
      return "Failed to interpret string as JSON or string property: " + value;
    }
  }

  // use the assignment operator, which also calls the validator
  try {
    *this = newVal;
  } catch (std::invalid_argument &except) {
    ret = except.what();
  }
  return ret;
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
