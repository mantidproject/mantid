// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL-3.0 +
#include "MantidPythonInterface/core/PythonObjectProperty.h"
#include "MantidJson/Json.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/PropertyWithValue.hxx"
#include "MantidKernel/PropertyWithValueJSON.h"
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"
#include "MantidPythonInterface/core/IsNone.h"

#include <boost/python/dict.hpp>
#include <boost/python/errors.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/import.hpp>
#include <boost/python/list.hpp>

namespace {
namespace bp = boost::python;

/** Atomic types which can be serialized by python's json.dumps */
std::set<std::string> const jsonAllowedTypes{"int", "float", "str", "NoneType", "bool"};

inline bool isJsonAtomic(bp::object const &obj) {
  std::string const objname = bp::extract<std::string>(obj.attr("__class__").attr("__name__"));
  return jsonAllowedTypes.count(objname) > 0;
}

/** Recurisvely JSONify the object, including replacing custom class objects with a dictionary representation
 * At each stage in the process, will check if the value is one of the above types, or a list-like object.
 * If the object is one of the atomic types, it remains.  If it is a list-like, the below is run on each individual
 * element. If not, the object is replaced with a dictionary, run recursively on each individual value
 * @param obj the object to be recursively written
 * @param depth the current recursion depth, which is limited to ten; beyond this, ellipses will be printed
 * @return a python object which is a dictionary corresponding to `obj`
 */
bp::object recursiveDictDump(bp::object const &obj, unsigned char depth = 0) {
  static unsigned char constexpr max_depth(10);
  bp::object ret;
  // limit recursion depth, to avoid infinity loops or possible segfaults
  if (depth >= max_depth) {
    ret = bp::str("...");
  }
  // if the object can be json-ified already, return this object
  else if (isJsonAtomic(obj)) {
    ret = obj;
  }
  // if the object is a list, json-ify each element of the list
  else if (PyList_Check(obj.ptr()) || PyTuple_Check(obj.ptr())) {
    bp::list ls;
    for (bp::ssize_t i = 0; i < bp::len(obj); i++) {
      ls.append(recursiveDictDump(obj[i], depth + 1));
    }
    ret = ls;
  }
  // if the object is a dictionary, json-ify all of the values
  else if (PyDict_Check(obj.ptr())) {
    bp::dict d;
    bp::list keyvals = bp::extract<bp::dict>(obj)().items();
    for (bp::ssize_t i = 0; i < bp::len(keyvals); i++) {
      bp::object key = keyvals[i][0];
      bp::object val = keyvals[i][1];
      d[key] = recursiveDictDump(val, depth + 1);
    }
    ret = d;
  }
  // if the object is not one of the above types, then extract its __dict__ method and repeat the above
  else if (PyObject_HasAttrString(obj.ptr(), "__dict__")) {
    bp::dict d = bp::extract<bp::dict>(obj.attr("__dict__"));
    ret = recursiveDictDump(d, depth); // NOTE re-run at same depth
  } else {
    ret = bp::str(obj);
  }
  return ret;
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
  // otherwise, use json to return a string representation of the class
  else {
    // try loading as a json -- will work for most 'built-in' types
    boost::python::object json = boost::python::import("json");
    try {
      rep = json.attr("dumps")(obj);
    }
    // if json doesn't work, then build a json-like dictionary representation of the object and dump that
    catch (boost::python::error_already_set const &) {
      PyErr_Clear(); // NOTE must clear error registry, or bizarre errors will occur at unexpected lines
      boost::python::object dict = recursiveDictDump(obj);
      try {
        rep = json.attr("dumps")(dict);
      } catch (boost::python::error_already_set const &) {
        PyErr_Clear();
        rep = boost::python::str("<unrepresentable object>");
      }
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

#if defined(__APPLE__) || (defined(__linux__) && defined(__clang__))
/** NOTE:
 *  For mac builds, it is necessary the DLL export occur here.
 *  This declaration normally lives in Framework/Kernel/PropertyWithValue.cpp.  However, because the boost library is
 *  not linked in Kernel, this declaration can only occur in a file inside the PythonInterface layer
 *  For mac builds, this MUST occur inside a source file, and not in the header
 *  For Linux with clang, we also need it in the source file to avoid linking issues
 */
// Instantiate a copy of the class with our template type so we generate the symbols for the methods in the hxx header.
template class MANTID_PYTHONINTERFACE_CORE_DLL PropertyWithValue<PythonObject>;
#endif

} // namespace Mantid::Kernel

namespace Mantid::PythonInterface {

using Kernel::PropertyWithValue;
using Kernel::Exception::NotImplementedError;

/**
 *  @return A string representati on of the default value
 */
std::string PythonObjectProperty::getDefault() const { return Mantid::Kernel::toString(m_initialValue); }

/** Set the property value.
 *  @param value :: The object definition string.
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

/** Set the property value.
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
  } catch (std::invalid_argument const &except) {
    ret = except.what();
  }
  return ret;
}

/**
 * Assumes the Json object is a string and parses it to create the object
 * @param value A Json::Value containing a string
 * @return An empty string indicating success otherwise the string will contain
 * the value of the error.
 */
std::string PythonObjectProperty::setValueFromJson(const Json::Value &json) {
  std::string jsonstr = JsonHelpers::jsonToString(json);
  return setValue(jsonstr);
}

std::string PythonObjectProperty::setDataItem(const std::shared_ptr<Kernel::DataItem> &) {
  throw NotImplementedError("PythonObjectProperty::setDataItem(const std::shared_ptr<Kernel::DataItem> &)");
}

/** Indicates if the value matches the value None
 *  @return true if the value is None
 *  NOTE: For reasons (surely good ones), trying to compare m_value to m_initialValue raises a segfault
 */
bool PythonObjectProperty::isDefault() const { return m_value.is_none(); }

} // namespace Mantid::PythonInterface
