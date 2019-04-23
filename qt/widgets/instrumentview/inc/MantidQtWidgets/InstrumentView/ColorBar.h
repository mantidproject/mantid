// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_WIDGETS_INSTRUMENTVIEW_COLORBAR_H
#define MANTIDQT_WIDGETS_INSTRUMENTVIEW_COLORBAR_H

#include <QtGlobal>

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include "MantidQtWidgets/Plotting/Qwt/DraggableColorBarWidget.h"
#elif QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include "MantidQtWidgets/MplCpp/ColorbarWidget.h"
#endif

namespace MantidQt {
namespace MantidWidgets {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
using ColorBar = DraggableColorBarWidget;
#elif QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
using ColorBar = MantidQt::Widgets::MplCpp::ColorbarWidget;
#endif
}
} // namespace MantidQt

#endif // MANTIDQT_WIDGETS_INSTRUMENTVIEW_COLORBAR_H
