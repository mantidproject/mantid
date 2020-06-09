// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "JumpFitDataTablePresenter.h"
#include "MantidAPI/TextAxis.h"

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
    : IndirectDataTablePresenter(model->m_fitDataModel.get(), dataTable,
                                 jumpFitHeaders()) {
  auto header = dataTable->horizontalHeader();
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  header->setResizeMode(1, QHeaderView::Stretch);
#elif QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
  header->setSectionResizeMode(1, QHeaderView::Stretch);
#endif
}

int JumpFitDataTablePresenter::workspaceIndexColumn() const { return 2; }

int JumpFitDataTablePresenter::startXColumn() const { return 3; }

int JumpFitDataTablePresenter::endXColumn() const { return 4; }

int JumpFitDataTablePresenter::excludeColumn() const { return 5; }

void JumpFitDataTablePresenter::addTableEntry(FitDomainIndex row) {
  IndirectDataTablePresenter::addTableEntry(row);

  auto subIndices = m_model->getSubIndices(row);
  const auto workspace = m_model->getWorkspace(subIndices.first);
  const auto axis =
      dynamic_cast<Mantid::API::TextAxis *>(workspace->getAxis(1));
  const auto parameter = axis->label(subIndices.second.value);
  auto cell =
      std::make_unique<QTableWidgetItem>(QString::fromStdString(parameter));
  auto flags = cell->flags();
  flags ^= Qt::ItemIsEditable;
  cell->setFlags(flags);
  setCell(std::move(cell), row, 1);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
