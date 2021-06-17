// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/BackendQt.h"

using namespace MantidQt::Widgets::Common;

namespace MantidQt {
namespace Widgets {
namespace Common {

/// Import and return the backend module for this version of Qt
Python::Object backendModule() {
  // Importing PyQt first allows matplotlib to select the correct
  // backend
  Python::NewRef(PyImport_ImportModule(PYQT_MODULE));
  return Python::NewRef(PyImport_ImportModule(MPL_QT_BACKEND));
}

} // namespace Common
} // namespace Widgets
} // namespace MantidQt
