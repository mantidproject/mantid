// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectSettingsPresenter.h"
#include "IndirectSettings.h"
#include "IndirectSettingsHelper.h"

namespace MantidQt::CustomInterfaces {
using namespace IndirectSettingsHelper;

IndirectSettingsPresenter::IndirectSettingsPresenter(std::unique_ptr<IndirectSettingsModel> model,
                                                     IIndirectSettingsView *view)
    : m_model(std::move(model)), m_view(view) {
  m_view->subscribePresenter(this);
  loadSettings();
}

QWidget *IndirectSettingsPresenter::getView() { return m_view->getView(); }

void IndirectSettingsPresenter::subscribeParent(IIndirectSettings *parent) { m_parent = parent; }

void IndirectSettingsPresenter::notifyOkClicked() {
  saveSettings();
  m_parent->notifyCloseSettings();
}

void IndirectSettingsPresenter::notifyApplyClicked() {
  setApplyingChanges(true);
  saveSettings();
  setApplyingChanges(false);
}

void IndirectSettingsPresenter::notifyCancelClicked() { m_parent->notifyCloseSettings(); }

void IndirectSettingsPresenter::loadSettings() {
  m_view->setSelectedFacility(QString::fromStdString(m_model->getFacility()));

  m_view->setRestrictInputByNameChecked(restrictInputDataByName());
  m_view->setPlotErrorBarsChecked(externalPlotErrorBars());

  m_view->setDeveloperFeatureFlags(developerFeatureFlags());
}

void IndirectSettingsPresenter::saveSettings() {
  m_model->setFacility(m_view->getSelectedFacility().toStdString());

  setRestrictInputDataByName(m_view->isRestrictInputByNameChecked());
  setExternalPlotErrorBars(m_view->isPlotErrorBarsChecked());

  setDeveloperFeatureFlags(m_view->developerFeatureFlags());

  m_parent->notifyApplySettings();
}

void IndirectSettingsPresenter::setApplyingChanges(bool applyingChanges) {
  m_view->setApplyText(applyingChanges ? "Applying..." : "Apply");
  m_view->setApplyEnabled(!applyingChanges);
  m_view->setOkEnabled(!applyingChanges);
  m_view->setCancelEnabled(!applyingChanges);
}

} // namespace MantidQt::CustomInterfaces
