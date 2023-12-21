// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectSettingsView.h"
#include "IndirectSettingsPresenter.h"

#include "MantidQtWidgets/Common/HelpWindow.h"

namespace MantidQt::CustomInterfaces {

IndirectSettingsView::IndirectSettingsView(QWidget *parent)
    : QWidget(parent), m_presenter(), m_uiForm(std::make_unique<Ui::IndirectInterfaceSettings>()) {
  m_uiForm->setupUi(this);

  connect(m_uiForm->pbOk, SIGNAL(clicked()), this, SLOT(notifyOkClicked()));
  connect(m_uiForm->pbApply, SIGNAL(clicked()), this, SLOT(notifyApplyClicked()));
  connect(m_uiForm->pbCancel, SIGNAL(clicked()), this, SLOT(notifyCancelClicked()));

  connect(m_uiForm->pbHelp, SIGNAL(clicked()), this, SLOT(openHelp()));
}

QWidget *IndirectSettingsView::getView() { return this; }

void IndirectSettingsView::subscribePresenter(IndirectSettingsPresenter *presenter) { m_presenter = presenter; }

void IndirectSettingsView::notifyOkClicked() { m_presenter->notifyOkClicked(); }

void IndirectSettingsView::notifyApplyClicked() { m_presenter->notifyApplyClicked(); }

void IndirectSettingsView::notifyCancelClicked() { m_presenter->notifyCancelClicked(); }

void IndirectSettingsView::openHelp() {
  MantidQt::API::HelpWindow::showCustomInterface(QString("Indirect Settings"), QString("indirect"));
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

void IndirectSettingsView::setDeveloperFeatureFlags(QStringList const &flags) {
  m_uiForm->leDeveloperFeatureFlags->setText(flags.join(" "));
}

QStringList IndirectSettingsView::developerFeatureFlags() const {
  return m_uiForm->leDeveloperFeatureFlags->text().split(" ");
}

void IndirectSettingsView::setApplyText(QString const &text) { m_uiForm->pbApply->setText(text); }

void IndirectSettingsView::setApplyEnabled(bool enable) { m_uiForm->pbApply->setEnabled(enable); }

void IndirectSettingsView::setOkEnabled(bool enable) { m_uiForm->pbOk->setEnabled(enable); }

void IndirectSettingsView::setCancelEnabled(bool enable) { m_uiForm->pbCancel->setEnabled(enable); }

} // namespace MantidQt::CustomInterfaces
