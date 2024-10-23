// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FunctionQDataView.h"
#include "FunctionQAddWorkspaceDialog.h"
#include "FunctionQDataPresenter.h"
#include "MantidQtWidgets/Spectroscopy/InterfaceUtils.h"
#include "MantidQtWidgets/Spectroscopy/SettingsWidget/SettingsHelper.h"

#include <QComboBox>
#include <QHeaderView>
#include <QtGlobal>

namespace {
QStringList functionQHeaders() {
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

FunctionQDataView::FunctionQDataView(QWidget *parent) : FunctionQDataView(functionQHeaders(), parent) {
  connect(m_uiForm->pbAdd, &QPushButton::clicked, this, &FunctionQDataView::notifyAddClicked);
}

FunctionQDataView::FunctionQDataView(const QStringList &headers, QWidget *parent) : FitDataView(headers, parent) {
  auto header = m_uiForm->tbFitData->horizontalHeader();
  header->setSectionResizeMode(1, QHeaderView::Stretch);
}

void FunctionQDataView::showAddWorkspaceDialog() {
  auto dialog = new FunctionQAddWorkspaceDialog(parentWidget());

  connect(dialog, &FunctionQAddWorkspaceDialog::addData, this, &FunctionQDataView::notifyAddData);
  connect(dialog, &FunctionQAddWorkspaceDialog::workspaceChanged, this, &FunctionQDataView::notifyWorkspaceChanged);
  connect(dialog, &FunctionQAddWorkspaceDialog::parameterTypeChanged, this,
          &FunctionQDataView::notifyParameterTypeChanged);

  auto tabName = m_presenter->tabName();
  dialog->setAttribute(Qt::WA_DeleteOnClose);
  dialog->setWSSuffices(InterfaceUtils::getSampleWSSuffixes(tabName));
  dialog->setFBSuffices(InterfaceUtils::getSampleFBSuffixes(tabName));
  dialog->setLoadProperty("LoadHistory", SettingsHelper::loadHistory());
  dialog->updateSelectedSpectra();
  dialog->show();
}

void FunctionQDataView::notifyAddClicked() {
  if (auto presenter = dynamic_cast<FunctionQDataPresenter *>(m_presenter)) {
    presenter->handleAddClicked();
  }
}

void FunctionQDataView::notifyWorkspaceChanged(FunctionQAddWorkspaceDialog *dialog, const std::string &workspaceName) {
  if (auto presenter = dynamic_cast<FunctionQDataPresenter *>(m_presenter)) {
    presenter->handleWorkspaceChanged(dialog, workspaceName);
  }
}

void FunctionQDataView::notifyParameterTypeChanged(FunctionQAddWorkspaceDialog *dialog, const std::string &type) {
  if (auto presenter = dynamic_cast<FunctionQDataPresenter *>(m_presenter)) {
    presenter->handleParameterTypeChanged(dialog, type);
  }
}

void FunctionQDataView::addTableEntry(size_t row, FitDataRow const &newRow) {
  FitDataView::addTableEntry(row, newRow);

  auto cell = std::make_unique<QTableWidgetItem>(QString::fromStdString(newRow.parameter));
  auto flags = cell->flags();
  flags ^= Qt::ItemIsEditable;
  cell->setFlags(flags);
  setCell(std::move(cell), row, 1);
}

} // namespace MantidQt::CustomInterfaces::Inelastic