#include "MantidQtWidgets/MplCpp/PythonObject.h"
#include "MantidQtWidgets/MplCpp/PythonErrors.h"

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

//-----------------------------------------------------------------------------
// PythonObject
//----------------------------------------------------------------------------
/**
 * Static creation from a new reference.
 * @param ptr A reference to a raw PyObject that is assumed to be a new
 * reference
 * @throws PythonError on a null ptr
 */
PythonObject PythonObject::fromNewRef(PyObject *ptr) {
  if (!ptr)
    throw PythonError();
  return PythonObject(ptr);
}

/**
 * Static creation from a borrowed reference.
 * @param ptr A reference to a raw PyObject that is assumed to be a borrowed
 * reference. The reference count is incremented in the way in.
 * @throws PythonError on a null ptr
 */
PythonObject PythonObject::fromBorrowedRef(PyObject *ptr) {
  if (!ptr)
    throw PythonError();
  return PythonObject(detail::incref(ptr));
}

/**
 * @param name Name of the attribute
 * @return The object defining this attribute
 * @throws A PythonError if the attribute does not exist
 */
PythonObject PythonObject::getAttr(const char *name) const {
  auto objPtr = PyObject_GetAttrString(m_ptr, name);
  if (objPtr)
    return PythonObject::fromNewRef(objPtr);
  else {
    throw PythonError();
  }
}

/**
 * @param name Name of the method
 * @return The object returned by the method
 * @throws A PythonError if the method does not exist
 */
PythonObject PythonObject::callMethod(const char *name) const {
  auto objPtr = PyObject_CallMethod(m_ptr, PYSTR_LITERAL(name), nullptr);
  if (objPtr)
    return PythonObject::fromNewRef(objPtr);
  else {
    throw PythonError();
  }
}

//-----------------------------------------------------------------------------
// Utilities
//----------------------------------------------------------------------------
/**
 * Import a module and return a new reference
 * @param name The name of the module
 * @return A PythonObject referencing the module
 * @throws PythonError if the module cannot be imported
 */
PythonObject importModule(const char *name) {
  return PythonObject::fromNewRef(PyImport_ImportModule(name));
}

/**
 * Retrieve an attribute from a module
 * @param moduleName The name of the module containing the attribute
 * @param attrName The name of the attribute
 * @return A PythonObject referencing the attribute
 * @throws PythonError if an error occurred
 */
PythonObject getAttrOnModule(const char *moduleName, const char *attrName) {
  auto module = importModule(moduleName);
  return module.getAttr(attrName);
}
}
}
}
