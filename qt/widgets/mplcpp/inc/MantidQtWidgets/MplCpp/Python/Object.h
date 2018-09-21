#ifndef MPLCPP_PYTHON_OBJECT_H
#define MPLCPP_PYTHON_OBJECT_H
/*
 Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
 National Laboratory & European Spallation Source

 This file is part of Mantid.

 Mantid is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 Mantid is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
*/
#include <boost/python/borrowed.hpp>
#include <boost/python/object.hpp>
#include <stdexcept>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {
namespace Python {

// Alias for boost python object wrapper
using Object = boost::python::object;

// Alias for handle wrapping a raw PyObject*
template <typename T = PyObject> using Handle = boost::python::handle<T>;

// Alias to borrowed function that increments the reference count
template <typename T> using BorrowedRef = boost::python::detail::borrowed<T>;

// Helper to create an Object from a new reference to a raw PyObject*
inline Python::Object NewRef(PyObject *obj) {
  return Python::Object(Python::Handle<>(obj));
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
   * @param attr The name of an attribute that must exist on the wrapped
   * object
   */
  explicit InstanceHolder(Object obj, const char *attr)
      : m_instance(std::move(obj)) {
    if (PyObject_HasAttrString(pyobj().ptr(), attr) == 0) {
      throw std::invalid_argument(
          std::string("object has no attribute ") + attr);
    }
  }

  /// Return the held instance object
  inline const Object &pyobj() const { return m_instance; }

private:
  Object m_instance;
};

} // namespace Python
} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt

#endif // MPLCPP_PYTHON_OBJECT_H
