#ifndef PYTHONOBJECT_H
#define PYTHONOBJECT_H

#include "MantidQtWidgets/MplCpp/DllOption.h"
#include "MantidQtWidgets/Common/PythonSystemHeader.h"

#include <utility>

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

namespace detail {

/**
 * @brief Increase the reference count of the given object
 * @param obj A raw Python object pointer
 * @return Object with increased reference count
 */
inline PyObject *incref(PyObject *obj) {
  Py_INCREF(obj);
  return obj;
}

/**
 * @brief Decrease the reference count of the given object. No NULL check.
 * @param obj A raw Python object pointer
 */
inline void decref(PyObject *obj) { Py_DECREF(obj); }

/**
 * @brief Decrease the reference count of the given object. Checks for NULL.
 * @param obj A raw Python object pointer
 */
inline void xdecref(PyObject *obj) { Py_XDECREF(obj); }
}

/**
 * @brief A handle for Python objects to handle reference counting in an
 * RAII style. By default an object is construct as None.
 */
class EXPORT_OPT_MANTIDQT_MPLCPP PythonObject {
public:
  static PythonObject fromNewRef(PyObject *ptr);
  static PythonObject fromBorrowedRef(PyObject *ptr);

  PythonObject() : m_ptr(detail::incref(Py_None)) {}
  ~PythonObject() { detail::xdecref(m_ptr); }
  PythonObject(const PythonObject &other)
      : m_ptr(detail::incref(other.m_ptr)) {}
  PythonObject &operator=(const PythonObject &other) {
    if (&other != this)
      m_ptr = detail::incref(other.m_ptr);
    return *this;
  }
  PythonObject(PythonObject &&other) : m_ptr(nullptr) {
    std::swap(m_ptr, other.m_ptr);
  }
  PythonObject &operator=(PythonObject &&other) {
    if (this != &other) {
      std::swap(m_ptr, other.m_ptr);
    }
    return *this;
  }

  /// Compare two objects. Return true if the refer to the same object.
  bool operator==(const PythonObject &rhs) const { return m_ptr == rhs.m_ptr; }

  /// Return true if this object is the None object
  inline bool isNone() const { return m_ptr == Py_None; }
  /// Return the reference count of the held object
  inline Py_ssize_t refCount() const { return Py_REFCNT(m_ptr); }
  /// Return the reference count of the held object
  inline bool hasAttr(const char *attr) const {
    return (PyObject_HasAttrString(m_ptr, attr) == 1);
  }

  /// Return the raw PyObject ptr handle. Use with care
  inline PyObject *get() const { return m_ptr; }
  /// Call the given method and return the object
  PythonObject getAttr(const char *name) const;
  /// Call the given method without arguments and return an object
  PythonObject callMethod(const char *name) const;

protected:
  /// Protected constructor. You cannot build directly from
  /// a raw pointer. Use one of the static creation functions
  PythonObject(PyObject *ptr) : m_ptr(ptr) {}
  /// Reset the raw PyObject ptr handle.
  inline void reset(PyObject *ptr) { m_ptr = ptr; }

private:
  PyObject *m_ptr;
};

//-----------------------------------------------------------------------------
// Utilities
//----------------------------------------------------------------------------

// Avoids a compiler warning about implicit 'const char *'->'char*' conversion
// in most of Python API under gcc/clang
#define PYSTR_LITERAL(str) const_cast<char *>(str)

/// Import a module and return a new reference
PythonObject importModule(const char *name);
/// Import and return the named object from the given module
PythonObject getAttrOnModule(const char *moduleName, const char *attrName);
}
}
}
#endif // PYTHONOBJECT_H
