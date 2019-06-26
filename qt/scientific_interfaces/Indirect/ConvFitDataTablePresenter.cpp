// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ConvFitDataTablePresenter.h"

#include <QComboBox>
#include <QHeaderView>

namespace {
QStringList convFitHeaders() {
  QStringList headers;
  headers << "Workspace"
          << "Resolution"
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

ConvFitDataTablePresenter::ConvFitDataTablePresenter(ConvFitModel *model,
                                                     QTableWidget *dataTable)
    : IndirectDataTablePresenter(model, dataTable, convFitHeaders()),
      m_convFitModel(model) {
  auto header = dataTable->horizontalHeader();
  header->setResizeMode(1, QHeaderView::Stretch);
}

int ConvFitDataTablePresenter::workspaceIndexColumn() const { return 2; }

int ConvFitDataTablePresenter::startXColumn() const { return 3; }

int ConvFitDataTablePresenter::endXColumn() const { return 4; }

int ConvFitDataTablePresenter::excludeColumn() const { return 5; }

std::string ConvFitDataTablePresenter::getResolutionName(SpectrumRowIndex row) const {
  return getString(row, 1);
}

void ConvFitDataTablePresenter::addTableEntry(DatasetIndex dataIndex,
                                              WorkspaceIndex spectrum, SpectrumRowIndex row) {
  IndirectDataTablePresenter::addTableEntry(dataIndex, spectrum, row);

  const auto resolution = m_convFitModel->getResolution(dataIndex);
  const auto name = resolution ? resolution->getName() : "";
  auto cell = std::make_unique<QTableWidgetItem>(QString::fromStdString(name));
  auto flags = cell->flags();
  flags ^= Qt::ItemIsEditable;
  cell->setFlags(flags);
  setCell(std::move(cell), row, 1);
}

void ConvFitDataTablePresenter::updateTableEntry(DatasetIndex dataIndex,
                                                 WorkspaceIndex spectrum,
                                                 SpectrumRowIndex row) {
  IndirectDataTablePresenter::updateTableEntry(dataIndex, spectrum, row);

  const auto &name = m_convFitModel->getResolution(dataIndex)->getName();
  setCellText(QString::fromStdString(name), row, 1);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
