#include "JumpFitDataTablePresenter.h"

#include <QComboBox>
#include <QHeaderView>

namespace {
QStringList jumpFitHeaders() {
  QStringList headers;
  headers << "Workspace"
          << "Parameter"
          << "WS Index"
          << "StartX"
          << "EndX"
          << "Exclude";
  return headers;
}
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

JumpFitDataTablePresenter::JumpFitDataTablePresenter(JumpFitModel *model,
                                                     QTableWidget *dataTable)
    : IndirectDataTablePresenter(model, dataTable, jumpFitHeaders()),
      m_jumpFitModel(model) {
  auto header = dataTable->horizontalHeader();
  header->setResizeMode(1, QHeaderView::Stretch);
}

int JumpFitDataTablePresenter::workspaceIndexColumn() const { return 2; }

int JumpFitDataTablePresenter::startXColumn() const { return 3; }

int JumpFitDataTablePresenter::endXColumn() const { return 4; }

int JumpFitDataTablePresenter::excludeColumn() const { return 5; }

void JumpFitDataTablePresenter::addTableEntry(std::size_t dataIndex,
                                              std::size_t spectrum, int row) {
  IndirectDataTablePresenter::addTableEntry(dataIndex, spectrum, row);

  const auto parameter =
      m_jumpFitModel->getFitParameterName(dataIndex, spectrum);
  auto cell = new QTableWidgetItem(QString::fromStdString(parameter));
  setCell(cell, row, 1);
  auto flags = cell->flags();
  flags ^= Qt::ItemIsEditable;
  cell->setFlags(flags);
}

void JumpFitDataTablePresenter::updateTableEntry(std::size_t dataIndex,
                                                 std::size_t spectrum,
                                                 int row) {
  IndirectDataTablePresenter::updateTableEntry(dataIndex, spectrum, row);

  const auto parameter =
      m_jumpFitModel->getFitParameterName(dataIndex, spectrum);
  setCellText(QString::fromStdString(parameter), row, 1);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
