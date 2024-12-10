// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ConvolutionAddWorkspaceDialog.h"

#include "FitTabConstants.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Common/TableWidgetValidators.h"
#include "MantidQtWidgets/Common/WorkspaceUtils.h"
#include "MantidQtWidgets/Spectroscopy/InterfaceUtils.h"
#include <qcheckbox.h>
#include <utility>

using namespace MantidQt::MantidWidgets::WorkspaceUtils;
using namespace MantidQt::MantidWidgets;

namespace MantidQt::CustomInterfaces::Inelastic {

ConvolutionAddWorkspaceDialog::ConvolutionAddWorkspaceDialog(QWidget *parent) : QDialog(parent) {
  m_uiForm.setupUi(this);
  const auto validatorString = QString::fromStdString(getRegexValidatorString(RegexValidatorStrings::SpectraValidator));
  m_uiForm.leWorkspaceIndices->setValidator(new QRegExpValidator(QRegExp(validatorString), this));
  setAllSpectraSelectionEnabled(false);

  connect(m_uiForm.dsWorkspace, &DataSelector::dataReady, this, &ConvolutionAddWorkspaceDialog::workspaceChanged);
  connect(m_uiForm.ckAllSpectra, &QCheckBox::stateChanged, this, &ConvolutionAddWorkspaceDialog::selectAllSpectra);
  connect(m_uiForm.pbAdd, &QPushButton::clicked, this, &ConvolutionAddWorkspaceDialog::emitAddData);
  connect(m_uiForm.pbClose, &QPushButton::clicked, this, &ConvolutionAddWorkspaceDialog::close);
}

std::string ConvolutionAddWorkspaceDialog::workspaceName() const {
  return m_uiForm.dsWorkspace->getCurrentDataName().toStdString();
}

std::string ConvolutionAddWorkspaceDialog::resolutionName() const {
  return m_uiForm.dsResolution->getCurrentDataName().toStdString();
}

MantidWidgets::FunctionModelSpectra ConvolutionAddWorkspaceDialog::workspaceIndices() const {
  return MantidWidgets::FunctionModelSpectra(m_uiForm.leWorkspaceIndices->text().toStdString());
}

void ConvolutionAddWorkspaceDialog::setWSSuffices(const QStringList &suffices) {
  m_uiForm.dsWorkspace->setWSSuffixes(suffices);
}

void ConvolutionAddWorkspaceDialog::setFBSuffices(const QStringList &suffices) {
  m_uiForm.dsWorkspace->setFBSuffixes(suffices);
}

void ConvolutionAddWorkspaceDialog::setLoadProperty(const std::string &propName, bool enable) {
  m_uiForm.dsWorkspace->setLoadProperty(propName, enable);
  m_uiForm.dsResolution->setLoadProperty(propName, enable);
}

void ConvolutionAddWorkspaceDialog::setResolutionWSSuffices(const QStringList &suffices) {
  m_uiForm.dsResolution->setWSSuffixes(suffices);
}

void ConvolutionAddWorkspaceDialog::setResolutionFBSuffices(const QStringList &suffices) {
  m_uiForm.dsResolution->setFBSuffixes(suffices);
}

void ConvolutionAddWorkspaceDialog::updateSelectedSpectra() {
  auto const state = m_uiForm.ckAllSpectra->isChecked() ? Qt::Checked : Qt::Unchecked;
  selectAllSpectra(state);
}

void ConvolutionAddWorkspaceDialog::selectAllSpectra(int state) {
  auto const name = workspaceName();
  if (doesExistInADS(name) && state == Qt::Checked) {
    m_uiForm.leWorkspaceIndices->setText(QString::fromStdString(getIndexString(name)));
    m_uiForm.leWorkspaceIndices->setEnabled(false);
  } else
    m_uiForm.leWorkspaceIndices->setEnabled(true);
}

void ConvolutionAddWorkspaceDialog::workspaceChanged(const QString &workspaceName) {
  const auto name = workspaceName.toStdString();
  const auto workspace = getADSWorkspace(name);
  if (workspace)
    setWorkspace(name);
  else
    setAllSpectraSelectionEnabled(false);
}

void ConvolutionAddWorkspaceDialog::emitAddData() { emit addData(this); }

void ConvolutionAddWorkspaceDialog::setWorkspace(const std::string &workspace) {
  setAllSpectraSelectionEnabled(true);
  if (m_uiForm.ckAllSpectra->isChecked()) {
    m_uiForm.leWorkspaceIndices->setText(QString::fromStdString(getIndexString(workspace)));
    m_uiForm.leWorkspaceIndices->setEnabled(false);
  }
}

void ConvolutionAddWorkspaceDialog::setAllSpectraSelectionEnabled(bool doEnable) {
  m_uiForm.ckAllSpectra->setEnabled(doEnable);
}

} // namespace MantidQt::CustomInterfaces::Inelastic
