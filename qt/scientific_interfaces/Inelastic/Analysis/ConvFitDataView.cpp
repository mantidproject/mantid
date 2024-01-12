// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ConvFitDataView.h"
#include "ConvFitAddWorkspaceDialog.h"

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

IAddWorkspaceDialog *ConvFitDataView::getAddWorkspaceDialog() {
  m_addWorkspaceDialog = new ConvFitAddWorkspaceDialog(parentWidget());
  if (auto dialog = dynamic_cast<ConvFitAddWorkspaceDialog *>(m_addWorkspaceDialog)) {
    dialog->setResolutionWSSuffices(m_wsResolutionSuffixes);
    dialog->setResolutionFBSuffices(m_fbResolutionSuffixes);
  }
  connect(m_addWorkspaceDialog, SIGNAL(addData()), this, SLOT(notifyAddData()));

  return m_addWorkspaceDialog;
}

void ConvFitDataView::addTableEntry(size_t row, FitDataRow newRow) {
  IndirectFitDataView::addTableEntry(row, newRow);

  auto cell = std::make_unique<QTableWidgetItem>(QString::fromStdString(newRow.resolution));
  auto flags = cell->flags();
  flags ^= Qt::ItemIsEditable;
  cell->setFlags(flags);
  setCell(std::move(cell), row, 1);
}

} // namespace MantidQt::CustomInterfaces::IDA