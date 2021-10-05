// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FqFitDataView.h"

#include <QComboBox>
#include <QHeaderView>
#include <QtGlobal>

namespace {
QStringList FqFitHeaders() {
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

namespace MantidQt::CustomInterfaces::IDA {

FqFitDataView::FqFitDataView(QWidget *parent) : FqFitDataView(FqFitHeaders(), parent) {}

FqFitDataView::FqFitDataView(const QStringList &headers, QWidget *parent) : IndirectFitDataView(headers, parent) {
  auto header = m_uiForm->tbFitData->horizontalHeader();
  header->setSectionResizeMode(1, QHeaderView::Stretch);
}

int FqFitDataView::workspaceIndexColumn() const { return 2; }

int FqFitDataView::startXColumn() const { return 3; }

int FqFitDataView::endXColumn() const { return 4; }

int FqFitDataView::excludeColumn() const { return 5; }

void FqFitDataView::addTableEntry(size_t row, FitDataRow newRow) {
  IndirectFitDataView::addTableEntry(row, newRow);

  auto cell = std::make_unique<QTableWidgetItem>(QString::fromStdString(newRow.parameter));
  auto flags = cell->flags();
  flags ^= Qt::ItemIsEditable;
  cell->setFlags(flags);
  setCell(std::move(cell), row, 1);
}

} // namespace MantidQt::CustomInterfaces::IDA