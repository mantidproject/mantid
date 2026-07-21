// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ConvolutionDataView.h"
#include "ConvolutionAddWorkspaceDialog.h"
#include "FitDataPresenter.h"
#include "MantidQtWidgets/Spectroscopy/InterfaceUtils.h"

#include <MantidQtWidgets/Spectroscopy/SettingsWidget/SettingsHelper.h>
#include <QComboBox>
#include <QHeaderView>
#include <QtGlobal>

namespace {
QStringList convolutionHeaders() {
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

ConvolutionDataView::ConvolutionDataView(QWidget *parent) : ConvolutionDataView(convolutionHeaders(), parent) {}

ConvolutionDataView::ConvolutionDataView(const QStringList &headers, QWidget *parent) : FitDataView(headers, parent) {
  auto header = m_uiForm->tbFitData->horizontalHeader();
  header->setSectionResizeMode(1, QHeaderView::Stretch);
}

void ConvolutionDataView::showAddWorkspaceDialog() {
  auto tabName = m_presenter->tabName();
  auto dialog = createAddWorkspaceDialog(InterfaceUtils::getSampleFBSuffixes(tabName),
                                         InterfaceUtils::getResolutionFBSuffixes(tabName));
  connect(dialog, &ConvolutionAddWorkspaceDialog::addData, this, &ConvolutionDataView::notifyAddData);
}

void ConvolutionDataView::showAddNumericWorkspaceDialog() {
  auto dialog = createAddWorkspaceDialog({".*"}, {".*"});
  connect(dialog, &ConvolutionAddWorkspaceDialog::addData, this, &ConvolutionDataView::notifyAddNumericData);
}

ConvolutionAddWorkspaceDialog *
ConvolutionDataView::createAddWorkspaceDialog(const QStringList &sampleFileSuffixes,
                                              const QStringList &resolutionFileSuffixes) const {
  auto dialog = new ConvolutionAddWorkspaceDialog(parentWidget());
  auto tabName = m_presenter->tabName();
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWSSuffices(InterfaceUtils::getSampleWSSuffixes(tabName));
  dialog->setFBSuffices(sampleFileSuffixes);
  dialog->setResolutionWSSuffices(InterfaceUtils::getResolutionWSSuffixes(tabName));
  dialog->setResolutionFBSuffices(resolutionFileSuffixes);
  dialog->setLoadProperty("LoadHistory", SettingsHelper::loadHistory());
  dialog->updateSelectedSpectra();
  dialog->show();
  return dialog;
}

void ConvolutionDataView::addTableEntry(size_t row, FitDataRow const &newRow) {
  FitDataView::addTableEntry(row, newRow);

  auto cell = std::make_unique<QTableWidgetItem>(QString::fromStdString(newRow.resolution));
  auto flags = cell->flags();
  flags ^= Qt::ItemIsEditable;
  cell->setFlags(flags);
  setCell(std::move(cell), row, 1);
}

} // namespace MantidQt::CustomInterfaces::Inelastic
