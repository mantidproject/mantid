#ifndef MPLCPP_BACKENDQT_H
#define MPLCPP_BACKENDQT_H
/*
 Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
#include "MantidQtWidgets/MplCpp/Python/Object.h"
#include <QtGlobal>

/*
 * Defines constants relating to the matplotlib backend
 */

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#error "Qt >= 5 required"
#elif QT_VERSION >= QT_VERSION_CHECK(5, 0, 0) &&                               \
    QT_VERSION < QT_VERSION_CHECK(6, 0, 0)

/// Define PyQt version that matches the matplotlib backend
const char *PYQT_MODULE = "PyQt5";

/// Define matplotlib backend that will be used to draw the canvas
const char *MPL_QT_BACKEND = "matplotlib.backends.backend_qt5agg";

#else
#error "Unknown Qt version. Cannot determine matplotlib backend."
#endif

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

#endif // MPLCPP_BACKENDQT_H
