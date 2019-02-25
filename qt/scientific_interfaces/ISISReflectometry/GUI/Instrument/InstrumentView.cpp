// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "InstrumentView.h"
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
InstrumentView::InstrumentView(
    Mantid::API::IAlgorithm_sptr algorithmForTooltips, QWidget *parent)
    : QWidget(parent) {
  initLayout();
  registerSettingsWidgets(algorithmForTooltips);
}

void InstrumentView::subscribe(InstrumentViewSubscriber *notifyee) {
  m_notifyee = notifyee;
}

/**
Initialise the Interface
*/
void InstrumentView::initLayout() {
  m_ui.setupUi(this);
  m_ui.monIntMinEdit->setSpecialValueText("Unset");
  m_ui.monIntMaxEdit->setSpecialValueText("Unset");
  m_ui.monBgMinEdit->setSpecialValueText("Unset");
  m_ui.monBgMaxEdit->setSpecialValueText("Unset");
  m_ui.lamMinEdit->setSpecialValueText("Unset");
  m_ui.lamMaxEdit->setSpecialValueText("Unset");
  connect(m_ui.getInstDefaultsButton, SIGNAL(clicked()), this,
          SLOT(onGetDefaultsClicked()));
}

void InstrumentView::connectSettingsChange(QLineEdit &edit) {
  connect(&edit, SIGNAL(textChanged(QString const &)), this,
          SLOT(onSettingsChanged()));
}

void InstrumentView::connectSettingsChange(QSpinBox &edit) {
  connect(&edit, SIGNAL(valueChanged(QString const &)), this,
          SLOT(onSettingsChanged()));
}

void InstrumentView::connectSettingsChange(QDoubleSpinBox &edit) {
  connect(&edit, SIGNAL(valueChanged(QString const &)), this,
          SLOT(onSettingsChanged()));
}

void InstrumentView::connectSettingsChange(QComboBox &edit) {
  connect(&edit, SIGNAL(currentIndexChanged(int)), this,
          SLOT(onSettingsChanged()));
}

void InstrumentView::connectSettingsChange(QCheckBox &edit) {
  connect(&edit, SIGNAL(stateChanged(int)), this, SLOT(onSettingsChanged()));
}

void InstrumentView::onSettingsChanged() {
  m_notifyee->notifySettingsChanged();
}

void InstrumentView::onGetDefaultsClicked() { m_notifyee->notifyGetDefaults(); }

void InstrumentView::disableAll() { m_ui.instSettingsGroup->setEnabled(false); }

void InstrumentView::enableAll() { m_ui.instSettingsGroup->setEnabled(true); }

void InstrumentView::enableDetectorCorrectionType() {
  m_ui.detectorCorrectionTypeComboBox->setEnabled(true);
}

void InstrumentView::disableDetectorCorrectionType() {
  m_ui.detectorCorrectionTypeComboBox->setEnabled(false);
}

void InstrumentView::registerSettingsWidgets(Mantid::API::IAlgorithm_sptr alg) {
  registerInstrumentSettingsWidgets(alg);
}

void InstrumentView::registerInstrumentSettingsWidgets(
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

template <typename Widget>
void InstrumentView::registerSettingWidget(Widget &widget,
                                           std::string const &propertyName,
                                           Mantid::API::IAlgorithm_sptr alg) {
  connectSettingsChange(widget);
  setToolTipAsPropertyDocumentation(widget, propertyName, alg);
}

void InstrumentView::setToolTipAsPropertyDocumentation(
    QWidget &widget, std::string const &propertyName,
    Mantid::API::IAlgorithm_sptr alg) {
  widget.setToolTip(QString::fromStdString(
      alg->getPointerToProperty(propertyName)->documentation()));
}

void InstrumentView::setSelected(QComboBox &box, std::string const &str) {
  auto const index = box.findText(QString::fromStdString(str));
  if (index != -1)
    box.setCurrentIndex(index);
}

void InstrumentView::setText(QLineEdit &lineEdit,
                             boost::optional<double> value) {
  if (value)
    setText(lineEdit, value.get());
}

void InstrumentView::setText(QLineEdit &lineEdit, boost::optional<int> value) {
  if (value)
    setText(lineEdit, value.get());
}

void InstrumentView::setText(QLineEdit &lineEdit,
                             boost::optional<std::string> const &text) {
  if (text && !text->empty())
    setText(lineEdit, text);
}

void InstrumentView::setText(QLineEdit &lineEdit, double value) {
  auto valueAsString = QString::number(value);
  lineEdit.setText(valueAsString);
}

void InstrumentView::setText(QLineEdit &lineEdit, int value) {
  auto valueAsString = QString::number(value);
  lineEdit.setText(valueAsString);
}

void InstrumentView::setText(QLineEdit &lineEdit, std::string const &text) {
  auto textAsQString = QString::fromStdString(text);
  lineEdit.setText(textAsQString);
}

void InstrumentView::setChecked(QCheckBox &checkBox, bool checked) {
  auto checkedAsCheckState = checked ? Qt::Checked : Qt::Unchecked;
  checkBox.setCheckState(checkedAsCheckState);
}

std::string InstrumentView::getText(QLineEdit const &lineEdit) const {
  return lineEdit.text().toStdString();
}

std::string InstrumentView::getText(QComboBox const &box) const {
  return box.currentText().toStdString();
}

QString InstrumentView::messageFor(
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

QString InstrumentView::messageFor(
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

int InstrumentView::getMonitorIndex() const {
  return m_ui.I0MonitorIndex->value();
}

bool InstrumentView::getIntegrateMonitors() const {
  return m_ui.intMonCheckBox->isChecked();
}

double InstrumentView::getLambdaMin() const { return m_ui.lamMinEdit->value(); }

double InstrumentView::getLambdaMax() const { return m_ui.lamMaxEdit->value(); }

void InstrumentView::showLambdaRangeInvalid() {
  showAsInvalid(*m_ui.lamMinEdit);
  showAsInvalid(*m_ui.lamMaxEdit);
}

void InstrumentView::showLambdaRangeValid() {
  showAsValid(*m_ui.lamMinEdit);
  showAsValid(*m_ui.lamMaxEdit);
}

double InstrumentView::getMonitorBackgroundMin() const {
  return m_ui.monBgMinEdit->value();
}

double InstrumentView::getMonitorBackgroundMax() const {
  return m_ui.monBgMaxEdit->value();
}

void InstrumentView::showMonitorBackgroundRangeInvalid() {
  showAsInvalid(*m_ui.monBgMinEdit);
  showAsInvalid(*m_ui.monBgMaxEdit);
}

void InstrumentView::showMonitorBackgroundRangeValid() {
  showAsValid(*m_ui.monBgMinEdit);
  showAsValid(*m_ui.monBgMaxEdit);
}

double InstrumentView::getMonitorIntegralMin() const {
  return m_ui.monIntMinEdit->value();
}

double InstrumentView::getMonitorIntegralMax() const {
  return m_ui.monIntMaxEdit->value();
}

void InstrumentView::showMonitorIntegralRangeInvalid() {
  showAsInvalid(*m_ui.monIntMinEdit);
  showAsInvalid(*m_ui.monIntMaxEdit);
}

void InstrumentView::showMonitorIntegralRangeValid() {
  showAsValid(*m_ui.monIntMinEdit);
  showAsValid(*m_ui.monIntMaxEdit);
}

bool InstrumentView::getCorrectDetectors() const {
  return m_ui.correctDetectorsCheckBox->isChecked();
}

std::string InstrumentView::getDetectorCorrectionType() const {
  return getText(*m_ui.detectorCorrectionTypeComboBox);
}
} // namespace CustomInterfaces
} // namespace MantidQt
