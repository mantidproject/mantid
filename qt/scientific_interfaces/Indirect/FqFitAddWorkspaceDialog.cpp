// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "FqFitAddWorkspaceDialog.h"

#include <boost/optional.hpp>

#include "MantidQtWidgets/Common/SignalBlocker.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

FqFitAddWorkspaceDialog::FqFitAddWorkspaceDialog(QWidget *parent) : IAddWorkspaceDialog(parent) {
  m_uiForm.setupUi(this);

  connect(m_uiForm.dsWorkspace, SIGNAL(dataReady(const QString &)), this, SLOT(emitWorkspaceChanged(const QString &)));
  connect(m_uiForm.cbParameterType, SIGNAL(currentIndexChanged(const QString &)), this,
          SLOT(emitParameterTypeChanged(const QString &)));
  connect(m_uiForm.pbAdd, SIGNAL(clicked()), this, SIGNAL(addData()));
  connect(m_uiForm.pbClose, SIGNAL(clicked()), this, SIGNAL(closeDialog()));
}

std::string FqFitAddWorkspaceDialog::workspaceName() const {
  return m_uiForm.dsWorkspace->getCurrentDataName().toStdString();
}

std::string FqFitAddWorkspaceDialog::parameterType() const {
  return m_uiForm.cbParameterType->currentText().toStdString();
}

int FqFitAddWorkspaceDialog::parameterNameIndex() const { return m_uiForm.cbParameterName->currentIndex(); }

void FqFitAddWorkspaceDialog::setParameterTypes(const std::vector<std::string> &types) {
  MantidQt::API::SignalBlocker blocker(m_uiForm.cbParameterType);
  m_uiForm.cbParameterType->clear();
  for (auto &&type : types)
    m_uiForm.cbParameterType->addItem(QString::fromStdString(type));
}

void FqFitAddWorkspaceDialog::setParameterNames(const std::vector<std::string> &names) {
  m_uiForm.cbParameterName->clear();
  for (auto &&name : names)
    m_uiForm.cbParameterName->addItem(QString::fromStdString(name));
}

void FqFitAddWorkspaceDialog::enableParameterSelection() {
  m_uiForm.cbParameterName->setEnabled(true);
  m_uiForm.cbParameterType->setEnabled(true);
}

void FqFitAddWorkspaceDialog::disableParameterSelection() {
  m_uiForm.cbParameterName->setEnabled(false);
  m_uiForm.cbParameterType->setEnabled(false);
}

void FqFitAddWorkspaceDialog::setWSSuffices(const QStringList &suffices) {
  m_uiForm.dsWorkspace->setWSSuffixes(suffices);
}

void FqFitAddWorkspaceDialog::setFBSuffices(const QStringList &suffices) {
  m_uiForm.dsWorkspace->setFBSuffixes(suffices);
}

void FqFitAddWorkspaceDialog::emitWorkspaceChanged(const QString &name) {
  emit workspaceChanged(this, name.toStdString());
}

void FqFitAddWorkspaceDialog::emitParameterTypeChanged(const QString &type) {
  emit parameterTypeChanged(this, type.toStdString());
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
