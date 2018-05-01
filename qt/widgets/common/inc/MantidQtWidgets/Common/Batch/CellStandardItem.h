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
  BorderThickness
};

void setBorderThickness(QStandardItem &item, int borderThickness);
int getBorderThickness(QStandardItem const &item);

void setBorderColor(QStandardItem &item, QColor const& borderColor);
QColor getBorderColor(QStandardItem const &item);

void applyCellPropertiesToItem(Cell const &cell, QStandardItem &item);
Cell extractCellPropertiesFromItem(QStandardItem const& item);

}
}
}
#endif // MANTIDQTMANTIDWIDGETS_CELLSTANDARDITEM_H_
