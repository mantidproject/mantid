// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectSettingsPresenter.h"
#include "IndirectSettingsHelper.h"

namespace MantidQt::CustomInterfaces {
using namespace IndirectSettingsHelper;

IndirectSettingsPresenter::IndirectSettingsPresenter(std::unique_ptr<IndirectSettingsModel> model,
                                                     IIndirectSettingsView *view)
    : QObject(nullptr), m_model(std::move(model)), m_view(view) {
  m_view->subscribePresenter(this);
  loadSettings();
}

QWidget *IndirectSettingsPresenter::getView() { return m_view->getView(); }

void IndirectSettingsPresenter::notifyOkClicked() {
  saveSettings();
  emit closeSettings();
}

void IndirectSettingsPresenter::notifyApplyClicked() {
  setApplyingChanges(true);
  saveSettings();
  setApplyingChanges(false);
}

void IndirectSettingsPresenter::notifyCancelClicked() { emit closeSettings(); }

void IndirectSettingsPresenter::loadSettings() {
  m_view->setSelectedFacility(QString::fromStdString(m_model->getFacility()));

  m_view->setRestrictInputByNameChecked(restrictInputDataByName());
  m_view->setPlotErrorBarsChecked(externalPlotErrorBars());
}

void IndirectSettingsPresenter::saveSettings() {
  m_model->setFacility(m_view->getSelectedFacility().toStdString());

  setRestrictInputDataByName(m_view->isRestrictInputByNameChecked());
  setExternalPlotErrorBars(m_view->isPlotErrorBarsChecked());

  emit applySettings();
}

void IndirectSettingsPresenter::setApplyingChanges(bool applyingChanges) {
  m_view->setApplyText(applyingChanges ? "Applying..." : "Apply");
  m_view->setApplyEnabled(!applyingChanges);
  m_view->setOkEnabled(!applyingChanges);
  m_view->setCancelEnabled(!applyingChanges);
}

} // namespace MantidQt::CustomInterfaces
