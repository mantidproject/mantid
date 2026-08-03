// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Spectroscopy/SettingsWidget/SettingsPresenter.h"
#include "MantidQtWidgets/Spectroscopy/SettingsWidget/Settings.h"
#include "MantidQtWidgets/Spectroscopy/SettingsWidget/SettingsHelper.h"

namespace MantidQt::CustomInterfaces {
using namespace SettingsHelper;

SettingsPresenter::SettingsPresenter(std::unique_ptr<SettingsModel> model, ISettingsView *view)
    : m_model(std::move(model)), m_view(view) {
  m_view->subscribePresenter(this);
  restoreSettings(SettingsHelper::readSettings());
}

QWidget *SettingsPresenter::getView() { return m_view->getView(); }

void SettingsPresenter::subscribeParent(ISettings *parent) { m_parent = parent; }

void SettingsPresenter::notifyOkClicked() {
  saveSettings();
  m_parent->notifyCloseSettings();
}

void SettingsPresenter::notifyApplyClicked() {
  setApplyingChanges(true);
  saveSettings();
  setApplyingChanges(false);
}

void SettingsPresenter::notifyCancelClicked() { m_parent->notifyCloseSettings(); }

void SettingsPresenter::restoreSettings(const SettingsHelper::SpectroscopySettings &settings) {
  m_view->setSelectedFacility(QString::fromStdString(m_model->getFacility()));
  m_view->setRestrictInputByNameChecked(settings.restrictInputByName());
  m_view->setPlotErrorBarsChecked(settings.externalPlotErrorBars());
  m_view->setLoadHistoryChecked(settings.loadHistory());
  m_view->setDeveloperFeatureFlags(settings.developerFeatureFlags());
}

SettingsHelper::SpectroscopySettings SettingsPresenter::captureSettings() const {
  return SettingsHelper::SpectroscopySettings(m_view->isRestrictInputByNameChecked(), m_view->isPlotErrorBarsChecked(),
                                              m_view->isLoadHistoryChecked(), m_view->developerFeatureFlags());
}

void SettingsPresenter::saveSettings() {
  m_model->setFacility(m_view->getSelectedFacility().toStdString());

  SettingsHelper::saveSettings(captureSettings());

  m_parent->notifyApplySettings();
}

void SettingsPresenter::setApplyingChanges(bool applyingChanges) {
  m_view->setApplyText(applyingChanges ? "Applying..." : "Apply");
  m_view->setApplyEnabled(!applyingChanges);
  m_view->setOkEnabled(!applyingChanges);
  m_view->setCancelEnabled(!applyingChanges);
}

} // namespace MantidQt::CustomInterfaces
