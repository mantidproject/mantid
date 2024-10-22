// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/Spectroscopy/OutputWidget/OutputName.h"

#include "MantidQtWidgets/Common/WorkspaceUtils.h"
#include <string>

namespace {
QString addLabelUnderscore(QString const &label) { return label.isEmpty() ? "" : "_" + label; }
} // namespace

namespace MantidQt::CustomInterfaces {
OutputName::OutputName(QWidget *parent) : QWidget(parent), m_suffixes(), m_currBasename() {
  m_uiForm.setupUi(this);
  auto const validator = new QRegExpValidator(QRegExp("[a-zA-Z-_0-9]*"));
  m_uiForm.leLabel->setValidator(validator);
  connect(m_uiForm.leLabel, &QLineEdit::editingFinished, this, &OutputName::updateOutputLabel);
}

void OutputName::setWsSuffixes(QStringList const &suffixes) { m_suffixes = suffixes; }

void OutputName::setOutputWsBasename(std::string const &outputBasename, std::string const &outputSuffix) {
  if (!m_uiForm.leLabel->isEnabled())
    m_uiForm.leLabel->setEnabled(true);
  m_currBasename = QString::fromStdString(outputBasename);
  m_currOutputSuffix = QString::fromStdString(outputSuffix);
  updateOutputLabel();
}

int OutputName::findInsertIndexLabel(QString const &outputBasename) {
  std::vector<int> indexes;
  std::transform(m_suffixes.begin(), m_suffixes.end(), std::back_inserter(indexes),
                 [&](auto const &suffix) { return outputBasename.lastIndexOf(suffix); });
  auto max_index = (*std::max_element(indexes.cbegin(), indexes.cend()));
  return (indexes.empty() || (max_index == -1)) ? outputBasename.length() : max_index;
}

void OutputName::generateLabelWarning() const {
  auto textColor = QString("color: darkRed");
  if (MantidWidgets::WorkspaceUtils::doesExistInADS(m_uiForm.lbName->text().toStdString())) {
    m_uiForm.lbWarning->setText("Output Name is in use, workspace will be overriden.");
  } else {
    m_uiForm.lbWarning->setText("Unused name, new workspace will be created.");
    textColor = QString("color: darkGreen");
  }
  m_uiForm.lbWarning->setStyleSheet(textColor);
}

std::string OutputName::getCurrentLabel() const { return m_uiForm.lbName->text().toStdString(); }

std::string OutputName::generateOutputLabel() {
  auto outputName = m_currBasename;
  return outputName.insert(findInsertIndexLabel(outputName), addLabelUnderscore(m_uiForm.leLabel->text()))
      .toStdString();
}

void OutputName::updateOutputLabel() {
  auto labelName = m_currBasename;
  if (!m_uiForm.leLabel->text().isEmpty())
    labelName.insert(findInsertIndexLabel(labelName), "_" + m_uiForm.leLabel->text());
  labelName += m_currOutputSuffix;
  m_uiForm.lbName->setText(labelName);
  m_uiForm.lbName->setToolTip(labelName);
  generateLabelWarning();
}
} // namespace MantidQt::CustomInterfaces
