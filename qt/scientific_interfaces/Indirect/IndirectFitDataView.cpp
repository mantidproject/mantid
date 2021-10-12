// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectFitDataView.h"
#include "MantidQtWidgets/Common/IndexTypes.h"
#include <QItemDelegate>
#include <QRegExpValidator>

using namespace Mantid::API;

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

QString makeNumber(double d) { return QString::number(d, 'g', 16); }

QStringList defaultHeaders() {
  QStringList headers;
  headers << "Workspace"
          << "WS Index"
          << "StartX"
          << "EndX"
          << "Mask X Range";
  return headers;
}
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectFitDataView::IndirectFitDataView(QWidget *parent) : IndirectFitDataView(defaultHeaders(), parent) {}

IndirectFitDataView::IndirectFitDataView(const QStringList &headers, QWidget *parent)
    : IIndirectFitDataView(parent), m_uiForm(new Ui::IndirectFitDataView) {
  m_uiForm->setupUi(this);

  setHorizontalHeaders(headers);

  connect(m_uiForm->tbFitData, SIGNAL(cellChanged(int, int)), this, SIGNAL(cellChanged(int, int)));
  connect(m_uiForm->pbAdd, SIGNAL(clicked()), this, SIGNAL(addClicked()));
  connect(m_uiForm->pbRemove, SIGNAL(clicked()), this, SIGNAL(removeClicked()));
  connect(m_uiForm->pbUnify, SIGNAL(clicked()), this, SIGNAL(unifyClicked()));
}

QTableWidget *IndirectFitDataView::getDataTable() const { return m_uiForm->tbFitData; }

void IndirectFitDataView::setHorizontalHeaders(const QStringList &headers) {
  m_uiForm->tbFitData->setColumnCount(headers.size());
  m_uiForm->tbFitData->setHorizontalHeaderLabels(headers);

  auto header = m_uiForm->tbFitData->horizontalHeader();
  header->setSectionResizeMode(0, QHeaderView::Stretch);
  m_uiForm->tbFitData->setItemDelegateForColumn(headers.size() - 1,
                                                std::make_unique<ExcludeRegionDelegate>().release());
  m_uiForm->tbFitData->verticalHeader()->setVisible(false);
}

UserInputValidator &IndirectFitDataView::validate(UserInputValidator &validator) {
  if (m_uiForm->tbFitData->rowCount() == 0)
    validator.addErrorMessage("No input data has been provided.");
  return validator;
}

void IndirectFitDataView::displayWarning(const std::string &warning) {
  QMessageBox::warning(parentWidget(), "MantidPlot - Warning", QString::fromStdString(warning));
}

void IndirectFitDataView::addTableEntry(size_t row, FitDataRow newRow) {
  m_uiForm->tbFitData->insertRow(static_cast<int>(row));

  auto cell = std::make_unique<QTableWidgetItem>(QString::fromStdString(newRow.name));
  auto flags = cell->flags();
  flags ^= Qt::ItemIsEditable;
  cell->setFlags(flags);
  setCell(std::move(cell), row, 0);

  cell = std::make_unique<QTableWidgetItem>(QString::number(newRow.workspaceIndex));
  cell->setFlags(flags);
  setCell(std::move(cell), row, workspaceIndexColumn());

  cell = std::make_unique<QTableWidgetItem>(makeNumber(newRow.startX));
  setCell(std::move(cell), row, startXColumn());

  cell = std::make_unique<QTableWidgetItem>(makeNumber(newRow.endX));
  setCell(std::move(cell), row, endXColumn());

  cell = std::make_unique<QTableWidgetItem>(QString::fromStdString(newRow.exclude));
  setCell(std::move(cell), row, excludeColumn());
}

bool IndirectFitDataView::isTableEmpty() const { return m_uiForm->tbFitData->rowCount() == 0; }

int IndirectFitDataView::workspaceIndexColumn() const { return 1; }

int IndirectFitDataView::startXColumn() const { return 2; }

int IndirectFitDataView::endXColumn() const { return 3; }

int IndirectFitDataView::excludeColumn() const { return 4; }

void IndirectFitDataView::clearTable() { m_uiForm->tbFitData->setRowCount(0); }

void IndirectFitDataView::setCell(std::unique_ptr<QTableWidgetItem> cell, size_t row, size_t column) {
  m_uiForm->tbFitData->setItem(static_cast<int>(row), static_cast<int>(column), cell.release());
}

QString IndirectFitDataView::getText(int row, int column) const {
  return m_uiForm->tbFitData->item(static_cast<int>(row), column)->text();
}

QModelIndexList IndirectFitDataView::getSelectedIndexes() const {
  return m_uiForm->tbFitData->selectionModel()->selectedIndexes();
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
