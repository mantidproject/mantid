// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "JumpFitAddWorkspaceDialog.h"

#include <boost/optional.hpp>

#include "MantidQtWidgets/Common/SignalBlocker.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

JumpFitAddWorkspaceDialog::JumpFitAddWorkspaceDialog(QWidget *parent)
    : IAddWorkspaceDialog(parent) {
  m_uiForm.setupUi(this);

  connect(m_uiForm.dsWorkspace, SIGNAL(dataReady(const QString &)), this,
          SLOT(emitWorkspaceChanged(const QString &)));
  connect(m_uiForm.cbParameterType,
          SIGNAL(currentIndexChanged(const QString &)), this,
          SLOT(emitParameterTypeChanged(const QString &)));
  connect(m_uiForm.pbAdd, SIGNAL(clicked()), this, SIGNAL(addData()));
  connect(m_uiForm.pbClose, SIGNAL(clicked()), this, SIGNAL(closeDialog()));
}

std::string JumpFitAddWorkspaceDialog::workspaceName() const {
  return m_uiForm.dsWorkspace->getCurrentDataName().toStdString();
}

std::string JumpFitAddWorkspaceDialog::parameterType() const {
  return m_uiForm.cbParameterType->currentText().toStdString();
}

int JumpFitAddWorkspaceDialog::parameterNameIndex() const {
  return m_uiForm.cbParameterName->currentIndex();
}

void JumpFitAddWorkspaceDialog::setParameterTypes(
    const std::vector<std::string> &types) {
  MantidQt::API::SignalBlocker<QObject> blocker(m_uiForm.cbParameterType);
  m_uiForm.cbParameterType->clear();
  for (auto &&type : types)
    m_uiForm.cbParameterType->addItem(QString::fromStdString(type));
}

void JumpFitAddWorkspaceDialog::setParameterNames(
    const std::vector<std::string> &names) {
  m_uiForm.cbParameterName->clear();
  for (auto &&name : names)
    m_uiForm.cbParameterName->addItem(QString::fromStdString(name));
}

void JumpFitAddWorkspaceDialog::enableParameterSelection() {
  m_uiForm.cbParameterName->setEnabled(true);
  m_uiForm.cbParameterType->setEnabled(true);
}

void JumpFitAddWorkspaceDialog::disableParameterSelection() {
  m_uiForm.cbParameterName->setEnabled(false);
  m_uiForm.cbParameterType->setEnabled(false);
}

void JumpFitAddWorkspaceDialog::setWSSuffices(const QStringList &suffices) {
  m_uiForm.dsWorkspace->setWSSuffixes(suffices);
}

void JumpFitAddWorkspaceDialog::setFBSuffices(const QStringList &suffices) {
  m_uiForm.dsWorkspace->setFBSuffixes(suffices);
}

void JumpFitAddWorkspaceDialog::emitWorkspaceChanged(const QString &name) {
  emit workspaceChanged(this, name.toStdString());
}

void JumpFitAddWorkspaceDialog::emitParameterTypeChanged(const QString &type) {
  emit parameterTypeChanged(this, type.toStdString());
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
