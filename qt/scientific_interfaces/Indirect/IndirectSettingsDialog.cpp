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
                                               QString const &settingGroup)
    : QDialog(parent), m_settingsGroup(settingGroup) {
  m_uiForm.setupUi(this);
  m_uiForm.iicInstrumentConfiguration->setShowInstrumentLabel(true);

  loadProperties();

  connect(m_uiForm.pbOk, SIGNAL(clicked()), this, SLOT(okClicked()));
  connect(m_uiForm.pbApply, SIGNAL(clicked()), this, SLOT(applyClicked()));
  connect(m_uiForm.pbCancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));

  // connect(m_uiForm.iicInstrumentConfiguration,
  //        SIGNAL(instrumentConfigurationUpdated(
  //            QString const &, QString const &, QString const &)),
  //        this,
  //        SIGNAL(instrumentSetupChanged(QString const &, QString const &,
  //                                      QString const &)));
}

void IndirectSettingsDialog::okClicked() {
  saveProperties();
  emit instrumentSetupChanged(getSelectedInstrument(), getSelectedAnalyser(),
                              getSelectedReflection());
  this->close();
}

void IndirectSettingsDialog::applyClicked() {
  setApplyingChanges(true);
  saveProperties();
  emit instrumentSetupChanged(getSelectedInstrument(), getSelectedAnalyser(),
                              getSelectedReflection());
  setApplyingChanges(false);
}

void IndirectSettingsDialog::cancelClicked() { this->close(); }

void IndirectSettingsDialog::loadProperties() {
  setSelectedFacility(getSavedFacility());

  QSettings settings;
  settings.beginGroup(m_settingsGroup);

  auto const instrumentName = settings.value("instrument-name", "").toString();
  auto const analyserName = settings.value("analyser-name", "").toString();
  auto const reflectionName = settings.value("reflection-name", "").toString();

  if (!instrumentName.isEmpty())
    setSelectedInstrument(instrumentName);
  if (!analyserName.isEmpty())
    setSelectedAnalyser(analyserName);
  if (!reflectionName.isEmpty())
    setSelectedReflection(reflectionName);

  settings.endGroup();
}

void IndirectSettingsDialog::saveProperties() {
  setSavedFacility(getSelectedFacility().toStdString());

  QSettings settings;
  settings.beginGroup(m_settingsGroup);

  settings.setValue("instrument-name", getSelectedInstrument());
  settings.setValue("analyser-name", getSelectedAnalyser());
  settings.setValue("reflection-name", getSelectedReflection());

  settings.endGroup();
}

void IndirectSettingsDialog::setSelectedFacility(std::string const &facility) {
  setSavedFacility(facility);
  m_uiForm.iicInstrumentConfiguration->setFacility(
      QString::fromStdString(facility));
  m_uiForm.cbFacility->setCurrentIndex(findFacilityIndex(facility));
}

int IndirectSettingsDialog::findFacilityIndex(std::string const &text) {
  auto const index =
      m_uiForm.cbFacility->findText(QString::fromStdString(text));
  return index != -1 ? index : 0;
}

void IndirectSettingsDialog::setSelectedInstrument(QString const &instrument) {
  m_uiForm.iicInstrumentConfiguration->setInstrument(instrument);
}

void IndirectSettingsDialog::setSelectedAnalyser(QString const &analyser) {
  m_uiForm.iicInstrumentConfiguration->setAnalyser(analyser);
}

void IndirectSettingsDialog::setSelectedReflection(QString const &reflection) {
  m_uiForm.iicInstrumentConfiguration->setReflection(reflection);
}

QString IndirectSettingsDialog::getSelectedFacility() const {
  return m_uiForm.cbFacility->currentText();
}

QString IndirectSettingsDialog::getSelectedInstrument() const {
  return m_uiForm.iicInstrumentConfiguration->getInstrumentName();
}

QString IndirectSettingsDialog::getSelectedAnalyser() const {
  return m_uiForm.iicInstrumentConfiguration->getAnalyserName();
}

QString IndirectSettingsDialog::getSelectedReflection() const {
  return m_uiForm.iicInstrumentConfiguration->getReflectionName();
}

void IndirectSettingsDialog::setDisabledInstruments(
    QStringList const &instrumentNames) {
  m_uiForm.iicInstrumentConfiguration->setDisabledInstruments(instrumentNames);
}

void IndirectSettingsDialog::updateInstrumentConfiguration() {
  m_uiForm.iicInstrumentConfiguration->newInstrumentConfiguration();
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
