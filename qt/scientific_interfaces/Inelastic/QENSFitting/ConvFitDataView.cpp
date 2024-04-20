// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ConvFitDataView.h"
#include "Common/InterfaceUtils.h"
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

namespace MantidQt::CustomInterfaces::Inelastic {

ConvFitDataView::ConvFitDataView(QWidget *parent, std::string const &tabName)
    : ConvFitDataView(convFitHeaders(), parent, tabName) {}

ConvFitDataView::ConvFitDataView(const QStringList &headers, QWidget *parent, std::string const &tabName)
    : FitDataView(headers, parent, tabName) {
  auto header = m_uiForm->tbFitData->horizontalHeader();
  header->setSectionResizeMode(1, QHeaderView::Stretch);
}

void ConvFitDataView::showAddWorkspaceDialog() {
  auto dialog = new ConvFitAddWorkspaceDialog(parentWidget());
  connect(dialog, SIGNAL(addData(MantidWidgets::IAddWorkspaceDialog *)), this,
          SLOT(notifyAddData(MantidWidgets::IAddWorkspaceDialog *)));

  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWSSuffices(InterfaceUtils::getSampleWSSuffixes(m_tabName));
  dialog->setFBSuffices(InterfaceUtils::getSampleFBSuffixes(m_tabName));
  dialog->setResolutionWSSuffices(InterfaceUtils::getResolutionWSSuffixes(m_tabName));
  dialog->setResolutionFBSuffices(InterfaceUtils::getResolutionFBSuffixes(m_tabName));
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