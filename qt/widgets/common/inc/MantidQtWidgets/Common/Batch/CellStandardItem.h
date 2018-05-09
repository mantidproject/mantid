#ifndef MANTIDQTMANTIDWIDGETS_CELLSTANDARDITEM_H_
#define MANTIDQTMANTIDWIDGETS_CELLSTANDARDITEM_H_
#include <QColor>
#include <QStandardItem>
#include "MantidQtWidgets/Common/Batch/Cell.h"

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
}
}
}
#endif // MANTIDQTMANTIDWIDGETS_CELLSTANDARDITEM_H_
