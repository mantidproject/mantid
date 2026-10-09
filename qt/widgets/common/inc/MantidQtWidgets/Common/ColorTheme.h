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

inline bool isDarkMode() { return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark; }

} // namespace MantidWidgets
} // namespace MantidQt
