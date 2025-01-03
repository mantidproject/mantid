// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "QtInstrumentView.h"
#include "MantidKernel/UsageService.h"

#include <QMessageBox>
#include <QScrollBar>
#include <boost/algorithm/string/join.hpp>
#include <utility>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace {
// Changing the palette for spin boxes doesn't work but we can
// change the background colour with a style sheet. This also changes
// the font slightly on Ubuntu so there may be a better way to do this,
// but it's not a big issue so this should be fine for now.
void showAsInvalid(QDoubleSpinBox &spinBox) { spinBox.setStyleSheet("QDoubleSpinBox { background-color: #ffb8ad; }"); }

void showAsValid(QDoubleSpinBox &spinBox) { spinBox.setStyleSheet(""); }

void showAsInvalid(QLineEdit &lineEdit) { lineEdit.setStyleSheet("QLineEdit { background-color: #ffb8ad; }"); }

void showAsValid(QLineEdit &lineEdit) { lineEdit.setStyleSheet(""); }
} // namespace

/** Constructor
 * @param algorithmForTooltips :: [input] An algorithm that will be
 * used to find tooltips for the input properties
 * @param parent :: [input] The parent of this widget
 */
QtInstrumentView::QtInstrumentView(const Mantid::API::IAlgorithm_sptr &algorithmForTooltips, QWidget *parent)
    : QWidget(parent) {
  initLayout();
  registerSettingsWidgets(algorithmForTooltips);
}

void QtInstrumentView::subscribe(InstrumentViewSubscriber *notifyee) { m_notifyee = notifyee; }

/**
Initialise the Interface
*/
void QtInstrumentView::initLayout() {
  m_ui.setupUi(this);
  m_ui.monIntMinEdit->setSpecialValueText("Unset");
  m_ui.monIntMaxEdit->setSpecialValueText("Unset");
  m_ui.monBgMinEdit->setSpecialValueText("Unset");
  m_ui.monBgMaxEdit->setSpecialValueText("Unset");
  m_ui.lamMinEdit->setSpecialValueText("Unset");
  m_ui.lamMaxEdit->setSpecialValueText("Unset");
  connect(m_ui.getInstDefaultsButton, SIGNAL(clicked()), this, SLOT(onRestoreDefaultsRequested()));
  connect(m_ui.calibrationPathButton, SIGNAL(clicked()), this, SLOT(browseToCalibrationFile()));
}

void QtInstrumentView::connectSettingsChange(QLineEdit &edit) {
  connect(&edit, SIGNAL(textChanged(QString const &)), this, SLOT(onSettingsChanged()));
}

void QtInstrumentView::connectSettingsChange(QSpinBox &edit) {
  connect(&edit, SIGNAL(valueChanged(QString const &)), this, SLOT(onSettingsChanged()));
}

void QtInstrumentView::connectSettingsChange(QDoubleSpinBox &edit) {
  connect(&edit, SIGNAL(valueChanged(QString const &)), this, SLOT(onSettingsChanged()));
}

void QtInstrumentView::connectSettingsChange(QComboBox &edit) {
  connect(&edit, SIGNAL(currentIndexChanged(int)), this, SLOT(onSettingsChanged()));
}

void QtInstrumentView::connectSettingsChange(QCheckBox &edit) {
  connect(&edit, SIGNAL(stateChanged(int)), this, SLOT(onSettingsChanged()));
}

void QtInstrumentView::disconnectSettingsChange(QLineEdit &edit) {
  disconnect(&edit, SIGNAL(textChanged(QString const &)), this, SLOT(onSettingsChanged()));
}

void QtInstrumentView::disconnectSettingsChange(QSpinBox &edit) {
  disconnect(&edit, SIGNAL(valueChanged(QString const &)), this, SLOT(onSettingsChanged()));
}

void QtInstrumentView::disconnectSettingsChange(QDoubleSpinBox &edit) {
  disconnect(&edit, SIGNAL(valueChanged(QString const &)), this, SLOT(onSettingsChanged()));
}

void QtInstrumentView::disconnectSettingsChange(QComboBox &edit) {
  disconnect(&edit, SIGNAL(currentIndexChanged(int)), this, SLOT(onSettingsChanged()));
}

void QtInstrumentView::disconnectSettingsChange(QCheckBox &edit) {
  disconnect(&edit, SIGNAL(stateChanged(int)), this, SLOT(onSettingsChanged()));
}

void QtInstrumentView::onSettingsChanged() { m_notifyee->notifySettingsChanged(); }

void QtInstrumentView::browseToCalibrationFile() { m_notifyee->notifyBrowseToCalibrationFileRequested(); }

void QtInstrumentView::onRestoreDefaultsRequested() {
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      Mantid::Kernel::FeatureType::Feature, {"ISIS Reflectometry", "InstrumentTab", "RestoreDefaults"}, false);
  m_notifyee->notifyRestoreDefaultsRequested();
}

void QtInstrumentView::disableAll() { m_ui.instSettingsGroup->setEnabled(false); }

void QtInstrumentView::enableAll() { m_ui.instSettingsGroup->setEnabled(true); }

void QtInstrumentView::enableDetectorCorrectionType() { m_ui.detectorCorrectionTypeComboBox->setEnabled(true); }

void QtInstrumentView::disableDetectorCorrectionType() { m_ui.detectorCorrectionTypeComboBox->setEnabled(false); }

void QtInstrumentView::registerSettingsWidgets(const Mantid::API::IAlgorithm_sptr &alg) {
  registerInstrumentSettingsWidgets(alg);
}

void QtInstrumentView::registerInstrumentSettingsWidgets(const Mantid::API::IAlgorithm_sptr &alg) {
  registerSettingWidget(*m_ui.intMonCheckBox, "NormalizeByIntegratedMonitors", alg);
  registerSettingWidget(*m_ui.monIntMinEdit, "MonitorIntegrationWavelengthMin", alg);
  registerSettingWidget(*m_ui.monIntMaxEdit, "MonitorIntegrationWavelengthMax", alg);
  registerSettingWidget(*m_ui.monBgMinEdit, "MonitorBackgroundWavelengthMin", alg);
  registerSettingWidget(*m_ui.monBgMaxEdit, "MonitorBackgroundWavelengthMax", alg);
  registerSettingWidget(*m_ui.lamMinEdit, "WavelengthMin", alg);
  registerSettingWidget(*m_ui.lamMaxEdit, "WavelengthMax", alg);
  registerSettingWidget(*m_ui.I0MonitorIndex, "I0MonitorIndex", alg);
  registerSettingWidget(*m_ui.detectorCorrectionTypeComboBox, "DetectorCorrectionType", alg);
  registerSettingWidget(*m_ui.correctDetectorsCheckBox, "CorrectDetectors", alg);
  registerSettingWidget(*m_ui.calibrationPathEdit, "CalibrationFile", alg);
}

void QtInstrumentView::connectInstrumentSettingsWidgets() {
  connectSettingsChange(*m_ui.intMonCheckBox);
  connectSettingsChange(*m_ui.monIntMinEdit);
  connectSettingsChange(*m_ui.monIntMaxEdit);
  connectSettingsChange(*m_ui.monBgMinEdit);
  connectSettingsChange(*m_ui.monBgMaxEdit);
  connectSettingsChange(*m_ui.lamMinEdit);
  connectSettingsChange(*m_ui.lamMaxEdit);
  connectSettingsChange(*m_ui.I0MonitorIndex);
  connectSettingsChange(*m_ui.detectorCorrectionTypeComboBox);
  connectSettingsChange(*m_ui.correctDetectorsCheckBox);
  connectSettingsChange(*m_ui.calibrationPathEdit);
}

void QtInstrumentView::disconnectInstrumentSettingsWidgets() {
  disconnectSettingsChange(*m_ui.intMonCheckBox);
  disconnectSettingsChange(*m_ui.monIntMinEdit);
  disconnectSettingsChange(*m_ui.monIntMaxEdit);
  disconnectSettingsChange(*m_ui.monBgMinEdit);
  disconnectSettingsChange(*m_ui.monBgMaxEdit);
  disconnectSettingsChange(*m_ui.lamMinEdit);
  disconnectSettingsChange(*m_ui.lamMaxEdit);
  disconnectSettingsChange(*m_ui.I0MonitorIndex);
  disconnectSettingsChange(*m_ui.detectorCorrectionTypeComboBox);
  disconnectSettingsChange(*m_ui.correctDetectorsCheckBox);
  disconnectSettingsChange(*m_ui.calibrationPathEdit);
}

template <typename Widget>
void QtInstrumentView::registerSettingWidget(Widget &widget, std::string const &propertyName,
                                             const Mantid::API::IAlgorithm_sptr &alg) {
  connectSettingsChange(widget);
  setToolTipAsPropertyDocumentation(widget, propertyName, alg);
}

void QtInstrumentView::setToolTipAsPropertyDocumentation(QWidget &widget, std::string const &propertyName,
                                                         const Mantid::API::IAlgorithm_sptr &alg) {
  widget.setToolTip(QString::fromStdString(alg->getPointerToProperty(propertyName)->documentation()));
}

void QtInstrumentView::setSelected(QComboBox &box, std::string const &str) {
  auto const index = box.findText(QString::fromStdString(str));
  if (index != -1)
    box.setCurrentIndex(index);
}

void QtInstrumentView::setText(QLineEdit &lineEdit, boost::optional<double> value) {
  if (value)
    setText(lineEdit, value.get());
}

void QtInstrumentView::setText(QLineEdit &lineEdit, std::optional<int> value) {
  if (value)
    setText(lineEdit, value.value());
}

void QtInstrumentView::setText(QLineEdit &lineEdit, boost::optional<std::string> const &text) {
  if (text && !text->empty())
    setText(lineEdit, text);
}

void QtInstrumentView::setText(QLineEdit &lineEdit, double value) {
  auto valueAsString = QString::number(value);
  lineEdit.setText(valueAsString);
}

void QtInstrumentView::setText(QLineEdit &lineEdit, int value) {
  auto valueAsString = QString::number(value);
  lineEdit.setText(valueAsString);
}

void QtInstrumentView::setText(QLineEdit &lineEdit, std::string const &text) {
  auto textAsQString = QString::fromStdString(text);
  lineEdit.setText(textAsQString);
}

void QtInstrumentView::setChecked(QCheckBox &checkBox, bool checked) {
  auto checkedAsCheckState = checked ? Qt::Checked : Qt::Unchecked;
  checkBox.setCheckState(checkedAsCheckState);
}

std::string QtInstrumentView::getText(QLineEdit const &lineEdit) const { return lineEdit.text().toStdString(); }

std::string QtInstrumentView::getText(QComboBox const &box) const { return box.currentText().toStdString(); }

int QtInstrumentView::getMonitorIndex() const { return m_ui.I0MonitorIndex->value(); }

void QtInstrumentView::setMonitorIndex(int value) { m_ui.I0MonitorIndex->setValue(value); }

bool QtInstrumentView::getIntegrateMonitors() const { return m_ui.intMonCheckBox->isChecked(); }

void QtInstrumentView::setIntegrateMonitors(bool value) { m_ui.intMonCheckBox->setChecked(value); }

double QtInstrumentView::getLambdaMin() const { return m_ui.lamMinEdit->value(); }

void QtInstrumentView::setLambdaMin(double value) { m_ui.lamMinEdit->setValue(value); }

double QtInstrumentView::getLambdaMax() const { return m_ui.lamMaxEdit->value(); }

void QtInstrumentView::setLambdaMax(double value) { m_ui.lamMaxEdit->setValue(value); }

void QtInstrumentView::showLambdaRangeInvalid() {
  showAsInvalid(*m_ui.lamMinEdit);
  showAsInvalid(*m_ui.lamMaxEdit);
}

void QtInstrumentView::showLambdaRangeValid() {
  showAsValid(*m_ui.lamMinEdit);
  showAsValid(*m_ui.lamMaxEdit);
}

double QtInstrumentView::getMonitorBackgroundMin() const { return m_ui.monBgMinEdit->value(); }

void QtInstrumentView::setMonitorBackgroundMin(double value) { m_ui.monBgMinEdit->setValue(value); }

double QtInstrumentView::getMonitorBackgroundMax() const { return m_ui.monBgMaxEdit->value(); }

void QtInstrumentView::setMonitorBackgroundMax(double value) { m_ui.monBgMaxEdit->setValue(value); }

void QtInstrumentView::showMonitorBackgroundRangeInvalid() {
  showAsInvalid(*m_ui.monBgMinEdit);
  showAsInvalid(*m_ui.monBgMaxEdit);
}

void QtInstrumentView::showMonitorBackgroundRangeValid() {
  showAsValid(*m_ui.monBgMinEdit);
  showAsValid(*m_ui.monBgMaxEdit);
}

double QtInstrumentView::getMonitorIntegralMin() const { return m_ui.monIntMinEdit->value(); }

void QtInstrumentView::setMonitorIntegralMin(double value) { m_ui.monIntMinEdit->setValue(value); }

double QtInstrumentView::getMonitorIntegralMax() const { return m_ui.monIntMaxEdit->value(); }

void QtInstrumentView::setMonitorIntegralMax(double value) { m_ui.monIntMaxEdit->setValue(value); }

void QtInstrumentView::showMonitorIntegralRangeInvalid() {
  showAsInvalid(*m_ui.monIntMinEdit);
  showAsInvalid(*m_ui.monIntMaxEdit);
}

void QtInstrumentView::showMonitorIntegralRangeValid() {
  showAsValid(*m_ui.monIntMinEdit);
  showAsValid(*m_ui.monIntMaxEdit);
}

bool QtInstrumentView::getCorrectDetectors() const { return m_ui.correctDetectorsCheckBox->isChecked(); }

void QtInstrumentView::setCorrectDetectors(bool value) { m_ui.correctDetectorsCheckBox->setChecked(value); }

std::string QtInstrumentView::getDetectorCorrectionType() const {
  return getText(*m_ui.detectorCorrectionTypeComboBox);
}

void QtInstrumentView::setDetectorCorrectionType(std::string const &value) {
  setSelected(*m_ui.detectorCorrectionTypeComboBox, value);
}

std::string QtInstrumentView::getCalibrationFilePath() const { return m_ui.calibrationPathEdit->text().toStdString(); }

void QtInstrumentView::setCalibrationFilePath(std::string const &value) {
  m_ui.calibrationPathEdit->setText(QString::fromStdString(value));
}

void QtInstrumentView::showCalibrationFilePathInvalid() { showAsInvalid(*m_ui.calibrationPathEdit); }

void QtInstrumentView::showCalibrationFilePathValid() { showAsValid(*m_ui.calibrationPathEdit); }
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
