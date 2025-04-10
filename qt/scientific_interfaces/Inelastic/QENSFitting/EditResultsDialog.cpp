// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "EditResultsDialog.h"

namespace MantidQt::CustomInterfaces::Inelastic {

EditResultsDialog::EditResultsDialog(QWidget *parent) : QDialog(parent) {
  m_uiForm.setupUi(this);
  m_uiForm.wsInputWorkspace->setLowerBinLimit(2);
  m_uiForm.wsInputWorkspace->showWorkspaceGroups(false);

  m_uiForm.wsSingleFitWorkspace->setUpperBinLimit(1);
  m_uiForm.wsSingleFitWorkspace->showWorkspaceGroups(false);

  connect(m_uiForm.pbPasteInputName, &QPushButton::clicked, this, &EditResultsDialog::setOutputWorkspaceName);
  connect(m_uiForm.pbReplaceFitResult, &QPushButton::clicked, this, &EditResultsDialog::replaceSingleFitResult);
  connect(m_uiForm.pbClose, &QPushButton::clicked, this, &EditResultsDialog::close);
}

void EditResultsDialog::setWorkspaceSelectorSuffices(QStringList const &suffices) {
  m_uiForm.wsInputWorkspace->setSuffixes(suffices);
  m_uiForm.wsSingleFitWorkspace->setSuffixes(suffices);
}

void EditResultsDialog::setOutputWorkspaceName() {
  m_uiForm.leOutputWorkspace->setText(QString::fromStdString(getSelectedInputWorkspaceName()));
}

std::string EditResultsDialog::getSelectedInputWorkspaceName() const {
  return m_uiForm.wsInputWorkspace->currentText().toStdString();
}

std::string EditResultsDialog::getSelectedSingleFitWorkspaceName() const {
  return m_uiForm.wsSingleFitWorkspace->currentText().toStdString();
}

std::string EditResultsDialog::getOutputWorkspaceName() const {
  return m_uiForm.leOutputWorkspace->text().toStdString();
}

void EditResultsDialog::setReplaceFitResultText(QString const &text) { m_uiForm.pbReplaceFitResult->setText(text); }

void EditResultsDialog::setReplaceFitResultEnabled(bool enable) { m_uiForm.pbReplaceFitResult->setEnabled(enable); }

} // namespace MantidQt::CustomInterfaces::Inelastic
