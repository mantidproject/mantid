// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ConvFitDataView.h"

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

namespace MantidQt::CustomInterfaces::IDA {

ConvFitDataView::ConvFitDataView(QWidget *parent) : ConvFitDataView(convFitHeaders(), parent) {}

ConvFitDataView::ConvFitDataView(const QStringList &headers, QWidget *parent) : IndirectFitDataView(headers, parent) {
  auto header = m_uiForm->tbFitData->horizontalHeader();
  header->setSectionResizeMode(1, QHeaderView::Stretch);
}

int ConvFitDataView::workspaceIndexColumn() const { return 2; }

int ConvFitDataView::startXColumn() const { return 3; }

int ConvFitDataView::endXColumn() const { return 4; }

int ConvFitDataView::excludeColumn() const { return 5; }

void ConvFitDataView::addTableEntry(size_t row, FitDataRow newRow) {
  IndirectFitDataView::addTableEntry(row, newRow);

  auto cell = std::make_unique<QTableWidgetItem>(QString::fromStdString(newRow.resolution));
  auto flags = cell->flags();
  flags ^= Qt::ItemIsEditable;
  cell->setFlags(flags);
  setCell(std::move(cell), row, 1);
}

} // namespace MantidQt::CustomInterfaces::IDA