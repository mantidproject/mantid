#include "MantidQtWidgets/MplCpp/PythonObject.h"
#include "MantidQtWidgets/MplCpp/PythonErrors.h"

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

//-----------------------------------------------------------------------------
// PythonObject
//----------------------------------------------------------------------------

/**
 * @param name Name of the attribute
 * @return The object defining this attribute
 * @throws
 */
PythonObject PythonObject::getAttr(const char *name) const {
  auto objPtr = PyObject_GetAttrString(m_ptr, name);
  if (objPtr)
    return PythonObject(NewRef(objPtr));
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
  auto objPtr = PyImport_ImportModule(name);
  if (!objPtr)
    throw PythonError();
  return PythonObject(NewRef(objPtr));
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
