// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/Batch/CellStandardItem.h"
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

void applyCellPropertiesToItem(Cell const &cell, QStandardItem &item) {
  item.setText(QString::fromStdString(cell.contentText()));
  item.setEditable(cell.isEditable());
  item.setToolTip(QString::fromStdString(cell.toolTip()));
  setBorderThickness(item, cell.borderThickness());
  setBackgroundColor(item, cell.backgroundColor());
  setBorderColor(item, cell.borderColor(), cell.borderOpacity());
  setIcon(item, cell.iconFilePath());
  setForegroundColor(item, cell.foregroundColor());
}

Cell extractCellPropertiesFromItem(QStandardItem const &item) {
  auto cell = Cell(item.text().toStdString());
  cell.setBorderThickness(getBorderThickness(item));
  cell.setIconFilePath(getIconFilePath(item));
  cell.setBackgroundColor(getBackgroundColor(item));
  cell.setForegroundColor(getForegroundColor(item));

  auto borderColor = getBorderColor(item);
  cell.setBorderColor(borderColor.name().toStdString());
  cell.setBorderOpacity(borderColor.alpha());

  cell.setEditable(item.isEditable());
  cell.setToolTip(item.toolTip().toStdString());
  return cell;
}

void setBorderThickness(QStandardItem &item, int borderThickness) {
  item.setData(borderThickness, CellUserRoles::BorderThickness);
}

int getBorderThickness(QStandardItem const &item) {
  return item.data(CellUserRoles::BorderThickness).toInt();
}

std::string getIconFilePath(QStandardItem const &item) {
  return item.data(CellUserRoles::IconFilePath).toString().toStdString();
}

void setIconFilePath(QStandardItem &item, QString const &iconFilePath) {
  item.setData(iconFilePath, CellUserRoles::IconFilePath);
}

void setIcon(QStandardItem &item, std::string const &iconFilePath) {
  auto qiconFilePath = QString::fromStdString(iconFilePath);
  setIconFilePath(item, qiconFilePath);
  if (!qiconFilePath.isEmpty())
    item.setIcon(QIcon(qiconFilePath));
  else
    item.setIcon(QIcon());
}

void setBorderColor(QStandardItem &item, std::string const &borderColor,
                    int alpha) {
  auto borderQColor = QColor(borderColor.c_str());
  borderQColor.setAlpha(alpha);
  item.setData(borderQColor, CellUserRoles::BorderColor);
}

void setBackgroundColor(QStandardItem &item,
                        std::string const &backgroundColor) {
  auto borderColor = QColor(backgroundColor.c_str());
  item.setData(QBrush(borderColor), Qt::BackgroundRole);
}

std::string getBackgroundColor(QStandardItem const &item) {
  return item.data(Qt::BackgroundRole)
      .value<QBrush>()
      .color()
      .name()
      .toStdString();
}

void setForegroundColor(QStandardItem &item,
                        std::string const &foregroundColor) {
  auto borderColor = QColor(foregroundColor.c_str());
  item.setData(QBrush(borderColor), Qt::ForegroundRole);
}

std::string getForegroundColor(QStandardItem const &item) {
  return item.data(Qt::ForegroundRole)
      .value<QBrush>()
      .color()
      .name()
      .toStdString();
}

QColor getBorderColor(QStandardItem const &item) {
  return item.data(CellUserRoles::BorderColor).value<QColor>();
}
} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
