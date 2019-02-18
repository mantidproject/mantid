// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectSettingsDialog.h"

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FacilityInfo.h"

#include <QSettings>

using namespace Mantid::Kernel;

namespace {

QString const FILTER_DATA_NAMES_SETTING("filter-input-by-name");

std::string getSavedFacility() {
  return ConfigService::Instance().getFacility().name();
}

void setSavedFacility(std::string const &facility) {
  auto const selectedFacility = getSavedFacility();
  if (selectedFacility != facility)
    ConfigService::Instance().setFacility(facility);
}

} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectSettingsDialog::IndirectSettingsDialog(QWidget *parent,
                                               QString const &settingsGroup)
    : QDialog(parent), m_settingsGroup(settingsGroup) {
  m_uiForm.setupUi(this);
  setInterfaceGroupBoxTitle(m_settingsGroup);

  connect(m_uiForm.cbFacility, SIGNAL(currentIndexChanged(QString const &)),
          this, SLOT(updateFilterInputByName(QString const &)));

  connect(m_uiForm.pbOk, SIGNAL(clicked()), this, SLOT(okClicked()));
  connect(m_uiForm.pbApply, SIGNAL(clicked()), this, SLOT(applyClicked()));
  connect(m_uiForm.pbCancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));

  loadSettings();
}

void IndirectSettingsDialog::okClicked() {
  saveSettings();
  this->close();
}

void IndirectSettingsDialog::applyClicked() {
  setApplyingChanges(true);
  saveSettings();
  setApplyingChanges(false);
}

void IndirectSettingsDialog::cancelClicked() { this->close(); }

void IndirectSettingsDialog::loadSettings() {
  setSelectedFacility(getSavedFacility());

  QSettings settings;
  settings.beginGroup(m_settingsGroup);

  auto const filter = settings.value(FILTER_DATA_NAMES_SETTING, true).toBool();

  settings.endGroup();

  setFilterInputByNameChecked(filter);

  emit updateSettings();
}

void IndirectSettingsDialog::saveSettings() {
  setSavedFacility(getSelectedFacility().toStdString());

  QSettings settings;
  settings.beginGroup(m_settingsGroup);

  settings.setValue(FILTER_DATA_NAMES_SETTING, isFilterInputByNameChecked());

  settings.endGroup();

  emit updateSettings();
}

void IndirectSettingsDialog::setInterfaceGroupBoxTitle(QString const &title) {
  m_uiForm.gbInterfaceSettings->setTitle(title);
}

void IndirectSettingsDialog::setSelectedFacility(std::string const &facility) {
  setSavedFacility(facility);
  m_uiForm.cbFacility->setCurrentIndex(findFacilityIndex(facility));
}

int IndirectSettingsDialog::findFacilityIndex(std::string const &text) {
  auto const index =
      m_uiForm.cbFacility->findText(QString::fromStdString(text));
  return index != -1 ? index : 0;
}

QString IndirectSettingsDialog::getSelectedFacility() const {
  return m_uiForm.cbFacility->currentText();
}

void IndirectSettingsDialog::updateFilterInputByName(QString const &text) {
  setFilterInputByNameChecked(text.toStdString() == "ISIS");
}

void IndirectSettingsDialog::setFilterInputByNameChecked(bool check) {
  m_uiForm.ckFilterDataNames->setChecked(check);
}

bool IndirectSettingsDialog::isFilterInputByNameChecked() const {
  return m_uiForm.ckFilterDataNames->isChecked();
}

void IndirectSettingsDialog::setApplyingChanges(bool applyingChanges) {
  setApplyText(applyingChanges ? "Applying..." : "Apply");
  setApplyEnabled(!applyingChanges);
  setOkEnabled(!applyingChanges);
  setCancelEnabled(!applyingChanges);
}

void IndirectSettingsDialog::setApplyText(QString const &text) {
  m_uiForm.pbApply->setText(text);
}

void IndirectSettingsDialog::setApplyEnabled(bool enable) {
  m_uiForm.pbApply->setEnabled(enable);
}

void IndirectSettingsDialog::setOkEnabled(bool enable) {
  m_uiForm.pbOk->setEnabled(enable);
}

void IndirectSettingsDialog::setCancelEnabled(bool enable) {
  m_uiForm.pbCancel->setEnabled(enable);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
