// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ConvFitDataTablePresenter.h"

#include <QComboBox>
#include <QHeaderView>
#include <QtGlobal>

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

ConvFitDataTablePresenter::ConvFitDataTablePresenter(ConvFitModel *model, QTableWidget *dataTable)
    : IndirectFitDataTablePresenter(model->getFitDataModel(), dataTable, convFitHeaders()) {
  auto header = dataTable->horizontalHeader();
  header->setSectionResizeMode(1, QHeaderView::Stretch);
}

int ConvFitDataTablePresenter::workspaceIndexColumn() const { return 2; }

int ConvFitDataTablePresenter::startXColumn() const { return 3; }

int ConvFitDataTablePresenter::endXColumn() const { return 4; }

int ConvFitDataTablePresenter::excludeColumn() const { return 5; }

void ConvFitDataTablePresenter::addTableEntry(FitDomainIndex row) {
  IndirectFitDataTablePresenter::addTableEntry(row);

  auto resolutionVector = m_model->getResolutionsForFit();
  const auto name = resolutionVector.at(row.value).first;
  auto cell = std::make_unique<QTableWidgetItem>(QString::fromStdString(name));
  auto flags = cell->flags();
  flags ^= Qt::ItemIsEditable;
  cell->setFlags(flags);
  setCell(std::move(cell), row, 1);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
