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
    : IndirectDataTablePresenter(model->m_fitDataModel.get(), dataTable, convFitHeaders()) {
  auto header = dataTable->horizontalHeader();
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  header->setResizeMode(1, QHeaderView::Stretch);
#else
  header->setSectionResizeMode(1, QHeaderView::Stretch);
#endif
}

int ConvFitDataTablePresenter::workspaceIndexColumn() const { return 2; }

int ConvFitDataTablePresenter::startXColumn() const { return 3; }

int ConvFitDataTablePresenter::endXColumn() const { return 4; }

int ConvFitDataTablePresenter::excludeColumn() const { return 5; }

std::string ConvFitDataTablePresenter::getResolutionName(FitDomainIndex row) const { return getString(row, 1); }

void ConvFitDataTablePresenter::addTableEntry(FitDomainIndex row) {
  IndirectDataTablePresenter::addTableEntry(row);

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
