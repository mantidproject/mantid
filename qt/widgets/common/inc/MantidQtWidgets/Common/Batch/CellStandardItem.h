// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/**
See the developer documentation for Batch Widget at
developer.mantidproject.org/BatchWidget/index.html
*/
#pragma once
#include "MantidQtWidgets/Common/Batch/Cell.h"
#include <QColor>
#include <QStandardItem>

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

enum CellUserRoles { BorderColor = Qt::UserRole + 1, BorderThickness, IconFilePath };

void setBorderThickness(QStandardItem &item, int borderThickness);
int getBorderThickness(QStandardItem const &item);

void setBorderColor(QStandardItem &item, std::string const &borderColor, int alpha);
QColor getBorderColor(QStandardItem const &item);

std::string getIconFilePath(QStandardItem const &item);
void setIcon(QStandardItem &item, std::string const &iconFilePath);
void setIconFilePath(QStandardItem &item, QString const &iconFilePath);

std::string getBackgroundColor(QStandardItem const &item);
void setBackgroundColor(QStandardItem &item, std::string const &backgroundColor);
std::string getForegroundColor(QStandardItem const &item);
void setForegroundColor(QStandardItem &item, std::string const &foregroundColor);

void applyCellPropertiesToItem(Cell const &cell, QStandardItem &item);
Cell extractCellPropertiesFromItem(QStandardItem const &item);
} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
