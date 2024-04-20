// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FqFitDataView.h"
#include "Common/InterfaceUtils.h"
#include "Common/SettingsHelper.h"
#include "FqFitAddWorkspaceDialog.h"
#include "FqFitDataPresenter.h"

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

namespace MantidQt::CustomInterfaces::Inelastic {

FqFitDataView::FqFitDataView(QWidget *parent, std::string const &tabName)
    : FqFitDataView(FqFitHeaders(), parent, tabName) {
  connect(m_uiForm->pbAdd, SIGNAL(clicked()), this, SLOT(notifyAddClicked()));
}

FqFitDataView::FqFitDataView(const QStringList &headers, QWidget *parent, std::string const &tabName)
    : FitDataView(headers, parent, tabName) {
  auto header = m_uiForm->tbFitData->horizontalHeader();
  header->setSectionResizeMode(1, QHeaderView::Stretch);
}

void FqFitDataView::showAddWorkspaceDialog() {
  auto dialog = new FqFitAddWorkspaceDialog(parentWidget());
  connect(dialog, SIGNAL(addData(MantidWidgets::IAddWorkspaceDialog *)), this,
          SLOT(notifyAddData(MantidWidgets::IAddWorkspaceDialog *)));
  connect(dialog, SIGNAL(workspaceChanged(FqFitAddWorkspaceDialog *, const std::string &)), this,
          SLOT(notifyWorkspaceChanged(FqFitAddWorkspaceDialog *, const std::string &)));
  connect(dialog, SIGNAL(parameterTypeChanged(FqFitAddWorkspaceDialog *, const std::string &)), this,
          SLOT(notifyParameterTypeChanged(FqFitAddWorkspaceDialog *, const std::string &)));

  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWSSuffices(InterfaceUtils::getSampleWSSuffixes(m_tabName));
  dialog->setFBSuffices(InterfaceUtils::getSampleFBSuffixes(m_tabName));
  dialog->updateSelectedSpectra();
  dialog->show();
}

void FqFitDataView::notifyAddClicked() {
  if (auto presenter = dynamic_cast<FqFitDataPresenter *>(m_presenter)) {
    presenter->handleAddClicked();
  }
}

void FqFitDataView::notifyWorkspaceChanged(FqFitAddWorkspaceDialog *dialog, const std::string &workspaceName) {
  if (auto presenter = dynamic_cast<FqFitDataPresenter *>(m_presenter)) {
    presenter->handleWorkspaceChanged(dialog, workspaceName);
  }
}

void FqFitDataView::notifyParameterTypeChanged(FqFitAddWorkspaceDialog *dialog, const std::string &type) {
  if (auto presenter = dynamic_cast<FqFitDataPresenter *>(m_presenter)) {
    presenter->handleParameterTypeChanged(dialog, type);
  }
}

void FqFitDataView::addTableEntry(size_t row, FitDataRow newRow) {
  FitDataView::addTableEntry(row, newRow);

  auto cell = std::make_unique<QTableWidgetItem>(QString::fromStdString(newRow.parameter));
  auto flags = cell->flags();
  flags ^= Qt::ItemIsEditable;
  cell->setFlags(flags);
  setCell(std::move(cell), row, 1);
}

} // namespace MantidQt::CustomInterfaces::Inelastic