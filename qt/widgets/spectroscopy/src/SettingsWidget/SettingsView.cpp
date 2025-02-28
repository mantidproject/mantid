// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Spectroscopy/SettingsWidget/SettingsView.h"
#include "MantidQtWidgets/Spectroscopy/SettingsWidget/SettingsPresenter.h"

#include "MantidQtWidgets/Common/HelpWindow.h"

namespace MantidQt::CustomInterfaces {

SettingsView::SettingsView(QWidget *parent)
    : QWidget(parent), m_presenter(), m_uiForm(std::make_unique<Ui::InterfaceSettings>()) {
  m_uiForm->setupUi(this);

  connect(m_uiForm->pbOk, &QPushButton::clicked, this, &SettingsView::notifyOkClicked);
  connect(m_uiForm->pbApply, &QPushButton::clicked, this, &SettingsView::notifyApplyClicked);
  connect(m_uiForm->pbCancel, &QPushButton::clicked, this, &SettingsView::notifyCancelClicked);
  connect(m_uiForm->pbHelp, &QPushButton::clicked, this, &SettingsView::openHelp);
}

QWidget *SettingsView::getView() { return this; }

void SettingsView::subscribePresenter(SettingsPresenter *presenter) { m_presenter = presenter; }

void SettingsView::notifyOkClicked() { m_presenter->notifyOkClicked(); }

void SettingsView::notifyApplyClicked() { m_presenter->notifyApplyClicked(); }

void SettingsView::notifyCancelClicked() { m_presenter->notifyCancelClicked(); }

void SettingsView::openHelp() {
  MantidQt::API::HelpWindow::showCustomInterface(QString("Indirect Settings"), QString("indirect"));
}

void SettingsView::setSelectedFacility(QString const &text) {
  auto const index = m_uiForm->cbFacility->findText(text);
  m_uiForm->cbFacility->setCurrentIndex(index != -1 ? index : 0);
}

QString SettingsView::getSelectedFacility() const { return m_uiForm->cbFacility->currentText(); }

void SettingsView::setRestrictInputByNameChecked(bool check) { m_uiForm->ckRestrictInputDataNames->setChecked(check); }

bool SettingsView::isRestrictInputByNameChecked() const { return m_uiForm->ckRestrictInputDataNames->isChecked(); }

void SettingsView::setPlotErrorBarsChecked(bool check) { m_uiForm->ckPlotErrorBars->setChecked(check); }

bool SettingsView::isPlotErrorBarsChecked() const { return m_uiForm->ckPlotErrorBars->isChecked(); }

void SettingsView::setLoadHistoryChecked(bool check) { m_uiForm->ckLoadHistory->setChecked(check); }

bool SettingsView::isLoadHistoryChecked() const { return m_uiForm->ckLoadHistory->isChecked(); }

void SettingsView::setDeveloperFeatureFlags(QStringList const &flags) {
  m_uiForm->leDeveloperFeatureFlags->setText(flags.join(" "));
}

QStringList SettingsView::developerFeatureFlags() const { return m_uiForm->leDeveloperFeatureFlags->text().split(" "); }

void SettingsView::setApplyText(QString const &text) { m_uiForm->pbApply->setText(text); }

void SettingsView::setApplyEnabled(bool enable) { m_uiForm->pbApply->setEnabled(enable); }

void SettingsView::setOkEnabled(bool enable) { m_uiForm->pbOk->setEnabled(enable); }

void SettingsView::setCancelEnabled(bool enable) { m_uiForm->pbCancel->setEnabled(enable); }

} // namespace MantidQt::CustomInterfaces
