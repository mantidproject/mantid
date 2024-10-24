// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/Spectroscopy/OutputWidget/OutputNameView.h"

#include "MantidQtWidgets/Common/WorkspaceUtils.h"
#include <MantidQtWidgets/Spectroscopy/OutputWidget/OutputNamePresenter.h>
#include <string>

namespace MantidQt::CustomInterfaces {
OutputNameView::OutputNameView(QWidget *parent) : QWidget(parent) {
  m_uiForm.setupUi(this);
  auto const validator = new QRegExpValidator(QRegExp("[a-zA-Z-_0-9]*"));
  m_uiForm.leLabel->setValidator(validator);
  connect(m_uiForm.leLabel, &QLineEdit::editingFinished, this, &OutputNameView::notifyUpdateOutputLabel);
}

void OutputNameView::subscribePresenter(IOutputNamePresenter *presenter) { m_presenter = presenter; }

void OutputNameView::enableLabelEditor() const {
  if (!m_uiForm.leLabel->isEnabled())
    m_uiForm.leLabel->setEnabled(true);
}

void OutputNameView::setWarningLabel(std::string const &text, std::string const &textColor) const {
  m_uiForm.lbWarning->setText(QString::fromStdString(text));
  m_uiForm.lbWarning->setStyleSheet(QString::fromStdString(textColor));
}

void OutputNameView::setOutputNameLabel(std::string const &text) const {
  m_uiForm.lbName->setText(QString::fromStdString(text));
  m_uiForm.lbName->setToolTip(QString::fromStdString(text));
}

std::string OutputNameView::getCurrentOutputName() const { return m_uiForm.lbName->text().toStdString(); }

std::string OutputNameView::getCurrentLabel() const { return m_uiForm.leLabel->text().toStdString(); }

void OutputNameView::notifyUpdateOutputLabel() { m_presenter->handleUpdateOutputLabel(); }

} // namespace MantidQt::CustomInterfaces
