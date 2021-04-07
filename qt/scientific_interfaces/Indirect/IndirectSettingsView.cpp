// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectSettingsView.h"

#include "MantidQtWidgets/Common/HelpWindow.h"

namespace MantidQt {
namespace CustomInterfaces {

IndirectSettingsView::IndirectSettingsView(QWidget *parent)
    : IIndirectSettingsView(parent), m_uiForm(std::make_unique<Ui::IndirectInterfaceSettings>()) {
  m_uiForm->setupUi(this);

  connect(m_uiForm->pbOk, SIGNAL(clicked()), this, SLOT(emitOkClicked()));
  connect(m_uiForm->pbApply, SIGNAL(clicked()), this, SLOT(emitApplyClicked()));
  connect(m_uiForm->pbCancel, SIGNAL(clicked()), this, SLOT(emitCancelClicked()));

  connect(m_uiForm->pbHelp, SIGNAL(clicked()), this, SLOT(openHelp()));
}

void IndirectSettingsView::emitOkClicked() { emit okClicked(); }

void IndirectSettingsView::emitApplyClicked() { emit applyClicked(); }

void IndirectSettingsView::emitCancelClicked() { emit cancelClicked(); }

void IndirectSettingsView::openHelp() {
  MantidQt::API::HelpWindow::showCustomInterface(nullptr, QString("Indirect Settings"), QString("indirect"));
}

void IndirectSettingsView::setSelectedFacility(QString const &text) {
  auto const index = m_uiForm->cbFacility->findText(text);
  m_uiForm->cbFacility->setCurrentIndex(index != -1 ? index : 0);
}

QString IndirectSettingsView::getSelectedFacility() const { return m_uiForm->cbFacility->currentText(); }

void IndirectSettingsView::setRestrictInputByNameChecked(bool check) {
  m_uiForm->ckRestrictInputDataNames->setChecked(check);
}

bool IndirectSettingsView::isRestrictInputByNameChecked() const {
  return m_uiForm->ckRestrictInputDataNames->isChecked();
}

void IndirectSettingsView::setPlotErrorBarsChecked(bool check) { m_uiForm->ckPlotErrorBars->setChecked(check); }

bool IndirectSettingsView::isPlotErrorBarsChecked() const { return m_uiForm->ckPlotErrorBars->isChecked(); }

void IndirectSettingsView::setApplyText(QString const &text) { m_uiForm->pbApply->setText(text); }

void IndirectSettingsView::setApplyEnabled(bool enable) { m_uiForm->pbApply->setEnabled(enable); }

void IndirectSettingsView::setOkEnabled(bool enable) { m_uiForm->pbOk->setEnabled(enable); }

void IndirectSettingsView::setCancelEnabled(bool enable) { m_uiForm->pbCancel->setEnabled(enable); }

} // namespace CustomInterfaces
} // namespace MantidQt
