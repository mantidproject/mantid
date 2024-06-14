// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "Common/InterfaceUtils.h"
#include "ConvFitAddWorkspaceDialog.h"
#include "ConvFitDataView.h"
#include "FitDataPresenter.h"

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

namespace MantidQt::CustomInterfaces::Inelastic {

ConvFitDataView::ConvFitDataView(QWidget *parent) : ConvFitDataView(convFitHeaders(), parent) {}

ConvFitDataView::ConvFitDataView(const QStringList &headers, QWidget *parent) : FitDataView(headers, parent) {
  auto header = m_uiForm->tbFitData->horizontalHeader();
  header->setSectionResizeMode(1, QHeaderView::Stretch);
}

void ConvFitDataView::showAddWorkspaceDialog() {
  auto dialog = new ConvFitAddWorkspaceDialog(parentWidget());
  connect(dialog, SIGNAL(addData(MantidWidgets::IAddWorkspaceDialog *)), this,
          SLOT(notifyAddData(MantidWidgets::IAddWorkspaceDialog *)));

  auto tabName = m_presenter->tabName();
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWSSuffices(InterfaceUtils::getSampleWSSuffixes(tabName));
  dialog->setFBSuffices(InterfaceUtils::getSampleFBSuffixes(tabName));
  dialog->setResolutionWSSuffices(InterfaceUtils::getResolutionWSSuffixes(tabName));
  dialog->setResolutionFBSuffices(InterfaceUtils::getResolutionFBSuffixes(tabName));
  dialog->updateSelectedSpectra();
  dialog->show();
}

void ConvFitDataView::addTableEntry(size_t row, FitDataRow newRow) {
  FitDataView::addTableEntry(row, newRow);

  auto cell = std::make_unique<QTableWidgetItem>(QString::fromStdString(newRow.resolution));
  auto flags = cell->flags();
  flags ^= Qt::ItemIsEditable;
  cell->setFlags(flags);
  setCell(std::move(cell), row, 1);
}

} // namespace MantidQt::CustomInterfaces::Inelastic