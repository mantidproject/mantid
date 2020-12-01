// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FitScriptGeneratorDataTable.h"

#include <QAbstractItemView>
#include <QDoubleValidator>
#include <QHeaderView>
#include <QHoverEvent>
#include <QLineEdit>
#include <QStringList>
#include <QValidator>

namespace {

int DOUBLE_PRECISION(5);

QValidator *createDoubleValidator() {
  auto validator = new QDoubleValidator(-100000.0, 100000.0, DOUBLE_PRECISION);
  validator->setNotation(QDoubleValidator::Notation::StandardNotation);
  return validator;
}

QValidator *createIntValidator() { return new QIntValidator(0, 100000); }

QTableWidgetItem *createTableItem(QString const &value,
                                  Qt::AlignmentFlag const &alignment,
                                  bool editable) {
  auto item = new QTableWidgetItem(value);
  item->setTextAlignment(alignment);
  if (!editable)
    item->setFlags(item->flags() ^ Qt::ItemIsEditable);
  return item;
}

QTableWidgetItem *createIntTableItem(int value,
                                     Qt::AlignmentFlag const &alignment,
                                     bool editable) {
  return createTableItem(QString::number(value), alignment, editable);
}

QTableWidgetItem *createDoubleTableItem(double value,
                                        Qt::AlignmentFlag const &alignment,
                                        bool editable) {
  return createTableItem(QString::number(value, 'f', DOUBLE_PRECISION),
                         alignment, editable);
}

} // namespace

namespace MantidQt {
namespace MantidWidgets {

using DelegateType = CustomItemDelegate::DelegateType;

/**
 * FitScriptGeneratorDataTable class methods.
 */

FitScriptGeneratorDataTable::FitScriptGeneratorDataTable(QWidget *parent)
    : QTableWidget(parent), m_selectedRow(-1), m_selectedColumn(-1),
      m_selectedValue(0.0) {
  this->setSelectionBehavior(QAbstractItemView::SelectRows);
  this->setSelectionMode(QAbstractItemView::ExtendedSelection);
  this->setShowGrid(false);
  this->setColumnCount(4);
  this->setRowCount(0);
  this->verticalHeader()->setVisible(false);
  this->horizontalHeader()->setHighlightSections(false);
  this->horizontalHeader()->setStretchLastSection(true);

  this->setColumnWidth(ColumnIndex::WorkspaceName, 280);
  this->setColumnWidth(ColumnIndex::WorkspaceIndex, 80);
  this->setColumnWidth(ColumnIndex::StartX, 100);
  this->setColumnWidth(ColumnIndex::EndX, 100);

  this->setHorizontalHeaderLabels(
      QStringList({"Name", "WS Index", "StartX", "EndX"}));

  this->setStyleSheet("QTableWidget {\n"
                      "    font-size: 8pt;\n"
                      "    border: 1px solid #828790;\n"
                      "}\n"
                      "\n"
                      "QTableWidget::item:selected {\n"
                      "    background-color: #c7e0ff;\n"
                      "    color: #000000;\n"
                      "}"
                      "\n"
                      "QTableWidget::item:hover {\n"
                      "    background-color: #c7e0ff;\n"
                      "    color: #000000;\n"
                      "}");

  m_lastIndex = QPersistentModelIndex();
  this->viewport()->installEventFilter(this);

  this->setItemDelegateForColumn(
      ColumnIndex::WorkspaceName,
      new CustomItemDelegate(this, DelegateType::String));
  this->setItemDelegateForColumn(
      ColumnIndex::WorkspaceIndex,
      new CustomItemDelegate(this, DelegateType::Int));
  this->setItemDelegateForColumn(
      ColumnIndex::StartX, new CustomItemDelegate(this, DelegateType::Double));
  this->setItemDelegateForColumn(
      ColumnIndex::EndX, new CustomItemDelegate(this, DelegateType::Double));

  connect(this, SIGNAL(itemClicked(QTableWidgetItem *)), this,
          SLOT(handleItemClicked(QTableWidgetItem *)));
}

bool FitScriptGeneratorDataTable::eventFilter(QObject *widget, QEvent *event) {
  if (widget == this->viewport()) {
    auto index = hoveredRowIndex(event);

    if (index != m_lastIndex) {
      if (this->item(m_lastIndex.row(), m_lastIndex.column()))
        emit itemExited(index.isValid() ? index.row() : -1);
      m_lastIndex = QPersistentModelIndex(index);
    }
  }
  return QTableWidget::eventFilter(widget, event);
}

void FitScriptGeneratorDataTable::handleItemClicked(QTableWidgetItem *item) {
  m_selectedRow = item->row();
  m_selectedColumn = item->column();
  if (m_selectedColumn == ColumnIndex::StartX ||
      m_selectedColumn == ColumnIndex::EndX)
    m_selectedValue = item->text().toDouble();
}

QPersistentModelIndex
FitScriptGeneratorDataTable::hoveredRowIndex(QEvent *event) {
  auto index = m_lastIndex;
  switch (event->type()) {
  case QEvent::HoverMove:
    index = QPersistentModelIndex(
        this->indexAt(static_cast<QHoverEvent *>(event)->pos()));
    break;
  case QEvent::Leave:
    index = QPersistentModelIndex(QModelIndex());
    break;
  }
  return index;
}

std::string
FitScriptGeneratorDataTable::workspaceName(FitDomainIndex row) const {
  return getText(row, ColumnIndex::WorkspaceName).toStdString();
}

WorkspaceIndex
FitScriptGeneratorDataTable::workspaceIndex(FitDomainIndex row) const {
  return getText(row, ColumnIndex::WorkspaceIndex).toInt();
}

double FitScriptGeneratorDataTable::startX(FitDomainIndex row) const {
  return getText(row, ColumnIndex::StartX).toDouble();
}

double FitScriptGeneratorDataTable::endX(FitDomainIndex row) const {
  return getText(row, ColumnIndex::EndX).toDouble();
}

std::vector<FitDomainIndex> FitScriptGeneratorDataTable::selectedRows() const {
  std::vector<FitDomainIndex> rowIndices;

  auto const selectionModel = this->selectionModel();
  if (selectionModel->hasSelection()) {

    for (auto const &rowIndex : selectionModel->selectedRows())
      rowIndices.emplace_back(
          FitDomainIndex(static_cast<std::size_t>(rowIndex.row())));

    std::sort(rowIndices.rbegin(), rowIndices.rend());
  }
  return rowIndices;
}

void FitScriptGeneratorDataTable::removeDomain(
    std::string const &workspaceName,
    MantidWidgets::WorkspaceIndex workspaceIndex) {
  this->removeRow(indexOfDomain(workspaceName, workspaceIndex));
}

void FitScriptGeneratorDataTable::addDomain(
    QString const &workspaceName, MantidWidgets::WorkspaceIndex workspaceIndex,
    double startX, double endX) {
  this->blockSignals(true);

  auto const rowIndex = this->rowCount();
  this->insertRow(rowIndex);

  this->setItem(rowIndex, ColumnIndex::WorkspaceName,
                createTableItem(workspaceName, Qt::AlignVCenter, false));
  this->setItem(rowIndex, ColumnIndex::WorkspaceIndex,
                createIntTableItem(static_cast<int>(workspaceIndex.value),
                                   Qt::AlignCenter, false));
  this->setItem(rowIndex, ColumnIndex::StartX,
                createDoubleTableItem(startX, Qt::AlignCenter, true));
  this->setItem(rowIndex, ColumnIndex::EndX,
                createDoubleTableItem(endX, Qt::AlignCenter, true));

  this->blockSignals(false);
}

int FitScriptGeneratorDataTable::indexOfDomain(
    std::string const &workspaceName,
    MantidWidgets::WorkspaceIndex workspaceIndex) const {
  for (auto rowIndex = 0; rowIndex < this->rowCount(); ++rowIndex) {
    if (this->workspaceName(rowIndex) == workspaceName &&
        this->workspaceIndex(rowIndex) == workspaceIndex)
      return rowIndex;
  }
  return -1;
}

QString FitScriptGeneratorDataTable::getText(FitDomainIndex row,
                                             int column) const {
  return this->item(static_cast<int>(row.value), column)->text();
}

void FitScriptGeneratorDataTable::formatSelection() {
  setSelectedXValue(
      this->item(m_selectedRow, m_selectedColumn)->text().toDouble());
}

void FitScriptGeneratorDataTable::resetSelection() {
  setSelectedXValue(m_selectedValue);
}

void FitScriptGeneratorDataTable::setSelectedXValue(double xValue) {
  this->blockSignals(true);
  this->setItem(m_selectedRow, m_selectedColumn,
                createDoubleTableItem(xValue, Qt::AlignCenter, true));
  this->blockSignals(false);
}

/**
 * CustomItemDelegate class methods.
 */

CustomItemDelegate::CustomItemDelegate(FitScriptGeneratorDataTable *parent,
                                       DelegateType const &type)
    : QStyledItemDelegate(parent), m_tableWidget(parent), m_type(type) {
  m_tableWidget = parent;
  m_tableWidget->setMouseTracking(true);

  connect(m_tableWidget, SIGNAL(itemEntered(QTableWidgetItem *)), this,
          SLOT(handleItemEntered(QTableWidgetItem *)));
  connect(m_tableWidget, SIGNAL(itemExited(int)), this,
          SLOT(handleItemExited(int)));

  m_hoveredIndex = -1;
}

void CustomItemDelegate::handleItemEntered(QTableWidgetItem *item) {
  m_hoveredIndex = item->row();
  m_tableWidget->viewport()->update();
}

void CustomItemDelegate::handleItemExited(int newRowIndex) {
  m_hoveredIndex = newRowIndex;
}

QWidget *CustomItemDelegate::createEditor(QWidget *parent,
                                          QStyleOptionViewItem const &option,
                                          QModelIndex const &index) const {
  auto lineEdit = new QLineEdit(parent);

  switch (m_type) {
  case DelegateType::Double:
    lineEdit->setValidator(createDoubleValidator());
    break;
  case DelegateType::Int:
    lineEdit->setValidator(createIntValidator());
    break;
  }

  return lineEdit;
}

void CustomItemDelegate::paint(QPainter *painter,
                               QStyleOptionViewItem const &option,
                               QModelIndex const &index) const {
  auto opt = QStyleOptionViewItem(option);
  if (index.row() == m_hoveredIndex)
    opt.state |= QStyle::State_MouseOver;
  QStyledItemDelegate::paint(painter, opt, index);
}

} // namespace MantidWidgets
} // namespace MantidQt
