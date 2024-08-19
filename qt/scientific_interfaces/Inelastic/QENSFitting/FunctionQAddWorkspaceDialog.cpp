// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FunctionQAddWorkspaceDialog.h"

#include <boost/optional.hpp>

#include <QSignalBlocker>

namespace MantidQt::CustomInterfaces::Inelastic {

FunctionQAddWorkspaceDialog::FunctionQAddWorkspaceDialog(QWidget *parent) : QDialog(parent) {
  m_uiForm.setupUi(this);

  connect(m_uiForm.dsWorkspace, SIGNAL(dataReady(const QString &)), this, SLOT(emitWorkspaceChanged(const QString &)));
  connect(m_uiForm.dsWorkspace, SIGNAL(filesAutoLoaded()), this, SLOT(handleAutoLoaded()));
  connect(m_uiForm.cbParameterType, SIGNAL(currentIndexChanged(const QString &)), this,
          SLOT(emitParameterTypeChanged(const QString &)));
  connect(m_uiForm.pbAdd, SIGNAL(clicked()), this, SLOT(emitAddData()));
  connect(m_uiForm.pbClose, SIGNAL(clicked()), this, SLOT(close()));
}

std::string FunctionQAddWorkspaceDialog::workspaceName() const {
  return m_uiForm.dsWorkspace->getCurrentDataName().toStdString();
}

std::string FunctionQAddWorkspaceDialog::parameterType() const {
  return m_uiForm.cbParameterType->currentText().toStdString();
}

int FunctionQAddWorkspaceDialog::parameterNameIndex() const { return m_uiForm.cbParameterName->currentIndex(); }

void FunctionQAddWorkspaceDialog::setParameterTypes(const std::vector<std::string> &types) {
  QSignalBlocker blocker(m_uiForm.cbParameterType);
  m_uiForm.cbParameterType->clear();
  for (auto &&type : types)
    m_uiForm.cbParameterType->addItem(QString::fromStdString(type));
}

void FunctionQAddWorkspaceDialog::setParameterNames(const std::vector<std::string> &names) {
  m_uiForm.cbParameterName->clear();
  for (auto &&name : names)
    m_uiForm.cbParameterName->addItem(QString::fromStdString(name));
}

void FunctionQAddWorkspaceDialog::enableParameterSelection() {
  m_uiForm.cbParameterName->setEnabled(true);
  m_uiForm.cbParameterType->setEnabled(true);
}

void FunctionQAddWorkspaceDialog::disableParameterSelection() {
  m_uiForm.cbParameterName->setEnabled(false);
  m_uiForm.cbParameterType->setEnabled(false);
}

void FunctionQAddWorkspaceDialog::setWSSuffices(const QStringList &suffices) {
  m_uiForm.dsWorkspace->setWSSuffixes(suffices);
}

void FunctionQAddWorkspaceDialog::setFBSuffices(const QStringList &suffices) {
  m_uiForm.dsWorkspace->setFBSuffixes(suffices);
}

void FunctionQAddWorkspaceDialog::setLoadProperty(const std::string &propName, bool enable) {
  m_uiForm.dsWorkspace->setLoadProperty(propName, enable);
}

void FunctionQAddWorkspaceDialog::emitWorkspaceChanged(const QString &name) {
  m_uiForm.pbAdd->setText("Add");
  m_uiForm.pbAdd->setEnabled(true);
  emit workspaceChanged(this, name.toStdString());
}

void FunctionQAddWorkspaceDialog::emitParameterTypeChanged(const QString &type) {
  emit parameterTypeChanged(this, type.toStdString());
}
void FunctionQAddWorkspaceDialog::handleAutoLoaded() {
  m_uiForm.pbAdd->setText("Loading");
  m_uiForm.pbAdd->setEnabled(false);
}

void FunctionQAddWorkspaceDialog::emitAddData() { emit addData(this); }

} // namespace MantidQt::CustomInterfaces::Inelastic
