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
    : QObject(nullptr), m_model(std::make_unique<IndirectSettingsModel>(
                            settingsGroup, availableSettings)),
      m_view(std::make_unique<IndirectSettingsView>(parent)) {
  setUpPresenter();
}

IndirectSettingsPresenter::IndirectSettingsPresenter(
    IndirectSettingsModel *model, IIndirectSettingsView *view)
    : QObject(nullptr), m_model(model), m_view(view) {
  setUpPresenter();
}

void IndirectSettingsPresenter::setUpPresenter() {
  connect(m_view.get(), SIGNAL(updateRestrictInputByName(std::string const &)),
          this, SLOT(updateRestrictInputByName(std::string const &)));

  connect(m_view.get(), SIGNAL(okClicked()), this,
          SLOT(applyAndCloseSettings()));
  connect(m_view.get(), SIGNAL(applyClicked()), this, SLOT(applyChanges()));
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
      m_model->isSettingAvailable("restrict-input-by-name"));
  m_view->setPlotErrorBarsVisible(
      m_model->isSettingAvailable("plot-error-bars"));
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

void IndirectSettingsPresenter::closeDialog() { m_view->close(); }

void IndirectSettingsPresenter::loadSettings() {
  m_view->setSelectedFacility(QString::fromStdString(m_model->getFacility()));

  if (m_model->isSettingAvailable("restrict-input-by-name"))
    m_view->setRestrictInputByNameChecked(
        getSetting("restrict-input-by-name").toBool());

  if (m_model->isSettingAvailable("plot-error-bars"))
    m_view->setPlotErrorBarsChecked(getSetting("plot-error-bars").toBool());

  emit applySettings();
}

QVariant IndirectSettingsPresenter::getSetting(std::string const &settingName) {
  return m_view->getSetting(QString::fromStdString(m_model->getSettingsGroup()),
                            QString::fromStdString(settingName));
}

void IndirectSettingsPresenter::saveSettings() {
  auto const group = QString::fromStdString(m_model->getSettingsGroup());

  m_model->setFacility(m_view->getSelectedFacility().toStdString());

  if (m_model->isSettingAvailable("restrict-input-by-name"))
    m_view->setSetting(group, "restrict-input-by-name",
                       m_view->isRestrictInputByNameChecked());

  if (m_model->isSettingAvailable("plot-error-bars"))
    m_view->setSetting(group, "plot-error-bars",
                       m_view->isPlotErrorBarsChecked());

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
