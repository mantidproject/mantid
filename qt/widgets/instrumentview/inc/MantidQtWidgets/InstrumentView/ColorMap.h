// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_WIDGETS_INSTRUMENTVIEW_COLORMAP_H
#define MANTIDQT_WIDGETS_INSTRUMENTVIEW_COLORMAP_H

#include <QtGlobal>

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include "MantidQtWidgets/Plotting/Qwt/MantidColorMap.h"
#elif QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include "MantidQtWidgets/MplCpp/MantidColorMap.h"
#endif

namespace MantidQt {
namespace MantidWidgets {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
using ColorMap = MantidColorMap;
#elif QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
using ColorMap = MantidQt::Widgets::MplCpp::MantidColorMap;
#endif
}
} // namespace MantidQt

#endif // MANTIDQT_WIDGETS_INSTRUMENTVIEW_COLORMAP_H
