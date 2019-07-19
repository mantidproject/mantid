// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectSettingsPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {

IndirectSettingsPresenter::IndirectSettingsPresenter(QWidget *parent)
    : QObject(nullptr), m_model(std::make_unique<IndirectSettingsModel>()),
      m_view(std::make_unique<IndirectSettingsView>(parent)) {
  setUpPresenter();
}

IndirectSettingsPresenter::IndirectSettingsPresenter(
    IndirectSettingsModel *model, IIndirectSettingsView *view)
    : QObject(nullptr), m_model(model), m_view(view) {
  setUpPresenter();
}

void IndirectSettingsPresenter::setUpPresenter() {
  connect(m_view.get(), SIGNAL(okClicked()), this,
          SLOT(applyAndCloseSettings()));
  connect(m_view.get(), SIGNAL(applyClicked()), this, SLOT(applyChanges()));
  connect(m_view.get(), SIGNAL(cancelClicked()), this, SLOT(closeDialog()));

  loadSettings();

  // Temporary until better validation is used when loading data into interfaces
  setDefaultRestrictData();
}

void IndirectSettingsPresenter::setDefaultRestrictData() const {
  auto const settingsGroup =
      QString::fromStdString(m_model->getSettingsGroup());
  auto const isisFacility = m_model->getFacility() == "ISIS";
  if (isisFacility)
    m_view->setSetting(settingsGroup, "restrict-input-by-name", isisFacility);
}

IIndirectSettingsView *IndirectSettingsPresenter::getView() {
  return m_view.get();
}

void IndirectSettingsPresenter::applyAndCloseSettings() {
  saveSettings();
  closeDialog();
}

void IndirectSettingsPresenter::applyChanges() {
  setApplyingChanges(true);
  saveSettings();
  setApplyingChanges(false);
}

void IndirectSettingsPresenter::closeDialog() { emit closeSettings(); }

void IndirectSettingsPresenter::loadSettings() {
  m_view->setSelectedFacility(QString::fromStdString(m_model->getFacility()));

  m_view->setRestrictInputByNameChecked(
      getSetting("restrict-input-by-name").toBool());
  m_view->setPlotErrorBarsChecked(getSetting("plot-error-bars").toBool());
}

QVariant IndirectSettingsPresenter::getSetting(std::string const &settingName) {
  return m_view->getSetting(QString::fromStdString(m_model->getSettingsGroup()),
                            QString::fromStdString(settingName));
}

void IndirectSettingsPresenter::saveSettings() {
  auto const settingsGroup =
      QString::fromStdString(m_model->getSettingsGroup());

  m_model->setFacility(m_view->getSelectedFacility().toStdString());

  m_view->setSetting(settingsGroup, "restrict-input-by-name",
                     m_view->isRestrictInputByNameChecked());
  m_view->setSetting(settingsGroup, "plot-error-bars",
                     m_view->isPlotErrorBarsChecked());

  emit applySettings();
}

void IndirectSettingsPresenter::setApplyingChanges(bool applyingChanges) {
  m_view->setApplyText(applyingChanges ? "Applying..." : "Apply");
  m_view->setApplyEnabled(!applyingChanges);
  m_view->setOkEnabled(!applyingChanges);
  m_view->setCancelEnabled(!applyingChanges);
}

} // namespace CustomInterfaces
} // namespace MantidQt
