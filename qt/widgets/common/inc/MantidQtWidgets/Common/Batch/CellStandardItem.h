// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/**
See the developer documentation for Batch Widget at
developer.mantidproject.org/BatchWidget/index.html
*/
#ifndef MANTIDQTMANTIDWIDGETS_CELLSTANDARDITEM_H_
#define MANTIDQTMANTIDWIDGETS_CELLSTANDARDITEM_H_
#include "MantidQtWidgets/Common/Batch/Cell.h"
#include <QColor>
#include <QStandardItem>

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

enum CellUserRoles {
  BorderColor = Qt::UserRole + 1,
  BorderThickness,
  IconFilePath
};

void setBorderThickness(QStandardItem &item, int borderThickness);
int getBorderThickness(QStandardItem const &item);

void setBorderColor(QStandardItem &item, std::string const &borderColor,
                    int alpha);
QColor getBorderColor(QStandardItem const &item);

std::string getIconFilePath(QStandardItem const &item);
void setIcon(QStandardItem &item, std::string const &iconFilePath);
void setIconFilePath(QStandardItem &item, QString const &iconFilePath);

std::string getBackgroundColor(QStandardItem const &item);
void setBackgroundColor(QStandardItem &item,
                        std::string const &backgroundColor);

void applyCellPropertiesToItem(Cell const &cell, QStandardItem &item);
Cell extractCellPropertiesFromItem(QStandardItem const &item);
} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
#endif // MANTIDQTMANTIDWIDGETS_CELLSTANDARDITEM_H_
