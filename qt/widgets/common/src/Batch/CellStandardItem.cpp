#include "MantidQtWidgets/Common/Batch/CellStandardItem.h"
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

void setBorderThickness(QStandardItem &item, int borderThickness) {
  item.setData(borderThickness, CellUserRoles::BorderThickness);
}

int getBorderThickness(QStandardItem const &item) {
  return item.data(CellUserRoles::BorderThickness).toInt();
}

void setBorderColor(QStandardItem &item, QColor const& borderColor) {
  item.setData(borderColor, CellUserRoles::BorderColor);
}

QColor getBorderColor(QStandardItem const &item) {
  return item.data(CellUserRoles::BorderColor).value<QColor>();
}

void applyCellPropertiesToItem(Cell const &cell, QStandardItem &item) {
  item.setText(QString::fromStdString(cell.contentText()));
  item.setEditable(cell.isEditable());
  auto borderColor = QColor(cell.borderColor().c_str());
  borderColor.setAlpha(cell.borderTransparency());
  item.setData(borderColor, CellUserRoles::BorderColor);
  item.setData(cell.borderThickness(), CellUserRoles::BorderThickness);
}

Cell extractCellPropertiesFromItem(QStandardItem const& item) {
  auto cell = Cell(item.text().toStdString());
  cell.setBorderThickness(getBorderThickness(item));

  auto borderColor = getBorderColor(item);
  cell.setBorderColor(borderColor.name().toStdString());
  cell.setBorderTransparency(borderColor.alpha());

  cell.setEditable(cell.isEditable());
  return cell;
}

}
}
}
