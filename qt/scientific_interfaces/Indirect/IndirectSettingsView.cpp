// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectSettingsView.h"

#include <QSettings>

namespace {

template <typename T>
void setQSetting(QString const &settingGroup, QString const &settingName,
                 T const &value) {
  QSettings settings;
  settings.beginGroup(settingGroup);
  settings.setValue(settingName, value);
  settings.endGroup();
}

QVariant getQSetting(QString const &settingGroup, QString const &settingName) {
  QSettings settings;
  settings.beginGroup(settingGroup);
  auto const settingValue = settings.value(settingName);
  settings.endGroup();

  return settingValue;
}

} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IndirectSettingsView::IndirectSettingsView(QWidget *parent)
    : IIndirectSettingsView(parent) {
  m_uiForm.setupUi(this);

  connect(m_uiForm.cbFacility, SIGNAL(currentIndexChanged(QString const &)),
          this, SLOT(emitUpdateRestrictInputByName(QString const &)));

  connect(m_uiForm.pbOk, SIGNAL(clicked()), this, SLOT(emitOkClicked()));
  connect(m_uiForm.pbApply, SIGNAL(clicked()), this, SLOT(emitApplyClicked()));
  connect(m_uiForm.pbCancel, SIGNAL(clicked()), this,
          SLOT(emitCancelClicked()));
}

void IndirectSettingsView::emitUpdateRestrictInputByName(QString const &text) {
  emit updateRestrictInputByName(text.toStdString());
}

void IndirectSettingsView::emitOkClicked() { emit okClicked(); }

void IndirectSettingsView::emitApplyClicked() { emit applyClicked(); }

void IndirectSettingsView::emitCancelClicked() { emit cancelClicked(); }

void IndirectSettingsView::setInterfaceSettingsVisible(bool visible) {
  m_uiForm.gbInterfaceSettings->setVisible(visible);
}

void IndirectSettingsView::setInterfaceGroupBoxTitle(QString const &title) {
  m_uiForm.gbInterfaceSettings->setTitle(title);
}

void IndirectSettingsView::setRestrictInputByNameVisible(bool visible) {
  m_uiForm.ckRestrictInputDataNames->setVisible(visible);
}

void IndirectSettingsView::setPlotErrorBarsVisible(bool visible) {
  m_uiForm.ckPlotErrorBars->setVisible(visible);
}

void IndirectSettingsView::setSelectedFacility(QString const &text) {
  auto const index = m_uiForm.cbFacility->findText(text);
  m_uiForm.cbFacility->setCurrentIndex(index != -1 ? index : 0);
}

QString IndirectSettingsView::getSelectedFacility() const {
  return m_uiForm.cbFacility->currentText();
}

void IndirectSettingsView::setRestrictInputByNameChecked(bool check) {
  m_uiForm.ckRestrictInputDataNames->setChecked(check);
}

bool IndirectSettingsView::isRestrictInputByNameChecked() const {
  return m_uiForm.ckRestrictInputDataNames->isChecked();
}

void IndirectSettingsView::setPlotErrorBarsChecked(bool check) {
  m_uiForm.ckPlotErrorBars->setChecked(check);
}

bool IndirectSettingsView::isPlotErrorBarsChecked() const {
  return m_uiForm.ckPlotErrorBars->isChecked();
}

void IndirectSettingsView::setSetting(QString const &settingsGroup,
                                      QString const &settingName,
                                      bool const &value) {
  setQSetting(settingsGroup, settingName, value);
}

QVariant IndirectSettingsView::getSetting(QString const &settingsGroup,
                                          QString const &settingName) {
  return getQSetting(settingsGroup, settingName);
}

void IndirectSettingsView::setApplyText(QString const &text) {
  m_uiForm.pbApply->setText(text);
}

void IndirectSettingsView::setApplyEnabled(bool enable) {
  m_uiForm.pbApply->setEnabled(enable);
}

void IndirectSettingsView::setOkEnabled(bool enable) {
  m_uiForm.pbOk->setEnabled(enable);
}

void IndirectSettingsView::setCancelEnabled(bool enable) {
  m_uiForm.pbCancel->setEnabled(enable);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
