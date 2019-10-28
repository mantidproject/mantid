// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectDataTablePresenter.h"

#include "MantidQtWidgets/Common/SignalBlocker.h"

#include <boost/numeric/conversion/cast.hpp>

#include <QHeaderView>
#include <QItemDelegate>
#include <QLineEdit>
#include <QRegExpValidator>

namespace {
using MantidQt::CustomInterfaces::IDA::Spectra;
using MantidQt::CustomInterfaces::IDA::WorkspaceIndex;

namespace Regexes {
const QString EMPTY = "^$";
const QString SPACE = "(\\s)*";
const QString COMMA = SPACE + "," + SPACE;
const QString NATURAL_NUMBER = "(0|[1-9][0-9]*)";
const QString REAL_NUMBER = "(-?" + NATURAL_NUMBER + "(\\.[0-9]*)?)";
const QString REAL_RANGE = "(" + REAL_NUMBER + COMMA + REAL_NUMBER + ")";
const QString MASK_LIST =
    "(" + REAL_RANGE + "(" + COMMA + REAL_RANGE + ")*" + ")|" + EMPTY;
} // namespace Regexes

class ExcludeRegionDelegate : public QItemDelegate {
public:
  QWidget *createEditor(QWidget *parent,
                        const QStyleOptionViewItem & /*option*/,
                        const QModelIndex & /*index*/) const override {
    auto lineEdit = std::make_unique<QLineEdit>(parent);
    auto validator =
        std::make_unique<QRegExpValidator>(QRegExp(Regexes::MASK_LIST), parent);
    lineEdit->setValidator(validator.release());
    return lineEdit.release();
  }

  void setEditorData(QWidget *editor, const QModelIndex &index) const override {
    const auto value = index.model()->data(index, Qt::EditRole).toString();
    static_cast<QLineEdit *>(editor)->setText(value);
  }

  void setModelData(QWidget *editor, QAbstractItemModel *model,
                    const QModelIndex &index) const override {
    QLineEdit *lineEdit = static_cast<QLineEdit *>(editor);
    model->setData(index, lineEdit->text(), Qt::EditRole);
  }

  void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                            const QModelIndex & /*index*/) const override {
    editor->setGeometry(option.rect);
  }
};

QStringList defaultHeaders() {
  QStringList headers;
  headers << "Workspace"
          << "WS Index"
          << "StartX"
          << "EndX"
          << "Mask X Range";
  return headers;
}

QString makeNumber(double d) { return QString::number(d, 'g', 16); }

std::string pairsToString(
    const std::vector<std::pair<WorkspaceIndex, WorkspaceIndex>> &pairs) {
  std::vector<std::string> pairStrings;
  for (auto const &value : pairs) {
    if (value.first == value.second)
      pairStrings.emplace_back(std::to_string(value.first.value));
    else
      pairStrings.emplace_back(std::to_string(value.first.value) + "-" +
                               std::to_string(value.second.value));
  }
  return boost::algorithm::join(pairStrings, ",");
}

boost::optional<Spectra> pairsToSpectra(
    const std::vector<std::pair<WorkspaceIndex, WorkspaceIndex>> &pairs) {
  if (pairs.empty())
    return boost::none;
  else if (pairs.size() == 1)
    return Spectra(pairs[0].first, pairs[0].second);
  return Spectra(pairsToString(pairs));
}

QVariant getVariant(std::size_t i) {
  return QVariant::fromValue<qulonglong>(i);
}
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectDataTablePresenter::IndirectDataTablePresenter(
    IndirectFittingModel *model, QTableWidget *dataTable)
    : IndirectDataTablePresenter(model, dataTable, defaultHeaders()) {}

IndirectDataTablePresenter::IndirectDataTablePresenter(
    IndirectFittingModel *model, QTableWidget *dataTable,
    const QStringList &headers)
    : m_model(model), m_dataTable(dataTable) {
  setHorizontalHeaders(headers);
  m_dataTable->setItemDelegateForColumn(
      headers.size() - 1, std::make_unique<ExcludeRegionDelegate>().release());
  m_dataTable->verticalHeader()->setVisible(false);

  connect(m_dataTable, SIGNAL(cellChanged(int, int)), this,
          SLOT(setModelFittingRange(int, int)));
}

bool IndirectDataTablePresenter::tableDatasetsMatchModel() const {
  if (m_dataPositions.size() != m_model->numberOfWorkspaces())
    return false;

  for (auto i = m_dataPositions.zero(); i < m_dataPositions.size(); ++i) {
    if (m_model->getWorkspace(i)->getName() !=
        getWorkspaceName(m_dataPositions[i]))
      return false;
  }
  return true;
}

bool IndirectDataTablePresenter::isTableEmpty() const {
  return m_dataPositions.empty();
}

int IndirectDataTablePresenter::workspaceIndexColumn() const { return 1; }

int IndirectDataTablePresenter::startXColumn() const { return 2; }

int IndirectDataTablePresenter::endXColumn() const { return 3; }

int IndirectDataTablePresenter::excludeColumn() const { return 4; }

double IndirectDataTablePresenter::startX(TableRowIndex row) const {
  return getDouble(row, startXColumn());
}

double IndirectDataTablePresenter::endX(TableRowIndex row) const {
  return getDouble(row, endXColumn());
}

std::string
IndirectDataTablePresenter::getExcludeString(TableRowIndex row) const {
  return getString(row, excludeColumn());
}

std::string
IndirectDataTablePresenter::getWorkspaceName(TableRowIndex row) const {
  return getString(row, 0);
}

WorkspaceIndex
IndirectDataTablePresenter::getWorkspaceIndex(TableRowIndex row) const {
  const auto item = m_dataTable->item(row.value, workspaceIndexColumn());
  return WorkspaceIndex{static_cast<int>(item->text().toULongLong())};
}

double IndirectDataTablePresenter::getDouble(TableRowIndex row,
                                             int column) const {
  return getText(row, column).toDouble();
}

std::string IndirectDataTablePresenter::getString(TableRowIndex row,
                                                  int column) const {
  return getText(row, column).toStdString();
}

QString IndirectDataTablePresenter::getText(TableRowIndex row,
                                            int column) const {
  return m_dataTable->item(row.value, column)->text();
}

TableRowIndex
IndirectDataTablePresenter::getNextPosition(TableDatasetIndex index) const {
  if (m_dataPositions.size() > index + TableDatasetIndex{1})
    return m_dataPositions[index + TableDatasetIndex{1}];
  return TableRowIndex{m_dataTable->rowCount()};
}

TableRowIndex
IndirectDataTablePresenter::getFirstRow(TableDatasetIndex dataIndex) const {
  if (m_dataPositions.size() > dataIndex)
    return m_dataPositions[dataIndex];
  return TableRowIndex{-1};
}

TableDatasetIndex
IndirectDataTablePresenter::getDataIndex(TableRowIndex row) const {
  return TableDatasetIndex{
      m_dataTable->item(row.value, 0)->data(Qt::UserRole).toInt()};
}

boost::optional<Spectra>
IndirectDataTablePresenter::getSpectra(TableDatasetIndex dataIndex) const {
  if (m_dataPositions.size() > dataIndex)
    return getSpectra(m_dataPositions[dataIndex], getNextPosition(dataIndex));
  return boost::none;
}

boost::optional<Spectra>
IndirectDataTablePresenter::getSpectra(TableRowIndex start,
                                       TableRowIndex end) const {
  std::vector<std::pair<WorkspaceIndex, WorkspaceIndex>> spectraPairs;
  while (start < end) {
    WorkspaceIndex minimum = getWorkspaceIndex(start);
    WorkspaceIndex maximum = minimum;
    start++;
    while (start < end &&
           getWorkspaceIndex(start) == maximum + WorkspaceIndex{1}) {
      ++maximum;
      start++;
    }
    spectraPairs.emplace_back(minimum, maximum);
  }
  return pairsToSpectra(spectraPairs);
}

boost::optional<TableRowIndex>
IndirectDataTablePresenter::getRowIndex(TableDatasetIndex dataIndex,
                                        WorkspaceIndex spectrumIndex) const {
  if (!m_dataPositions.empty()) {
    const auto position = m_model->getDomainIndex(dataIndex, spectrumIndex);
    if (getNextPosition(dataIndex) > position)
      return position;
  }
  return boost::none;
}

void IndirectDataTablePresenter::setStartX(double startX,
                                           TableDatasetIndex dataIndex,
                                           WorkspaceIndex spectrumIndex) {
  if (auto const row = getRowIndex(dataIndex, spectrumIndex))
    setStartX(startX, *row);
}

void IndirectDataTablePresenter::setStartX(double startX,
                                           TableDatasetIndex dataIndex) {
  if (auto const spectra = getSpectra(dataIndex)) {
    for (auto const spectrumIndex : *spectra) {
      if (auto const row = getRowIndex(dataIndex, spectrumIndex))
        setStartX(startX, *row);
    }
  }
}

void IndirectDataTablePresenter::setStartX(double startX, TableRowIndex index) {
  MantidQt::API::SignalBlocker blocker(m_dataTable);
  m_dataTable->item(index.value, startXColumn())->setText(makeNumber(startX));
}

void IndirectDataTablePresenter::setStartX(double startX) {
  setColumnValues(startXColumn(), makeNumber(startX));
}

void IndirectDataTablePresenter::setEndX(double endX,
                                         TableDatasetIndex dataIndex,
                                         WorkspaceIndex spectrumIndex) {
  if (auto const row = getRowIndex(dataIndex, spectrumIndex))
    setEndX(endX, *row);
}

void IndirectDataTablePresenter::setEndX(double endX,
                                         TableDatasetIndex dataIndex) {
  if (auto const spectra = getSpectra(dataIndex)) {
    for (auto const spectrumIndex : *spectra) {
      if (auto const row = getRowIndex(dataIndex, spectrumIndex))
        setEndX(endX, *row);
    }
  }
}

void IndirectDataTablePresenter::setEndX(double endX, TableRowIndex index) {
  MantidQt::API::SignalBlocker blocker(m_dataTable);
  m_dataTable->item(index.value, endXColumn())->setText(makeNumber(endX));
}

void IndirectDataTablePresenter::setEndX(double endX) {
  setColumnValues(endXColumn(), makeNumber(endX));
}

void IndirectDataTablePresenter::setExclude(const std::string &exclude,
                                            TableDatasetIndex dataIndex,
                                            WorkspaceIndex spectrumIndex) {
  auto const row = getRowIndex(dataIndex, spectrumIndex);
  if (FittingMode::SEQUENTIAL == m_model->getFittingMode() || !row)
    setExcludeRegion(exclude);
  else if (row)
    setExcludeRegion(exclude, *row);
}

void IndirectDataTablePresenter::setExcludeRegion(const std::string &exclude,
                                                  TableRowIndex index) {
  MantidQt::API::SignalBlocker blocker(m_dataTable);
  if (FittingMode::SEQUENTIAL == m_model->getFittingMode())
    setExcludeRegion(exclude);
  else
    m_dataTable->item(index.value, excludeColumn())
        ->setText(QString::fromStdString(exclude));
}

void IndirectDataTablePresenter::setExcludeRegion(const std::string &exclude) {
  setExcludeRegion(QString::fromStdString(exclude));
}

void IndirectDataTablePresenter::setExcludeRegion(const QString &exclude) {
  setColumnValues(excludeColumn(), exclude);
}

void IndirectDataTablePresenter::addData(TableDatasetIndex index) {
  if (m_dataPositions.size() > index)
    updateData(index);
  else
    addNewData(index);
}

void IndirectDataTablePresenter::addNewData(TableDatasetIndex index) {
  MantidQt::API::SignalBlocker blocker(m_dataTable);
  const auto start = TableRowIndex{m_dataTable->rowCount()};

  const auto addRow = [&](WorkspaceIndex spectrum) {
    addTableEntry(index, spectrum);
  };
  m_model->applySpectra(index, addRow);

  if (m_model->numberOfWorkspaces() > m_dataPositions.size())
    m_dataPositions.emplace_back(start);
}

void IndirectDataTablePresenter::updateData(TableDatasetIndex index) {
  if (m_dataPositions.size() > index)
    updateExistingData(index);
  else
    addNewData(index);
}

void IndirectDataTablePresenter::updateExistingData(TableDatasetIndex index) {
  MantidQt::API::SignalBlocker blocker(m_dataTable);
  auto position = m_dataPositions[index];
  const auto nextPosition = getNextPosition(index);
  const auto initialSize = nextPosition - position;

  const auto updateRow = [&](WorkspaceIndex spectrum) {
    if (position < nextPosition)
      updateTableEntry(index, spectrum, position++);
    else
      addTableEntry(index, spectrum, position++);
  };
  m_model->applySpectra(index, updateRow);

  collapseData(position, nextPosition, initialSize, index);
}

void IndirectDataTablePresenter::collapseData(TableRowIndex from,
                                              TableRowIndex to,
                                              TableRowIndex initialSize,
                                              TableDatasetIndex dataIndex) {
  const auto shift = from - to;
  if (shift != TableRowIndex{0}) {
    for (auto i = from; i < to; ++i)
      removeTableEntry(from);

    if (initialSize + shift == TableRowIndex{0} &&
        m_dataPositions.size() > dataIndex) {
      m_dataPositions.remove(dataIndex);
      shiftDataPositions(shift, dataIndex, m_dataPositions.size());
      updateDataPositionsInCells(dataIndex, m_dataPositions.size());
    } else
      shiftDataPositions(shift, dataIndex + TableDatasetIndex{1},
                         m_dataPositions.size());
  }
}

void IndirectDataTablePresenter::removeSelectedData() {
  MantidQt::API::SignalBlocker blocker(m_dataTable);
  auto selectedIndices = m_dataTable->selectionModel()->selectedIndexes();
  const auto modifiedIndicesAndCount = removeTableRows(selectedIndices);
  const auto &modifiedCount = modifiedIndicesAndCount.second;
  auto &modifiedIndices = modifiedIndicesAndCount.first;

  for (auto i = 0u; i < modifiedIndices.size(); ++i)
    shiftDataPositions(-modifiedCount[i],
                       modifiedIndices[i] + TableDatasetIndex{1},
                       m_dataPositions.size());

  if (!modifiedIndices.empty()) {
    updateFromRemovedIndices(modifiedIndices);
    updateDataPositionsInCells(modifiedIndices.back() > TableDatasetIndex{0}
                                   ? modifiedIndices.back() -
                                         TableDatasetIndex{1}
                                   : TableDatasetIndex{0},
                               m_dataPositions.size());
  }
}

void IndirectDataTablePresenter::updateFromRemovedIndices(
    const std::vector<TableDatasetIndex> &indices) {
  for (const auto &index : indices) {
    const auto existingSpectra = getSpectra(index);
    if (existingSpectra)
      m_model->setSpectra(*existingSpectra, index);
    else {
      const auto originalNumberOfWorkspaces = m_model->numberOfWorkspaces();
      m_model->removeWorkspace(index);
      m_dataPositions.remove(index);

      if (m_model->numberOfWorkspaces() ==
          originalNumberOfWorkspaces - TableDatasetIndex{2})
        m_dataPositions.remove(index);
    }
  }
}

std::pair<std::vector<TableDatasetIndex>, std::vector<TableRowIndex>>
IndirectDataTablePresenter::removeTableRows(QModelIndexList &selectedRows) {
  std::vector<TableDatasetIndex> modifiedIndices;
  std::vector<TableRowIndex> modifiedCount;
  TableRowIndex previous{-1};

  qSort(selectedRows);
  for (auto i = selectedRows.count() - 1; i >= 0; --i) {
    const auto current = TableRowIndex{selectedRows[i].row()};
    if (current != previous) {
      auto modifiedIndex = removeTableEntry(current);

      if (!modifiedIndices.empty() && modifiedIndices.back() == modifiedIndex)
        ++modifiedCount.back();
      else {
        modifiedIndices.emplace_back(modifiedIndex);
        modifiedCount.emplace_back(TableRowIndex{1});
      }
      previous = current;
    }
  }
  return {modifiedIndices, modifiedCount};
}

void IndirectDataTablePresenter::setModelFittingRange(int irow, int column) {
  TableRowIndex row{irow};
  const auto workspaceIndex = getWorkspaceIndex(row);
  const auto dataIndex = getDataIndex(row);

  if (startXColumn() == column) {
    setModelStartXAndEmit(getDouble(row, column), dataIndex, workspaceIndex);
  } else if (endXColumn() == column) {
    setModelEndXAndEmit(getDouble(row, column), dataIndex, workspaceIndex);
  } else if (excludeColumn() == column) {
    setModelExcludeAndEmit(getString(row, column), dataIndex, workspaceIndex);
  }
}

void IndirectDataTablePresenter::setModelStartXAndEmit(
    double startX, TableDatasetIndex dataIndex, WorkspaceIndex workspaceIndex) {
  m_model->setStartX(startX, dataIndex, workspaceIndex);
  emit startXChanged(startX, dataIndex, workspaceIndex);
}

void IndirectDataTablePresenter::setModelEndXAndEmit(
    double endX, TableDatasetIndex dataIndex, WorkspaceIndex workspaceIndex) {
  m_model->setEndX(endX, dataIndex, workspaceIndex);
  emit endXChanged(endX, dataIndex, workspaceIndex);
}

void IndirectDataTablePresenter::setModelExcludeAndEmit(
    const std::string &exclude, TableDatasetIndex dataIndex,
    WorkspaceIndex workspaceIndex) {
  m_model->setExcludeRegion(exclude, dataIndex, workspaceIndex);
  emit excludeRegionChanged(exclude, dataIndex, workspaceIndex);
}

void IndirectDataTablePresenter::setGlobalFittingRange(bool global) {
  if (global)
    enableGlobalFittingRange();
  else
    disableGlobalFittingRange();
}

void IndirectDataTablePresenter::updateAllFittingRangeFrom(int irow,
                                                           int column) {
  TableRowIndex row{irow};
  MantidQt::API::SignalBlocker blocker(m_dataTable);
  if (startXColumn() == column)
    setStartX(getDouble(row, column));
  else if (endXColumn() == column)
    setEndX(getDouble(row, column));
  else if (excludeColumn() == column)
    setExcludeRegion(getText(row, column));
}

void IndirectDataTablePresenter::enableGlobalFittingRange() {
  MantidQt::API::SignalBlocker blocker(m_dataTable);
  const auto range =
      m_model->getFittingRange(TableDatasetIndex{0}, WorkspaceIndex{0});
  setStartX(range.first);
  setEndX(range.second);
  setExcludeRegion(
      m_model->getExcludeRegion(TableDatasetIndex{0}, WorkspaceIndex{0}));
  connect(m_dataTable, SIGNAL(cellChanged(int, int)), this,
          SLOT(updateAllFittingRangeFrom(int, int)));
}

void IndirectDataTablePresenter::disableGlobalFittingRange() {
  disconnect(m_dataTable, SIGNAL(cellChanged(int, int)), this,
             SLOT(updateAllFittingRangeFrom(int, int)));
}

void IndirectDataTablePresenter::enableTable() {
  m_dataTable->setEnabled(true);
}

void IndirectDataTablePresenter::disableTable() {
  m_dataTable->setDisabled(true);
}

void IndirectDataTablePresenter::clearTable() {
  m_dataTable->setRowCount(0);
  m_dataPositions.clear();
}

void IndirectDataTablePresenter::setColumnValues(int column,
                                                 const QString &value) {
  MantidQt::API::SignalBlocker blocker(m_dataTable);
  for (int i = 0; i < m_dataTable->rowCount(); ++i)
    m_dataTable->item(i, column)->setText(value);
}

void IndirectDataTablePresenter::setHorizontalHeaders(
    const QStringList &headers) {
  m_dataTable->setColumnCount(headers.size());
  m_dataTable->setHorizontalHeaderLabels(headers);

  auto header = m_dataTable->horizontalHeader();
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  header->setResizeMode(0, QHeaderView::Stretch);
#elif QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
  header->setSectionResizeMode(0, QHeaderView::Stretch);
#endif
}

void IndirectDataTablePresenter::addTableEntry(TableDatasetIndex dataIndex,
                                               WorkspaceIndex spectrum) {
  const auto row = TableRowIndex{m_dataTable->rowCount()};
  addTableEntry(dataIndex, spectrum, row);
  m_dataTable->item(row.value, 0)
      ->setData(Qt::UserRole, getVariant(dataIndex.value));
}

void IndirectDataTablePresenter::addTableEntry(TableDatasetIndex dataIndex,
                                               WorkspaceIndex spectrum,
                                               TableRowIndex row) {
  m_dataTable->insertRow(row.value);

  const auto &name = m_model->getWorkspace(dataIndex)->getName();
  auto cell = std::make_unique<QTableWidgetItem>(QString::fromStdString(name));
  auto flags = cell->flags();
  flags ^= Qt::ItemIsEditable;
  cell->setFlags(flags);
  setCell(std::move(cell), row, 0);

  cell = std::make_unique<QTableWidgetItem>(QString::number(spectrum.value));
  cell->setFlags(flags);
  setCell(std::move(cell), row, workspaceIndexColumn());

  const auto range = m_model->getFittingRange(dataIndex, spectrum);
  cell = std::make_unique<QTableWidgetItem>(makeNumber(range.first));
  setCell(std::move(cell), row, startXColumn());

  cell = std::make_unique<QTableWidgetItem>(makeNumber(range.second));
  setCell(std::move(cell), row, endXColumn());

  const auto exclude = m_model->getExcludeRegion(dataIndex, spectrum);
  cell = std::make_unique<QTableWidgetItem>(QString::fromStdString(exclude));
  setCell(std::move(cell), row, excludeColumn());
}

void IndirectDataTablePresenter::setCell(std::unique_ptr<QTableWidgetItem> cell,
                                         TableRowIndex row, int column) {
  m_dataTable->setItem(row.value, column, cell.release());
}

void IndirectDataTablePresenter::updateTableEntry(TableDatasetIndex dataIndex,
                                                  WorkspaceIndex spectrum,
                                                  TableRowIndex row) {
  const auto &name = m_model->getWorkspace(dataIndex)->getName();
  setCellText(QString::fromStdString(name), row, 0);
  setCellText(QString::number(spectrum.value), row, workspaceIndexColumn());

  const auto range = m_model->getFittingRange(dataIndex, spectrum);
  setCellText(makeNumber(range.first), row, startXColumn());
  setCellText(makeNumber(range.second), row, endXColumn());

  const auto exclude = m_model->getExcludeRegion(dataIndex, spectrum);
  setCellText(QString::fromStdString(exclude), row, excludeColumn());
}

void IndirectDataTablePresenter::setCellText(const QString &text,
                                             TableRowIndex row, int column) {
  m_dataTable->item(row.value, column)->setText(text);
}

TableDatasetIndex
IndirectDataTablePresenter::removeTableEntry(TableRowIndex row) {
  const auto dataIndex = m_dataTable->item(row.value, 0)->data(Qt::UserRole);
  m_dataTable->removeRow(row.value);
  return TableDatasetIndex{dataIndex.toInt()};
}

void IndirectDataTablePresenter::shiftDataPositions(TableRowIndex shift,
                                                    TableDatasetIndex from,
                                                    TableDatasetIndex to) {
  for (auto i = from; i < to; ++i)
    m_dataPositions[i] += shift;
}

void IndirectDataTablePresenter::updateDataPositionsInCells(
    TableDatasetIndex from, TableDatasetIndex to) {
  for (auto i = from; i < to; ++i) {
    const auto nextPosition = getNextPosition(i);
    for (auto row = m_dataPositions[i]; row < nextPosition; ++row)
      m_dataTable->item(row.value, 0)
          ->setData(Qt::UserRole, getVariant(i.value));
  }
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
