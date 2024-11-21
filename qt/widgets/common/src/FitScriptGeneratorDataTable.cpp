// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/FitScriptGeneratorDataTable.h"

#include <QAbstractItemView>
#include <QColor>
#include <QDoubleValidator>
#include <QHeaderView>
#include <QHoverEvent>
#include <QItemSelectionModel>
#include <QLineEdit>
#include <QStringList>
#include <QValidator>

#include <algorithm>

namespace {

int WS_INDEX_MIN(0);
int WS_INDEX_MAX(100000);
double X_EXTENT(100000.0);
int X_PRECISION(5);

QStringList const COLUMN_HEADINGS({"Name", "WS Index", "StartX", "EndX"});
QColor const FUNCTION_INDEX_COLOR(QColor(30, 144, 255));
QString const TABLE_STYLESHEET("QTableWidget {\n"
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
                               "}");

QValidator *createXValidator() {
  auto validator = new QDoubleValidator(-X_EXTENT, X_EXTENT, X_PRECISION);
  validator->setNotation(QDoubleValidator::Notation::StandardNotation);
  return validator;
}

QValidator *createWSIndexValidator() { return new QIntValidator(WS_INDEX_MIN, WS_INDEX_MAX); }

QTableWidgetItem *createTableItem(QString const &value, Qt::AlignmentFlag const &alignment, bool editable,
                                  QColor const &color = QColor(0, 0, 0)) {
  auto item = new QTableWidgetItem(value);
  item->setData(Qt::ForegroundRole, color);
  item->setTextAlignment(alignment);
  if (!editable)
    item->setFlags(item->flags() ^ Qt::ItemIsEditable);
  return item;
}

QTableWidgetItem *createWSIndexTableItem(int value, Qt::AlignmentFlag const &alignment, bool editable) {
  return createTableItem(QString::number(value), alignment, editable);
}

QTableWidgetItem *createXTableItem(double value, Qt::AlignmentFlag const &alignment, bool editable) {
  return createTableItem(QString::number(value, 'f', X_PRECISION), alignment, editable);
}

QString toFunctionIndex(MantidQt::MantidWidgets::FitDomainIndex index) {
  return "f" + QString::number(index.value) + ".";
}

} // namespace

namespace MantidQt::MantidWidgets {

/**
 * FitScriptGeneratorDataTable class methods.
 */

FitScriptGeneratorDataTable::FitScriptGeneratorDataTable(QWidget *parent)
    : QTableWidget(parent), m_selectedRows(), m_selectedColumn(-1), m_selectedValue(0.0),
      m_lastHoveredIndex(QPersistentModelIndex()) {
  this->setSelectionBehavior(QAbstractItemView::SelectRows);
  this->setSelectionMode(QAbstractItemView::ExtendedSelection);
  this->setShowGrid(false);
  this->setColumnCount(COLUMN_HEADINGS.size());
  this->setRowCount(0);
  this->horizontalHeader()->setHighlightSections(false);
  this->horizontalHeader()->setStretchLastSection(true);

  this->setColumnWidth(ColumnIndex::WorkspaceName, 280);
  this->setColumnWidth(ColumnIndex::WorkspaceIndex, 80);
  this->setColumnWidth(ColumnIndex::StartX, 100);
  this->setColumnWidth(ColumnIndex::EndX, 100);

  this->setHorizontalHeaderLabels(COLUMN_HEADINGS);

  this->setStyleSheet(TABLE_STYLESHEET);

  this->viewport()->installEventFilter(this);

  this->setItemDelegateForColumn(ColumnIndex::WorkspaceName, new CustomItemDelegate(this, ColumnIndex::WorkspaceName));
  this->setItemDelegateForColumn(ColumnIndex::WorkspaceIndex,
                                 new CustomItemDelegate(this, ColumnIndex::WorkspaceIndex));
  this->setItemDelegateForColumn(ColumnIndex::StartX, new CustomItemDelegate(this, ColumnIndex::StartX));
  this->setItemDelegateForColumn(ColumnIndex::EndX, new CustomItemDelegate(this, ColumnIndex::EndX));

  connect(this, SIGNAL(itemClicked(QTableWidgetItem *)), this, SLOT(handleItemClicked(QTableWidgetItem *)));
  connect(this, SIGNAL(itemSelectionChanged()), this, SLOT(handleItemSelectionChanged()));
  disconnect(this->verticalHeader(), SIGNAL(sectionPressed(int)), this, SLOT(selectRow(int)));
}

bool FitScriptGeneratorDataTable::eventFilter(QObject *widget, QEvent *event) {
  if (widget == this->viewport()) {
    auto index = hoveredRowIndex(event);

    if (index != m_lastHoveredIndex) {
      if (this->item(m_lastHoveredIndex.row(), m_lastHoveredIndex.column()))
        emit itemExited(index.isValid() ? index.row() : -1);
      m_lastHoveredIndex = QPersistentModelIndex(index);
    }
  }
  return QTableWidget::eventFilter(widget, event);
}

void FitScriptGeneratorDataTable::handleItemClicked(QTableWidgetItem *item) {
  m_selectedRows = selectedRows();
  m_selectedColumn = item->column();
  if (m_selectedColumn == ColumnIndex::StartX || m_selectedColumn == ColumnIndex::EndX)
    m_selectedValue = item->text().toDouble();
}

void FitScriptGeneratorDataTable::handleItemSelectionChanged() {
  auto *selectionModel = this->selectionModel();

  if (!selectionModel->hasSelection()) {
    this->blockSignals(true);

    // Makes sure that multi-selection rows are stored within the selectionModel
    // as should be expected. This prevents a bug where not all selected rows
    // were being stored in the selection model.
    auto itemSelection = selectionModel->selection();
    for (auto const &selectedRow : m_selectedRows) {
      this->selectRow(static_cast<int>(selectedRow.value));
      itemSelection.merge(selectionModel->selection(), QItemSelectionModel::Select);
    }
    selectionModel->clearSelection();
    selectionModel->select(itemSelection, QItemSelectionModel::Select);

    this->blockSignals(false);
  } else {
    m_selectedRows = selectedRows();
  }
}

QPersistentModelIndex FitScriptGeneratorDataTable::hoveredRowIndex(QEvent *event) {
  auto index = m_lastHoveredIndex;
  auto const eventType = event->type();
  if (eventType == QEvent::HoverMove)
    index = QPersistentModelIndex(this->indexAt(static_cast<QHoverEvent *>(event)->pos()));
  else if (eventType == QEvent::Leave)
    index = QPersistentModelIndex(QModelIndex());

  return index;
}

std::string FitScriptGeneratorDataTable::workspaceName(FitDomainIndex row) const {
  return getText(row, ColumnIndex::WorkspaceName).toStdString();
}

MantidWidgets::WorkspaceIndex FitScriptGeneratorDataTable::workspaceIndex(FitDomainIndex row) const {
  return getText(row, ColumnIndex::WorkspaceIndex).toInt();
}

double FitScriptGeneratorDataTable::startX(FitDomainIndex row) const {
  return getText(row, ColumnIndex::StartX).toDouble();
}

double FitScriptGeneratorDataTable::endX(FitDomainIndex row) const {
  return getText(row, ColumnIndex::EndX).toDouble();
}

std::vector<FitDomainIndex> FitScriptGeneratorDataTable::allRows() const {
  std::vector<FitDomainIndex> rowIndices;
  rowIndices.reserve(this->rowCount());
  for (auto index = 0; index < this->rowCount(); ++index)
    rowIndices.emplace_back(FitDomainIndex(index));

  std::reverse(rowIndices.begin(), rowIndices.end());
  return rowIndices;
}

std::vector<FitDomainIndex> FitScriptGeneratorDataTable::selectedRows() const {
  std::vector<FitDomainIndex> rowIndices;

  auto const selectionModel = this->selectionModel();
  if (selectionModel->hasSelection()) {
    const auto rows = selectionModel->selectedRows();
    std::transform(rows.cbegin(), rows.cend(), std::back_inserter(rowIndices),
                   [](const auto &rowIndex) { return FitDomainIndex(static_cast<std::size_t>(rowIndex.row())); });
    std::reverse(rowIndices.begin(), rowIndices.end());
  }
  return rowIndices;
}

FitDomainIndex FitScriptGeneratorDataTable::currentRow() const {
  if (hasLoadedData()) {
    auto const rows = selectedRows();
    return rows.size() > 0 ? rows[0] : FitDomainIndex{0};
  }

  throw std::runtime_error("There is no currentRow as data has not been loaded yet.");
}

bool FitScriptGeneratorDataTable::hasLoadedData() const { return this->rowCount() > 0; }

QString FitScriptGeneratorDataTable::selectedDomainFunctionPrefix() const {
  auto const rows = selectedRows();
  if (rows.empty())
    return "";
  return this->verticalHeaderItem(static_cast<int>(rows[0].value))->text();
}

void FitScriptGeneratorDataTable::renameWorkspace(QString const &workspaceName, QString const &newName) {
  for (auto rowIndex = 0; rowIndex < this->rowCount(); ++rowIndex) {
    auto tableItem = this->item(rowIndex, ColumnIndex::WorkspaceName);
    if (tableItem->text() == workspaceName)
      tableItem->setText(newName);
  }
}

void FitScriptGeneratorDataTable::removeDomain(FitDomainIndex domainIndex) {
  this->removeRow(static_cast<int>(domainIndex.value));
  updateVerticalHeaders();

  m_selectedRows = selectedRows();

  if (m_selectedRows.empty() && this->rowCount() > 0)
    this->selectRow(0);

  m_selectedRows = selectedRows();
}

void FitScriptGeneratorDataTable::addDomain(QString const &workspaceName, MantidWidgets::WorkspaceIndex workspaceIndex,
                                            double startX, double endX) {
  this->blockSignals(true);

  auto const rowIndex = this->rowCount();
  this->insertRow(rowIndex);

  this->setVerticalHeaderItem(rowIndex, createTableItem(toFunctionIndex(FitDomainIndex(rowIndex)), Qt::AlignCenter,
                                                        false, FUNCTION_INDEX_COLOR));
  this->setItem(rowIndex, ColumnIndex::WorkspaceName, createTableItem(workspaceName, Qt::AlignVCenter, false));
  this->setItem(rowIndex, ColumnIndex::WorkspaceIndex,
                createWSIndexTableItem(static_cast<int>(workspaceIndex.value), Qt::AlignCenter, false));
  this->setItem(rowIndex, ColumnIndex::StartX, createXTableItem(startX, Qt::AlignCenter, true));
  this->setItem(rowIndex, ColumnIndex::EndX, createXTableItem(endX, Qt::AlignCenter, true));

  if (!this->selectionModel()->hasSelection()) {
    m_selectedRows.emplace_back(rowIndex);
    this->selectRow(rowIndex);
  }

  this->blockSignals(false);
}

void FitScriptGeneratorDataTable::updateVerticalHeaders() {
  for (auto i = FitDomainIndex(0); i < FitDomainIndex(this->rowCount()); ++i)
    this->setVerticalHeaderItem(static_cast<int>(i.value),
                                createTableItem(toFunctionIndex(i), Qt::AlignCenter, false, FUNCTION_INDEX_COLOR));
}

QString FitScriptGeneratorDataTable::getText(FitDomainIndex row, int column) const {
  return this->item(static_cast<int>(row.value), column)->text();
}

void FitScriptGeneratorDataTable::formatSelection() {
  if (!m_selectedRows.empty() && m_selectedColumn >= 0)
    setSelectedXValue(this->item(static_cast<int>(m_selectedRows[0].value), m_selectedColumn)->text().toDouble());
}

void FitScriptGeneratorDataTable::resetSelection() { setSelectedXValue(m_selectedValue); }

void FitScriptGeneratorDataTable::setFunctionPrefixVisible(bool visible) {
  this->verticalHeader()->setVisible(visible);
}

void FitScriptGeneratorDataTable::setSelectedXValue(double xValue) {
  this->blockSignals(true);
  if (!m_selectedRows.empty())
    this->setItem(static_cast<int>(m_selectedRows[0].value), m_selectedColumn,
                  createXTableItem(xValue, Qt::AlignCenter, true));
  this->blockSignals(false);
}

/**
 * CustomItemDelegate class methods.
 */

CustomItemDelegate::CustomItemDelegate(FitScriptGeneratorDataTable *parent, ColumnIndex const &index)
    : QStyledItemDelegate(parent), m_tableWidget(parent), m_columnIndex(index), m_hoveredIndex(-1) {
  m_tableWidget = parent;
  m_tableWidget->setMouseTracking(true);

  connect(m_tableWidget, SIGNAL(itemEntered(QTableWidgetItem *)), this, SLOT(handleItemEntered(QTableWidgetItem *)));
  connect(m_tableWidget, SIGNAL(itemExited(int)), this, SLOT(handleItemExited(int)));
}

void CustomItemDelegate::handleItemEntered(QTableWidgetItem *item) {
  m_hoveredIndex = item->row();
  m_tableWidget->viewport()->update();
}

void CustomItemDelegate::handleItemExited(int newRowIndex) { m_hoveredIndex = newRowIndex; }

QWidget *CustomItemDelegate::createEditor(QWidget *parent, QStyleOptionViewItem const &option,
                                          QModelIndex const &index) const {
  Q_UNUSED(option);
  Q_UNUSED(index);

  auto lineEdit = new QLineEdit(parent);
  switch (m_columnIndex) {
  case ColumnIndex::WorkspaceName:
    break;
  case ColumnIndex::WorkspaceIndex:
    lineEdit->setValidator(createWSIndexValidator());
    break;
  case ColumnIndex::StartX:
    lineEdit->setValidator(createXValidator());
    break;
  case ColumnIndex::EndX:
    lineEdit->setValidator(createXValidator());
    break;
  }

  return lineEdit;
}

void CustomItemDelegate::paint(QPainter *painter, QStyleOptionViewItem const &option, QModelIndex const &index) const {
  auto opt = QStyleOptionViewItem(option);
  if (index.row() == m_hoveredIndex)
    opt.state |= QStyle::State_MouseOver;
  QStyledItemDelegate::paint(painter, opt, index);
}

} // namespace MantidQt::MantidWidgets
