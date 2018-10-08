#include "MantidQtWidgets/MplCpp/BackendQt.h"

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/// Import and return the backend module for this version of Qt
Python::Object backendModule() {
  // Importing PyQt first allows matplotlib to select the correct
  // backend
  Python::NewRef(PyImport_ImportModule(PYQT_MODULE));
  return Python::NewRef(PyImport_ImportModule(MPL_QT_BACKEND));
}

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
