// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MPLCPP_BACKENDQT_H
#define MPLCPP_BACKENDQT_H

#include "MantidQtWidgets/Common/Python/Object.h"
#include "MantidQtWidgets/MplCpp/DllConfig.h"
#include <QtGlobal>

/*
 * Defines constants relating to the matplotlib backend
 */

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#error "Qt >= 5 required"
#elif QT_VERSION >= QT_VERSION_CHECK(5, 0, 0) &&                               \
    QT_VERSION < QT_VERSION_CHECK(6, 0, 0)

/// Define PyQt version that matches the matplotlib backend
constexpr static const char *PYQT_MODULE = "PyQt5";

/// Define matplotlib backend that will be used to draw the canvas
constexpr static const char *MPL_QT_BACKEND =
    "matplotlib.backends.backend_qt5agg";

#else
#error "Unknown Qt version. Cannot determine matplotlib backend."
#endif

using namespace MantidQt::Widgets::Common;

namespace MantidQt {
namespace Widgets {
namespace MplCpp {

/// Import and return the backend module for this version of Qt
MANTID_MPLCPP_DLL Python::Object backendModule();

} // namespace MplCpp
} // namespace Widgets
} // namespace MantidQt

#endif // MPLCPP_BACKENDQT_H
