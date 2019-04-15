// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MPLCPP_PYTHON_OBJECT_H
#define MPLCPP_PYTHON_OBJECT_H

#include "MantidPythonInterface/core/ErrorHandling.h"
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"
#include <boost/python/borrowed.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/object.hpp>

/**
 * @file The intetion of this module is to centralize the access
 * to boost::python so that it is not scattered throughout this library. In
 * theory updating to a different wrapper library would just require altering
 * this file.
 */

namespace MantidQt {
namespace Widgets {
namespace Common {
namespace Python {

// Alias for boost python object wrapper
using Object = boost::python::object;

// Alias for boost python dict wrapper
using Dict = boost::python::dict;

// Alias for handle wrapping a raw PyObject*
template <typename T = PyObject> using Handle = boost::python::handle<T>;

// Helper to forward to boost python
inline ssize_t Len(const Python::Object &obj) {
  return boost::python::len(obj);
}

// Helper to create an Object from a new reference to a raw PyObject*
inline Python::Object NewRef(PyObject *obj) {
  if (!obj) {
    throw Mantid::PythonInterface::PythonException();
  }
  return Python::Object(Python::Handle<>(obj));
}

// Helper to create an Object from a borrowed reference to a raw PyObject*
inline Python::Object BorrowedRef(PyObject *obj) {
  if (!obj) {
    throw Mantid::PythonInterface::PythonException();
  }
  return Python::Object(Python::Handle<>(boost::python::borrowed(obj)));
}

// Alias for exception indicating Python error handler is set
using ErrorAlreadySet = boost::python::error_already_set;

/**
 * @brief Holds a Python instance of an object with a method to access it
 */
class InstanceHolder {
public:
  /**
   * Construct an InstanceHolder with an existing Python object.
   * @param obj An existing Python instance
   */
  explicit InstanceHolder(Object obj) : m_instance(std::move(obj)) {}

  /**
   * Construct an InstanceHolder with an existing Python object. The provided
   * object is checked to ensure it has the named attr
   * @param obj An existing Python instance
   * @param attr The name of an attribute that must exist on the wrapped
   * object
   */
  InstanceHolder(Object obj, const char *attr) : m_instance(std::move(obj)) {
    Mantid::PythonInterface::GlobalInterpreterLock lock;
    if (PyObject_HasAttrString(pyobj().ptr(), attr) == 0) {
      throw std::invalid_argument(std::string("object has no attribute ") +
                                  attr);
    }
  }

  /// The destructor must hold the GIL to be able reduce the refcount of
  /// the object
  ~InstanceHolder() {
    Mantid::PythonInterface::GlobalInterpreterLock lock;
    m_instance = Python::Object(); // none
  }

  /// Return the held instance object
  inline const Object &pyobj() const { return m_instance; }

private:
  Object m_instance;
};

} // namespace Python
} // namespace Common
} // namespace Widgets
} // namespace MantidQt

#endif // MPLCPP_PYTHON_OBJECT_H
