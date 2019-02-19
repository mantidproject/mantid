// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectSettingsPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectSettingsPresenter::IndirectSettingsPresenter(
    QWidget *parent, std::string const &settingsGroup,
    std::string const &availableSettings)
    : QObject(nullptr),
      m_view(Mantid::Kernel::make_unique<IndirectSettingsView>(parent)),
      m_model(Mantid::Kernel::make_unique<IndirectSettingsModel>(
          settingsGroup, availableSettings)) {

  connect(m_view.get(), SIGNAL(updateRestrictInputByName(std::string const &)),
          this, SLOT(updateRestrictInputByName(std::string const &)));

  connect(m_view.get(), SIGNAL(okClicked()), this,
          SLOT(applyAndCloseSettings()));
  connect(m_view.get(), SIGNAL(applyClicked()), this, SLOT(applySettings()));
  connect(m_view.get(), SIGNAL(cancelClicked()), this, SLOT(closeDialog()));

  initLayout();
  loadSettings();
}

void IndirectSettingsPresenter::showDialog() { m_view->show(); }

void IndirectSettingsPresenter::initLayout() {
  m_view->setInterfaceSettingsVisible(m_model->hasInterfaceSettings());
  m_view->setInterfaceGroupBoxTitle(
      QString::fromStdString(m_model->getSettingsGroup()));

  m_view->setRestrictInputByNameVisible(
      m_model->isSettingAvailable("filter-input-by-name"));
}

void IndirectSettingsPresenter::applyAndCloseSettings() {
  saveSettings();
  closeDialog();
}

void IndirectSettingsPresenter::applySettings() {
  setApplyingChanges(true);
  saveSettings();
  setApplyingChanges(false);
}

void IndirectSettingsPresenter::closeDialog() { m_view->close(); }

void IndirectSettingsPresenter::loadSettings() {
  m_view->setSelectedFacility(QString::fromStdString(m_model->getFacility()));

  if (m_model->isSettingAvailable("filter-input-by-name"))
    loadRestrictInputSetting(m_model->getSettingsGroup());

  emit applySettings();
}

void IndirectSettingsPresenter::loadRestrictInputSetting(
    std::string const &settingsGroup) {
  auto const filter = m_view->getSetting(QString::fromStdString(settingsGroup),
                                         "filter-input-by-name");
  m_view->setRestrictInputByNameChecked(filter.toBool());
}

void IndirectSettingsPresenter::saveSettings() {
  m_model->setFacility(m_view->getSelectedFacility().toStdString());

  if (m_model->isSettingAvailable("filter-input-by-name"))
    m_view->setSetting(QString::fromStdString(m_model->getSettingsGroup()),
                       "filter-input-by-name",
                       m_view->isRestrictInputByNameChecked());

  emit applySettings();
}

void IndirectSettingsPresenter::updateRestrictInputByName(
    std::string const &text) {
  m_view->setRestrictInputByNameChecked(text == "ISIS");
}

void IndirectSettingsPresenter::setApplyingChanges(bool applyingChanges) {
  m_view->setApplyText(applyingChanges ? "Applying..." : "Apply");
  m_view->setApplyEnabled(!applyingChanges);
  m_view->setOkEnabled(!applyingChanges);
  m_view->setCancelEnabled(!applyingChanges);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
