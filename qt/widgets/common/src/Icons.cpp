// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/Common/Icons.h"
#include "MantidQtWidgets/Common/Python/Object.h"
#include "MantidQtWidgets/Common/Python/Sip.h"

using namespace Mantid::PythonInterface;

namespace MantidQt {
namespace Widgets {
namespace Common {

namespace {
Python::Object constructArgs(const std::string &iconString) {
  return Python::NewRef(
      Py_BuildValue("(s)", Py_BuildValue("s", iconString.c_str())));
}
Python::Object constructKwargs() { return Python::NewRef(Py_BuildValue("{}")); }
} // namespace

/**
 * Uses the "mantidqt.icons" python library to get the icon, this allows the
 * usage of the same icon libraries as Python.
 * @param iconString
 * @return QIcon
 */
QIcon getIcon(const std::string &iconString) {
  GlobalInterpreterLock lock;
  PyObject *functionsString = PyString_FromString("mantidqt.icons");
  PyObject *funcsModule = PyImport_Import(functionsString);
  PyObject *iconFunc = PyObject_GetAttrString(funcsModule, "get_icon");
  auto args = constructArgs(iconString);
  auto kwargs = constructKwargs();
  auto icon = Python::NewRef(PyObject_Call(iconFunc, args.ptr(), kwargs.ptr()));
  auto qIconPointer = Python::extract<QIcon>(*icon);

  // Return a value not a pointer so memory doesn't leak
  QIcon qIconObject(*qIconPointer);

  // Free memory
  delete qIconPointer;

  return qIconObject;
}

} // namespace Common
} // namespace Widgets
} // namespace MantidQt