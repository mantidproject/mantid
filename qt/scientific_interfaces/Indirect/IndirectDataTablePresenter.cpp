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

namespace Regexes {
const QString EMPTY = "^$";
const QString SPACE = "(\\s)*";
const QString COMMA = SPACE + "," + SPACE;
const QString NATURAL_NUMBER = "(0|[1-9][0-9]*)";
const QString REAL_NUMBER = "(-?" + NATURAL_NUMBER + "(\\.[0-9]*)?)";
const QString REAL_RANGE = "(" + REAL_NUMBER + COMMA + REAL_NUMBER + ")";
const QString MASK_LIST = "(" + REAL_RANGE + "(" + COMMA + REAL_RANGE + ")*" + ")|" + EMPTY;
} // namespace Regexes

class ExcludeRegionDelegate : public QItemDelegate {
public:
  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem & /*option*/,
                        const QModelIndex & /*index*/) const override {
    auto lineEdit = std::make_unique<QLineEdit>(parent);
    auto validator = std::make_unique<QRegExpValidator>(QRegExp(Regexes::MASK_LIST), parent);
    lineEdit->setValidator(validator.release());
    return lineEdit.release();
  }

  void setEditorData(QWidget *editor, const QModelIndex &index) const override {
    const auto value = index.model()->data(index, Qt::EditRole).toString();
    static_cast<QLineEdit *>(editor)->setText(value);
  }

  void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override {
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

class ScopedFalse {
  bool &m_ref;
  bool m_oldValue;

public:
  // this sets the input bool to false whilst this object is in scope and then
  // resets it to its old value when this object drops out of scope.
  explicit ScopedFalse(bool &variable) : m_ref(variable), m_oldValue(variable) { m_ref = false; }
  ~ScopedFalse() { m_ref = m_oldValue; }
};
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectDataTablePresenter::IndirectDataTablePresenter(IIndirectFitDataModel *model, QTableWidget *dataTable)
    : IndirectDataTablePresenter(model, dataTable, defaultHeaders()) {}

IndirectDataTablePresenter::IndirectDataTablePresenter(IIndirectFitDataModel *model, QTableWidget *dataTable,
                                                       const QStringList &headers)
    : m_model(model), m_dataTable(dataTable) {

  setHorizontalHeaders(headers);
  m_dataTable->setItemDelegateForColumn(headers.size() - 1, std::make_unique<ExcludeRegionDelegate>().release());
  m_dataTable->verticalHeader()->setVisible(false);

  connect(m_dataTable, SIGNAL(cellChanged(int, int)), this, SLOT(handleCellChanged(int, int)));
}

bool IndirectDataTablePresenter::isTableEmpty() const { return m_dataTable->rowCount() == 0; }

int IndirectDataTablePresenter::workspaceIndexColumn() const { return 1; }

int IndirectDataTablePresenter::startXColumn() const { return 2; }

int IndirectDataTablePresenter::endXColumn() const { return 3; }

int IndirectDataTablePresenter::excludeColumn() const { return 4; }

double IndirectDataTablePresenter::getDouble(FitDomainIndex row, int column) const {
  return getText(row, column).toDouble();
}

std::string IndirectDataTablePresenter::getString(FitDomainIndex row, int column) const {
  return getText(row, column).toStdString();
}

QString IndirectDataTablePresenter::getText(FitDomainIndex row, int column) const {
  return m_dataTable->item(static_cast<int>(row.value), column)->text();
}

void IndirectDataTablePresenter::removeSelectedData() {
  auto selectedIndices = m_dataTable->selectionModel()->selectedIndexes();
  std::sort(selectedIndices.begin(), selectedIndices.end());
  for (auto item = selectedIndices.end(); item != selectedIndices.begin();) {
    --item;
    m_model->removeDataByIndex(FitDomainIndex(item->row()));
  }
  updateTableFromModel();
}

void IndirectDataTablePresenter::updateTableFromModel() {
  ScopedFalse _signalBlock(m_emitCellChanged);
  m_dataTable->setRowCount(0);
  for (auto domainIndex = FitDomainIndex{0}; domainIndex < m_model->getNumberOfDomains(); domainIndex++) {
    addTableEntry(domainIndex);
  }
}

void IndirectDataTablePresenter::handleCellChanged(int irow, int column) {
  if (!m_emitCellChanged) {
    return;
  }
  FitDomainIndex row{static_cast<size_t>(irow)};

  if (startXColumn() == column) {
    setModelStartXAndEmit(getDouble(row, column), row);
  } else if (endXColumn() == column) {
    setModelEndXAndEmit(getDouble(row, column), row);
  } else if (excludeColumn() == column) {
    setModelExcludeAndEmit(getString(row, column), row);
  }
}

void IndirectDataTablePresenter::setModelStartXAndEmit(double startX, FitDomainIndex row) {
  auto subIndices = m_model->getSubIndices(row);
  m_model->setStartX(startX, subIndices.first, subIndices.second);
  emit startXChanged(startX, subIndices.first, subIndices.second);
}

void IndirectDataTablePresenter::setModelEndXAndEmit(double endX, FitDomainIndex row) {
  auto subIndices = m_model->getSubIndices(row);
  m_model->setEndX(endX, subIndices.first, subIndices.second);
  emit endXChanged(endX, subIndices.first, subIndices.second);
}

void IndirectDataTablePresenter::setModelExcludeAndEmit(const std::string &exclude, FitDomainIndex row) {
  auto subIndices = m_model->getSubIndices(row);
  m_model->setExcludeRegion(exclude, subIndices.first, subIndices.second);
  emit excludeRegionChanged(exclude, subIndices.first, subIndices.second);
}

void IndirectDataTablePresenter::clearTable() { m_dataTable->setRowCount(0); }

void IndirectDataTablePresenter::setColumnValues(int column, const QString &value) {
  MantidQt::API::SignalBlocker blocker(m_dataTable);
  for (int i = 0; i < m_dataTable->rowCount(); ++i)
    m_dataTable->item(i, column)->setText(value);
}

void IndirectDataTablePresenter::setHorizontalHeaders(const QStringList &headers) {
  m_dataTable->setColumnCount(headers.size());
  m_dataTable->setHorizontalHeaderLabels(headers);

  auto header = m_dataTable->horizontalHeader();
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  header->setResizeMode(0, QHeaderView::Stretch);
#elif QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
  header->setSectionResizeMode(0, QHeaderView::Stretch);
#endif
}

void IndirectDataTablePresenter::addTableEntry(FitDomainIndex row) {
  m_dataTable->insertRow(static_cast<int>(row.value));
  const auto &name = m_model->getWorkspace(row)->getName();
  auto cell = std::make_unique<QTableWidgetItem>(QString::fromStdString(name));
  auto flags = cell->flags();
  flags ^= Qt::ItemIsEditable;
  cell->setFlags(flags);
  setCell(std::move(cell), row.value, 0);

  cell = std::make_unique<QTableWidgetItem>(QString::number(m_model->getSpectrum(row)));
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

void IndirectDataTablePresenter::setCell(std::unique_ptr<QTableWidgetItem> cell, FitDomainIndex row, int column) {
  m_dataTable->setItem(static_cast<int>(row.value), column, cell.release());
}

void IndirectDataTablePresenter::setCellText(const QString &text, FitDomainIndex row, int column) {
  m_dataTable->item(static_cast<int>(row.value), column)->setText(text);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
