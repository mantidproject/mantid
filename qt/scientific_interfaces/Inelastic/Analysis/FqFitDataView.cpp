// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FqFitDataView.h"
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

namespace MantidQt::CustomInterfaces::IDA {

FqFitDataView::FqFitDataView(QWidget *parent) : FqFitDataView(FqFitHeaders(), parent) {
  connect(m_uiForm->pbAdd, SIGNAL(clicked()), this, SLOT(notifyAddClicked()));
}

FqFitDataView::FqFitDataView(const QStringList &headers, QWidget *parent) : IndirectFitDataView(headers, parent) {
  auto header = m_uiForm->tbFitData->horizontalHeader();
  header->setSectionResizeMode(1, QHeaderView::Stretch);
}

IAddWorkspaceDialog *FqFitDataView::getAddWorkspaceDialog() {
  m_addWorkspaceDialog = new FqFitAddWorkspaceDialog(parentWidget());

  connect(m_addWorkspaceDialog, SIGNAL(addData()), this, SLOT(notifyAddData()));
  connect(m_addWorkspaceDialog, SIGNAL(workspaceChanged(FqFitAddWorkspaceDialog *, const std::string &)), this,
          SLOT(notifyWorkspaceChanged(FqFitAddWorkspaceDialog *, const std::string &)));
  connect(m_addWorkspaceDialog, SIGNAL(parameterTypeChanged(FqFitAddWorkspaceDialog *, const std::string &)), this,
          SLOT(notifyParameterTypeChanged(FqFitAddWorkspaceDialog *, const std::string &)));

  return m_addWorkspaceDialog;
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
    presenter->handleParameterTypeChanged(dynamic_cast<FqFitAddWorkspaceDialog *>(m_addWorkspaceDialog), type);
  }
}

void FqFitDataView::addTableEntry(size_t row, FitDataRow newRow) {
  IndirectFitDataView::addTableEntry(row, newRow);

  auto cell = std::make_unique<QTableWidgetItem>(QString::fromStdString(newRow.parameter));
  auto flags = cell->flags();
  flags ^= Qt::ItemIsEditable;
  cell->setFlags(flags);
  setCell(std::move(cell), row, 1);
}

} // namespace MantidQt::CustomInterfaces::IDA