// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "QInstrumentView.h"
#include <QMessageBox>
#include <QScrollBar>
#include <boost/algorithm/string/join.hpp>

namespace MantidQt {
namespace CustomInterfaces {

namespace {
// Changing the palette for spin boxes doesn't work but we can
// change the background colour with a style sheet. This also changes
// the font slightly on Ubuntu so there may be a better way to do this,
// but it's not a big issue so this should be fine for now.
void showAsInvalid(QDoubleSpinBox &spinBox) {
  spinBox.setStyleSheet("QDoubleSpinBox { background-color: #ffb8ad; }");
}

void showAsValid(QDoubleSpinBox &spinBox) { spinBox.setStyleSheet(""); }
} // namespace

/** Constructor
 * @param algorithmForTooltips :: [input] An algorithm that will be
 * used to find tooltips for the input properties
 * @param parent :: [input] The parent of this widget
 */
QInstrumentView::QInstrumentView(
    Mantid::API::IAlgorithm_sptr algorithmForTooltips, QWidget *parent)
    : QWidget(parent) {
  initLayout();
  registerSettingsWidgets(algorithmForTooltips);
}

void QInstrumentView::subscribe(InstrumentViewSubscriber *notifyee) {
  m_notifyee = notifyee;
}

/**
Initialise the Interface
*/
void QInstrumentView::initLayout() {
  m_ui.setupUi(this);
  m_ui.monIntMinEdit->setSpecialValueText("Unset");
  m_ui.monIntMaxEdit->setSpecialValueText("Unset");
  m_ui.monBgMinEdit->setSpecialValueText("Unset");
  m_ui.monBgMaxEdit->setSpecialValueText("Unset");
  m_ui.lamMinEdit->setSpecialValueText("Unset");
  m_ui.lamMaxEdit->setSpecialValueText("Unset");
  connect(m_ui.getInstDefaultsButton, SIGNAL(clicked()), this,
          SLOT(onRestoreDefaultsRequested()));
}

void QInstrumentView::connectSettingsChange(QLineEdit &edit) {
  connect(&edit, SIGNAL(textChanged(QString const &)), this,
          SLOT(onSettingsChanged()));
}

void QInstrumentView::connectSettingsChange(QSpinBox &edit) {
  connect(&edit, SIGNAL(valueChanged(QString const &)), this,
          SLOT(onSettingsChanged()));
}

void QInstrumentView::connectSettingsChange(QDoubleSpinBox &edit) {
  connect(&edit, SIGNAL(valueChanged(QString const &)), this,
          SLOT(onSettingsChanged()));
}

void QInstrumentView::connectSettingsChange(QComboBox &edit) {
  connect(&edit, SIGNAL(currentIndexChanged(int)), this,
          SLOT(onSettingsChanged()));
}

void QInstrumentView::connectSettingsChange(QCheckBox &edit) {
  connect(&edit, SIGNAL(stateChanged(int)), this, SLOT(onSettingsChanged()));
}

void QInstrumentView::disconnectSettingsChange(QLineEdit &edit) {
  disconnect(&edit, SIGNAL(textChanged(QString const &)), this,
             SLOT(onSettingsChanged()));
}

void QInstrumentView::disconnectSettingsChange(QSpinBox &edit) {
  disconnect(&edit, SIGNAL(valueChanged(QString const &)), this,
             SLOT(onSettingsChanged()));
}

void QInstrumentView::disconnectSettingsChange(QDoubleSpinBox &edit) {
  disconnect(&edit, SIGNAL(valueChanged(QString const &)), this,
             SLOT(onSettingsChanged()));
}

void QInstrumentView::disconnectSettingsChange(QComboBox &edit) {
  disconnect(&edit, SIGNAL(currentIndexChanged(int)), this,
             SLOT(onSettingsChanged()));
}

void QInstrumentView::disconnectSettingsChange(QCheckBox &edit) {
  disconnect(&edit, SIGNAL(stateChanged(int)), this, SLOT(onSettingsChanged()));
}

void QInstrumentView::onSettingsChanged() {
  m_notifyee->notifySettingsChanged();
}

void QInstrumentView::onRestoreDefaultsRequested() {
  m_notifyee->notifyRestoreDefaultsRequested();
}

void QInstrumentView::disableAll() {
  m_ui.instSettingsGroup->setEnabled(false);
}

void QInstrumentView::enableAll() { m_ui.instSettingsGroup->setEnabled(true); }

void QInstrumentView::enableDetectorCorrectionType() {
  m_ui.detectorCorrectionTypeComboBox->setEnabled(true);
}

void QInstrumentView::disableDetectorCorrectionType() {
  m_ui.detectorCorrectionTypeComboBox->setEnabled(false);
}

void QInstrumentView::registerSettingsWidgets(
    Mantid::API::IAlgorithm_sptr alg) {
  registerInstrumentSettingsWidgets(alg);
}

void QInstrumentView::registerInstrumentSettingsWidgets(
    Mantid::API::IAlgorithm_sptr alg) {
  registerSettingWidget(*m_ui.intMonCheckBox, "NormalizeByIntegratedMonitors",
                        alg);
  registerSettingWidget(*m_ui.monIntMinEdit, "MonitorIntegrationWavelengthMin",
                        alg);
  registerSettingWidget(*m_ui.monIntMaxEdit, "MonitorIntegrationWavelengthMax",
                        alg);
  registerSettingWidget(*m_ui.monBgMinEdit, "MonitorBackgroundWavelengthMin",
                        alg);
  registerSettingWidget(*m_ui.monBgMaxEdit, "MonitorBackgroundWavelengthMax",
                        alg);
  registerSettingWidget(*m_ui.lamMinEdit, "WavelengthMin", alg);
  registerSettingWidget(*m_ui.lamMaxEdit, "WavelengthMax", alg);
  registerSettingWidget(*m_ui.I0MonitorIndex, "I0MonitorIndex", alg);
  registerSettingWidget(*m_ui.detectorCorrectionTypeComboBox,
                        "DetectorCorrectionType", alg);
  registerSettingWidget(*m_ui.correctDetectorsCheckBox, "CorrectDetectors",
                        alg);
}

void QInstrumentView::connectInstrumentSettingsWidgets() {
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
}

void QInstrumentView::disconnectInstrumentSettingsWidgets() {
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
}

template <typename Widget>
void QInstrumentView::registerSettingWidget(Widget &widget,
                                            std::string const &propertyName,
                                            Mantid::API::IAlgorithm_sptr alg) {
  connectSettingsChange(widget);
  setToolTipAsPropertyDocumentation(widget, propertyName, alg);
}

void QInstrumentView::setToolTipAsPropertyDocumentation(
    QWidget &widget, std::string const &propertyName,
    Mantid::API::IAlgorithm_sptr alg) {
  widget.setToolTip(QString::fromStdString(
      alg->getPointerToProperty(propertyName)->documentation()));
}

void QInstrumentView::setSelected(QComboBox &box, std::string const &str) {
  auto const index = box.findText(QString::fromStdString(str));
  if (index != -1)
    box.setCurrentIndex(index);
}

void QInstrumentView::setText(QLineEdit &lineEdit,
                              boost::optional<double> value) {
  if (value)
    setText(lineEdit, value.get());
}

void QInstrumentView::setText(QLineEdit &lineEdit, boost::optional<int> value) {
  if (value)
    setText(lineEdit, value.get());
}

void QInstrumentView::setText(QLineEdit &lineEdit,
                              boost::optional<std::string> const &text) {
  if (text && !text->empty())
    setText(lineEdit, text);
}

void QInstrumentView::setText(QLineEdit &lineEdit, double value) {
  auto valueAsString = QString::number(value);
  lineEdit.setText(valueAsString);
}

void QInstrumentView::setText(QLineEdit &lineEdit, int value) {
  auto valueAsString = QString::number(value);
  lineEdit.setText(valueAsString);
}

void QInstrumentView::setText(QLineEdit &lineEdit, std::string const &text) {
  auto textAsQString = QString::fromStdString(text);
  lineEdit.setText(textAsQString);
}

void QInstrumentView::setChecked(QCheckBox &checkBox, bool checked) {
  auto checkedAsCheckState = checked ? Qt::Checked : Qt::Unchecked;
  checkBox.setCheckState(checkedAsCheckState);
}

std::string QInstrumentView::getText(QLineEdit const &lineEdit) const {
  return lineEdit.text().toStdString();
}

std::string QInstrumentView::getText(QComboBox const &box) const {
  return box.currentText().toStdString();
}

QString QInstrumentView::messageFor(
    InstrumentParameterTypeMissmatch const &typeError) const {
  return QString::fromStdString(typeError.parameterName()) +
         " should hold an " + QString::fromStdString(typeError.expectedType()) +
         " value but does not.\n";
}

template <typename T, typename StringConverter>
std::string toCsv(std::vector<T> const &values, StringConverter toString) {
  std::vector<std::string> valuesAsStrings;
  valuesAsStrings.reserve(values.size());
  std::transform(values.cbegin(), values.cend(),
                 std::back_inserter(valuesAsStrings), toString);
  return boost::algorithm::join(valuesAsStrings, ", ");
}

QString QInstrumentView::messageFor(
    std::vector<MissingInstrumentParameterValue> const &missingValues) const {
  auto missingNamesCsv = toCsv(
      missingValues,
      [](const MissingInstrumentParameterValue &missingValue) -> std::string {
        return missingValue.parameterName();
      });

  return QString::fromStdString(missingNamesCsv) +
         QString(missingValues.size() == 1 ? " is" : " are") +
         " not set in the instrument parameter file but should be.\n";
}

int QInstrumentView::getMonitorIndex() const {
  return m_ui.I0MonitorIndex->value();
}

void QInstrumentView::setMonitorIndex(int value) {
  m_ui.I0MonitorIndex->setValue(value);
}

bool QInstrumentView::getIntegrateMonitors() const {
  return m_ui.intMonCheckBox->isChecked();
}

void QInstrumentView::setIntegrateMonitors(bool value) {
  m_ui.intMonCheckBox->setChecked(value);
}

double QInstrumentView::getLambdaMin() const {
  return m_ui.lamMinEdit->value();
}

void QInstrumentView::setLambdaMin(double value) {
  m_ui.lamMinEdit->setValue(value);
}

double QInstrumentView::getLambdaMax() const {
  return m_ui.lamMaxEdit->value();
}

void QInstrumentView::setLambdaMax(double value) {
  m_ui.lamMaxEdit->setValue(value);
}

void QInstrumentView::showLambdaRangeInvalid() {
  showAsInvalid(*m_ui.lamMinEdit);
  showAsInvalid(*m_ui.lamMaxEdit);
}

void QInstrumentView::showLambdaRangeValid() {
  showAsValid(*m_ui.lamMinEdit);
  showAsValid(*m_ui.lamMaxEdit);
}

double QInstrumentView::getMonitorBackgroundMin() const {
  return m_ui.monBgMinEdit->value();
}

void QInstrumentView::setMonitorBackgroundMin(double value) {
  m_ui.monBgMinEdit->setValue(value);
}

double QInstrumentView::getMonitorBackgroundMax() const {
  return m_ui.monBgMaxEdit->value();
}

void QInstrumentView::setMonitorBackgroundMax(double value) {
  m_ui.monBgMaxEdit->setValue(value);
}

void QInstrumentView::showMonitorBackgroundRangeInvalid() {
  showAsInvalid(*m_ui.monBgMinEdit);
  showAsInvalid(*m_ui.monBgMaxEdit);
}

void QInstrumentView::showMonitorBackgroundRangeValid() {
  showAsValid(*m_ui.monBgMinEdit);
  showAsValid(*m_ui.monBgMaxEdit);
}

double QInstrumentView::getMonitorIntegralMin() const {
  return m_ui.monIntMinEdit->value();
}

void QInstrumentView::setMonitorIntegralMin(double value) {
  m_ui.monIntMinEdit->setValue(value);
}

double QInstrumentView::getMonitorIntegralMax() const {
  return m_ui.monIntMaxEdit->value();
}

void QInstrumentView::setMonitorIntegralMax(double value) {
  m_ui.monIntMaxEdit->setValue(value);
}

void QInstrumentView::showMonitorIntegralRangeInvalid() {
  showAsInvalid(*m_ui.monIntMinEdit);
  showAsInvalid(*m_ui.monIntMaxEdit);
}

void QInstrumentView::showMonitorIntegralRangeValid() {
  showAsValid(*m_ui.monIntMinEdit);
  showAsValid(*m_ui.monIntMaxEdit);
}

bool QInstrumentView::getCorrectDetectors() const {
  return m_ui.correctDetectorsCheckBox->isChecked();
}

void QInstrumentView::setCorrectDetectors(bool value) {
  m_ui.correctDetectorsCheckBox->setChecked(value);
}

std::string QInstrumentView::getDetectorCorrectionType() const {
  return getText(*m_ui.detectorCorrectionTypeComboBox);
}

void QInstrumentView::setDetectorCorrectionType(std::string const &value) {
  setSelected(*m_ui.detectorCorrectionTypeComboBox, value);
}
} // namespace CustomInterfaces
} // namespace MantidQt
