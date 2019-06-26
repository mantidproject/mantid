// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
          << "Mask X Range";
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

void JumpFitDataTablePresenter::addTableEntry(DatasetIndex dataIndex,
  WorkspaceIndex spectrum, SpectrumRowIndex row) {
  IndirectDataTablePresenter::addTableEntry(dataIndex, spectrum, row);

  const auto parameter =
      m_jumpFitModel->getFitParameterName(dataIndex, spectrum);
  auto cell =
      std::make_unique<QTableWidgetItem>(QString::fromStdString(parameter));
  auto flags = cell->flags();
  flags ^= Qt::ItemIsEditable;
  cell->setFlags(flags);
  setCell(std::move(cell), row, 1);
}

void JumpFitDataTablePresenter::updateTableEntry(DatasetIndex dataIndex,
  WorkspaceIndex spectrum,
  SpectrumRowIndex row) {
  IndirectDataTablePresenter::updateTableEntry(dataIndex, spectrum, row);

  const auto parameter =
      m_jumpFitModel->getFitParameterName(dataIndex, spectrum);
  setCellText(QString::fromStdString(parameter), row, 1);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
