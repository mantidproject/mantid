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

struct NewRef {
  explicit NewRef(PyObject *ref) : ptr(ref) {}
  // No copy
  NewRef(const NewRef &) = delete;
  NewRef &operator=(const NewRef &) = delete;
  // Move allowed
  NewRef(NewRef &&) = default;
  NewRef &operator=(NewRef &&) = default;

  PyObject *ptr;
};
struct BorrowedRef {
  explicit BorrowedRef(PyObject *ref) : ptr(detail::incref(ref)) {}
  // No copy
  BorrowedRef(const BorrowedRef &) = delete;
  BorrowedRef &operator=(const BorrowedRef &) = delete;
  // Move allowed
  BorrowedRef(BorrowedRef &&) = default;
  BorrowedRef &operator=(BorrowedRef &&) = default;

  PyObject *ptr;
};

/**
 * @brief A handle for Python objects to handle reference counting in an
 * RAII style. By default an object is construct as None.
 */
class EXPORT_OPT_MANTIDQT_MPLCPP PythonObject {
public:
  PythonObject() : m_ptr(detail::incref(Py_None)) {}
  explicit PythonObject(NewRef ref) : m_ptr(ref.ptr) {}
  explicit PythonObject(BorrowedRef ref) : m_ptr(ref.ptr) {}
  ~PythonObject() { detail::xdecref(m_ptr); }
  /// Copy constructor
  PythonObject(const PythonObject &other)
      : m_ptr(detail::incref(other.m_ptr)) {}
  /// Copy from another object
  PythonObject &operator=(const PythonObject &other) {
    if (&other != this)
      m_ptr = detail::incref(other.m_ptr);
    return *this;
  }
  /// Move constructor
  PythonObject(PythonObject &&other) : m_ptr(nullptr) {
    std::swap(m_ptr, other.m_ptr);
  }
  /// Move assignment
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

  /// Return the raw PyObject ptr handle. Use with care
  inline PyObject *get() const { return m_ptr; }
  /// Call the given method and return the object
  PythonObject getAttr(const char *name) const;

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
