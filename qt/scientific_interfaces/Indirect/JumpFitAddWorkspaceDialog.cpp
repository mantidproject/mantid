#include "JumpFitAddWorkspaceDialog.h"

#include <boost/optional.hpp>

#include "MantidQtWidgets/Common/SignalBlocker.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

JumpFitAddWorkspaceDialog::JumpFitAddWorkspaceDialog(QWidget *parent)
    : IAddWorkspaceDialog(parent) {
  m_uiForm.setupUi(this);
  m_uiForm.dsWorkspace->showWorkspaceGroups(false);

  connect(m_uiForm.dsWorkspace, SIGNAL(dataReady(const QString &)), this,
          SLOT(emitWorkspaceChanged(const QString &)));
  connect(m_uiForm.cbParameterType, SIGNAL(currentIndexChanged(int)), this,
          SLOT(emitParameterTypeChanged(int)));
}

std::string JumpFitAddWorkspaceDialog::workspaceName() const {
  return m_uiForm.dsWorkspace->getCurrentDataName().toStdString();
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

void JumpFitAddWorkspaceDialog::emitParameterTypeChanged(int type) {
  emit parameterTypeChanged(this, type);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
