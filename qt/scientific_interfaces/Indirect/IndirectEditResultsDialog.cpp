// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectEditResultsDialog.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectEditResultsDialog::IndirectEditResultsDialog(QWidget *parent)
    : QDialog(parent) {
  m_uiForm.setupUi(this);
  m_uiForm.wsInputWorkspace->setLowerBinLimit(2);
  m_uiForm.wsSingleSpectrumWorkspace->setUpperBinLimit(1);

  connect(m_uiForm.pbPasteInputName, SIGNAL(clicked()), this,
          SLOT(setOutputWorkspaceName()));
  connect(m_uiForm.pbReplaceBin, SIGNAL(clicked()), this,
          SIGNAL(replaceSingleBin()));
  connect(m_uiForm.pbClose, SIGNAL(clicked()), this, SIGNAL(closeDialog()));
}

void IndirectEditResultsDialog::setWorkspaceSelectorSuffices(
    QStringList const &suffices) {
  m_uiForm.wsInputWorkspace->setSuffixes(suffices);
  m_uiForm.wsSingleSpectrumWorkspace->setSuffixes(suffices);
}

void IndirectEditResultsDialog::setOutputWorkspaceName() {
  m_uiForm.leOutputWorkspace->setText(
      QString::fromStdString(getSelectedInputWorkspaceName()));
}

std::string IndirectEditResultsDialog::getSelectedInputWorkspaceName() const {
  return m_uiForm.wsInputWorkspace->currentText().toStdString();
}

std::string
IndirectEditResultsDialog::getSelectedSingleFitWorkspaceName() const {
  return m_uiForm.wsSingleSpectrumWorkspace->currentText().toStdString();
}

std::string IndirectEditResultsDialog::getOutputWorkspaceName() const {
  return m_uiForm.leOutputWorkspace->text().toStdString();
}

void IndirectEditResultsDialog::setReplaceBinText(QString const &text) {
  m_uiForm.pbReplaceBin->setText(text);
}

void IndirectEditResultsDialog::setReplaceBinEnabled(bool enable) {
  m_uiForm.pbReplaceBin->setEnabled(enable);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
