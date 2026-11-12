// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <QGuiApplication>
#include <QStyleHints>

namespace MantidQt {
namespace MantidWidgets {

inline const QColor DARK_CANVAS_COLOUR("#1d1d1d");
inline bool isDarkMode() { return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark; }
inline QColor defaultLineColour() { return isDarkMode() ? QColor(Qt::white) : QColor(Qt::black); }
inline QColor canvasColour(const QColor &light = Qt::white) { return isDarkMode() ? DARK_CANVAS_COLOUR : light; }

} // namespace MantidWidgets
} // namespace MantidQt
