// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/AddWorkspaceDialog.h"
#include "MantidQtWidgets/Common/TableWidgetValidators.h"
#include "MantidQtWidgets/Common/WorkspaceUtils.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include <utility>

namespace MantidQt::MantidWidgets {

AddWorkspaceDialog::AddWorkspaceDialog(QWidget *parent) : QDialog(parent) {
  m_uiForm.setupUi(this);
  const auto validatorString = QString::fromStdString(getRegexValidatorString(RegexValidatorStrings::SpectraValidator));
  m_uiForm.leWorkspaceIndices->setValidator(new QRegExpValidator(QRegExp(validatorString), this));
  setAllSpectraSelectionEnabled(false);
  connect(m_uiForm.dsWorkspace, SIGNAL(filesAutoLoaded()), this, SLOT(handleAutoLoaded()));
  connect(m_uiForm.dsWorkspace, SIGNAL(dataReady(const QString &)), this, SLOT(workspaceChanged(const QString &)));
  connect(m_uiForm.ckAllSpectra, SIGNAL(stateChanged(int)), this, SLOT(selectAllSpectra(int)));
  connect(m_uiForm.pbAdd, SIGNAL(clicked()), this, SLOT(emitAddData()));
  connect(m_uiForm.pbClose, SIGNAL(clicked()), this, SLOT(close()));
}

std::string AddWorkspaceDialog::workspaceName() const {
  return m_uiForm.dsWorkspace->getCurrentDataName().toStdString();
}

FunctionModelSpectra AddWorkspaceDialog::workspaceIndices() const {
  return FunctionModelSpectra(m_uiForm.leWorkspaceIndices->text().toStdString());
}

void AddWorkspaceDialog::setWSSuffices(const QStringList &suffices) { m_uiForm.dsWorkspace->setWSSuffixes(suffices); }

void AddWorkspaceDialog::setFBSuffices(const QStringList &suffices) { m_uiForm.dsWorkspace->setFBSuffixes(suffices); }

void AddWorkspaceDialog::setLoadProperty(const std::string &propName, bool enable) {
  m_uiForm.dsWorkspace->setLoadProperty(propName, enable);
}

void AddWorkspaceDialog::updateSelectedSpectra() {
  auto const state = m_uiForm.ckAllSpectra->isChecked() ? Qt::Checked : Qt::Unchecked;
  selectAllSpectra(state);
}

void AddWorkspaceDialog::selectAllSpectra(int state) {
  auto const name = workspaceName();
  if (WorkspaceUtils::doesExistInADS(name) && state == Qt::Checked) {
    m_uiForm.leWorkspaceIndices->setText(QString::fromStdString(WorkspaceUtils::getIndexString(name)));
    m_uiForm.leWorkspaceIndices->setEnabled(false);
  } else
    m_uiForm.leWorkspaceIndices->setEnabled(true);
}

void AddWorkspaceDialog::workspaceChanged(const QString &workspaceName) {
  const auto name = workspaceName.toStdString();
  const auto workspace = WorkspaceUtils::getADSWorkspace(name);
  m_uiForm.pbAdd->setText("Add");
  m_uiForm.pbAdd->setEnabled(true);

  if (workspace)
    setWorkspace(name);
  else
    setAllSpectraSelectionEnabled(false);
}

void AddWorkspaceDialog::emitAddData() { emit addData(this); }

void AddWorkspaceDialog::handleAutoLoaded() {
  m_uiForm.pbAdd->setText("Loading");
  m_uiForm.pbAdd->setEnabled(false);
}

void AddWorkspaceDialog::setWorkspace(const std::string &workspace) {
  setAllSpectraSelectionEnabled(true);
  if (m_uiForm.ckAllSpectra->isChecked()) {
    m_uiForm.leWorkspaceIndices->setText(QString::fromStdString(WorkspaceUtils::getIndexString(workspace)));
    m_uiForm.leWorkspaceIndices->setEnabled(false);
  }
}

void AddWorkspaceDialog::setAllSpectraSelectionEnabled(bool doEnable) { m_uiForm.ckAllSpectra->setEnabled(doEnable); }

std::string AddWorkspaceDialog::getFileName() const { return m_uiForm.dsWorkspace->getFullFilePath().toStdString(); }

} // namespace MantidQt::MantidWidgets
