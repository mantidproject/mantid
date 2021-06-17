// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "DllOption.h"
#include "MantidQtWidgets/Common/Python/Object.h"
#include <QtGlobal>

/*
 * Defines constants relating to the matplotlib backend
 */

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#error "Qt >= 5 required"
#elif QT_VERSION >= QT_VERSION_CHECK(5, 0, 0) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)

/// Define PyQt version that matches the matplotlib backend
constexpr static const char *PYQT_MODULE = "PyQt5";

/// Define matplotlib backend that will be used to draw the canvas
constexpr static const char *MPL_QT_BACKEND = "matplotlib.backends.backend_qt5agg";

#else
#error "Unknown Qt version. Cannot determine matplotlib backend."
#endif

namespace MantidQt {
namespace Widgets {
namespace Common {

/// Import and return the backend module for this version of Qt
EXPORT_OPT_MANTIDQT_COMMON Common::Python::Object backendModule();

} // namespace Common
} // namespace Widgets
} // namespace MantidQt
