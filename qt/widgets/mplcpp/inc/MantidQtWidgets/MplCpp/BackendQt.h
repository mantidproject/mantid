// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/Python/Object.h"
#include "MantidQtWidgets/MplCpp/DllConfig.h"
#include <QtGlobal>

/*
 * Defines constants relating to the matplotlib backend
 */

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)

/// Define PyQt version that matches the matplotlib backend
constexpr static const char *PYQT_MODULE = "PyQt5";

/// Define matplotlib backend that will be used to draw the canvas
constexpr static const char *MPL_QT_BACKEND = "matplotlib.backends.backend_qt5agg";

#else
#error "Unknown Qt version. Cannot determine matplotlib backend."
#endif

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/// Import and return the backend module for this version of Qt
MANTID_MPLCPP_DLL Common::Python::Object backendModule();

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt
