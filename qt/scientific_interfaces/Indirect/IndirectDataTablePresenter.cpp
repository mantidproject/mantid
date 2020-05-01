// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
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
    auto *lineEdit = static_cast<QLineEdit *>(editor);
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

class ScopedFalse {
  bool &m_ref;
  bool m_oldValue;

public:
  // this sets the input bool to false whilst this object is in scope and then
  // resets it to its old value when this object drops out of scope.
  explicit ScopedFalse(bool &variable) : m_ref(variable), m_oldValue(variable) {
    m_ref = false;
  }
  ~ScopedFalse() { m_ref = m_oldValue; }
};
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectDataTablePresenter::IndirectDataTablePresenter(IIndirectFitData *model,
                                                       QTableWidget *dataTable)
    : IndirectDataTablePresenter(model, dataTable, defaultHeaders()) {}

IndirectDataTablePresenter::IndirectDataTablePresenter(
    IIndirectFitData *model, QTableWidget *dataTable,
    const QStringList &headers)
    : m_model(model), m_dataTable(dataTable) {
  setHorizontalHeaders(headers);
  m_dataTable->setItemDelegateForColumn(
      headers.size() - 1, std::make_unique<ExcludeRegionDelegate>().release());
  m_dataTable->verticalHeader()->setVisible(false);

  connect(m_dataTable, SIGNAL(cellChanged(int, int)), this,
          SLOT(handleCellChanged(int, int)));
}

bool IndirectDataTablePresenter::isTableEmpty() const {
  return m_dataPositions.empty();
}

int IndirectDataTablePresenter::workspaceIndexColumn() const { return 1; }

int IndirectDataTablePresenter::startXColumn() const { return 2; }

int IndirectDataTablePresenter::endXColumn() const { return 3; }

int IndirectDataTablePresenter::excludeColumn() const { return 4; }

double IndirectDataTablePresenter::startX(FitDomainIndex row) const {
  return getDouble(row, startXColumn());
}

double IndirectDataTablePresenter::endX(FitDomainIndex row) const {
  return getDouble(row, endXColumn());
}

std::string
IndirectDataTablePresenter::getExcludeString(FitDomainIndex row) const {
  return getString(row, excludeColumn());
}

std::string
IndirectDataTablePresenter::getWorkspaceName(FitDomainIndex row) const {
  return getString(row, 0);
}

WorkspaceIndex
IndirectDataTablePresenter::getWorkspaceIndex(FitDomainIndex row) const {
  const auto item = m_dataTable->item(row.value, workspaceIndexColumn());
  return WorkspaceIndex{static_cast<int>(item->text().toULongLong())};
}

double IndirectDataTablePresenter::getDouble(FitDomainIndex row,
                                             int column) const {
  return getText(row, column).toDouble();
}

std::string IndirectDataTablePresenter::getString(FitDomainIndex row,
                                                  int column) const {
  return getText(row, column).toStdString();
}

QString IndirectDataTablePresenter::getText(FitDomainIndex row,
                                            int column) const {
  return m_dataTable->item(row.value, column)->text();
}

FitDomainIndex
IndirectDataTablePresenter::getNextPosition(TableDatasetIndex index) const {
  if (m_dataPositions.size() > index + TableDatasetIndex{1})
    return m_dataPositions[index + TableDatasetIndex{1}];
  return FitDomainIndex{m_dataTable->rowCount()};
}

FitDomainIndex
IndirectDataTablePresenter::getFirstRow(TableDatasetIndex dataIndex) const {
  if (m_dataPositions.size() > dataIndex)
    return m_dataPositions[dataIndex];
  return FitDomainIndex{-1};
}

TableDatasetIndex
IndirectDataTablePresenter::getDataIndex(FitDomainIndex row) const {
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
IndirectDataTablePresenter::getSpectra(FitDomainIndex start,
                                       FitDomainIndex end) const {
  std::vector<std::pair<WorkspaceIndex, WorkspaceIndex>> spectraPairs;
  while (start < end) {
    WorkspaceIndex minimum = getWorkspaceIndex(start);
    WorkspaceIndex maximum = minimum;
    ++start;
    while (start < end &&
           getWorkspaceIndex(start) == maximum + WorkspaceIndex{1}) {
      ++maximum;
      ++start;
    }
    spectraPairs.emplace_back(minimum, maximum);
  }
  return pairsToSpectra(spectraPairs);
}

boost::optional<FitDomainIndex>
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

void IndirectDataTablePresenter::setStartX(double startX,
                                           FitDomainIndex index) {
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

void IndirectDataTablePresenter::setEndX(double endX, FitDomainIndex index) {
  MantidQt::API::SignalBlocker blocker(m_dataTable);
  m_dataTable->item(index.value, endXColumn())->setText(makeNumber(endX));
}

void IndirectDataTablePresenter::setEndX(double endX) {
  setColumnValues(endXColumn(), makeNumber(endX));
}

void IndirectDataTablePresenter::setExclude(const std::string &exclude,
                                            TableDatasetIndex dataIndex,
                                            WorkspaceIndex spectrumIndex) {
  // auto const row = getRowIndex(dataIndex, spectrumIndex);
  // if (FittingMode::SEQUENTIAL == m_model->getFittingMode() || !row)
  //   setExcludeRegion(exclude);
  // else if (row)
  //   setExcludeRegion(exclude, *row);
}

void IndirectDataTablePresenter::setExcludeRegion(const std::string &exclude,
                                                  FitDomainIndex index) {
  // MantidQt::API::SignalBlocker blocker(m_dataTable);
  // if (FittingMode::SEQUENTIAL == m_model->getFittingMode())
  //   setExcludeRegion(exclude);
  // else
  //   m_dataTable->item(index.value, excludeColumn())
  //       ->setText(QString::fromStdString(exclude));
}

void IndirectDataTablePresenter::setExcludeRegion(const std::string &exclude) {
  setExcludeRegion(QString::fromStdString(exclude));
}

void IndirectDataTablePresenter::setExcludeRegion(const QString &exclude) {
  setColumnValues(excludeColumn(), exclude);
}

void IndirectDataTablePresenter::removeSelectedData() {
  auto selectedIndices = m_dataTable->selectionModel()->selectedIndexes();

  for (auto item : selectedIndices) {
    m_model->removeDataByIndex(FitDomainIndex(item.row()));
  }

  updateTableFromModel(m_model);
}

void IndirectDataTablePresenter::updateTableFromModel(IIndirectFitData *model) {
  ScopedFalse _signalBlock(m_emitCellChanged);
  m_dataTable->setRowCount(0);

  for (auto domainIndex = FitDomainIndex{0};
       domainIndex < model->getNumberOfDomains(); domainIndex++) {
    addTableEntry(model, domainIndex);
  }
};

void IndirectDataTablePresenter::handleCellChanged(int irow, int column) {
  if (!m_emitCellChanged) {
    return;
  }
  FitDomainIndex row{irow};
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

void IndirectDataTablePresenter::addTableEntry(IIndirectFitData *model,
                                               FitDomainIndex row) {
  m_dataTable->insertRow(row.value);
  const auto &name = m_model->getWorkspace(row)->getName();
  auto cell = std::make_unique<QTableWidgetItem>(QString::fromStdString(name));
  auto flags = cell->flags();
  flags ^= Qt::ItemIsEditable;
  cell->setFlags(flags);
  setCell(std::move(cell), row.value, 0);

  cell = std::make_unique<QTableWidgetItem>(
      QString::number(m_model->getSpectrum(row)));
  cell->setFlags(flags);
  setCell(std::move(cell), row.value, workspaceIndexColumn());

  const auto range = m_model->getFittingRange(row);
  cell = std::make_unique<QTableWidgetItem>(makeNumber(range.first));
  setCell(std::move(cell), row.value, startXColumn());

  cell = std::make_unique<QTableWidgetItem>(makeNumber(range.second));
  setCell(std::move(cell), row.value, endXColumn());

  const auto exclude = m_model->getExcludeRegion(row);
  cell = std::make_unique<QTableWidgetItem>(QString::fromStdString(exclude));
  setCell(std::move(cell), row.value, excludeColumn());
}

void IndirectDataTablePresenter::setCell(std::unique_ptr<QTableWidgetItem> cell,
                                         FitDomainIndex row, int column) {
  m_dataTable->setItem(row.value, column, cell.release());
}

void IndirectDataTablePresenter::updateTableEntry(TableDatasetIndex dataIndex,
                                                  WorkspaceIndex spectrum,
                                                  FitDomainIndex row) {
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
                                             FitDomainIndex row, int column) {
  m_dataTable->item(row.value, column)->setText(text);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
