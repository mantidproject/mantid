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
#include "MantidQtWidgets/LegacyQwt/MantidColorMap.h"

namespace MantidQt {
namespace MantidWidgets {
using ColorMap = MantidColorMap;
#else
#error No type defined for ColorMap for Qt >=5!
#endif

} // namespace MantidWidgets
} // namespace MantidQt

#endif // MANTIDQT_WIDGETS_INSTRUMENTVIEW_COLORMAP_H
