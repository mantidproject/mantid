#include "InstrumentView.h"
#include <boost/algorithm/string/join.hpp>
#include <QMessageBox>
#include <QScrollBar>

namespace MantidQt {
namespace CustomInterfaces {

/** Constructor
* @param parent :: [input] The parent of this widget
*/
InstrumentView::InstrumentView(
    Mantid::API::IAlgorithm_sptr algorithmForTooltips, QWidget *parent) {
  UNUSED_ARG(parent);
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
}

void InstrumentView::connectSettingsChange(QLineEdit &edit) {
  connect(&edit, SIGNAL(textChanged(QString const &)), this,
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

void InstrumentView::connectSettingsChange(QTableWidget &edit) {
  connect(&edit, SIGNAL(cellChanged(int, int)), this,
          SLOT(onPerAngleDefaultsChanged(int, int)));
}

void InstrumentView::disableAll() { m_ui.instSettingsGrid->setEnabled(false); }

void InstrumentView::enableAll() { m_ui.instSettingsGrid->setEnabled(true); }

void InstrumentView::registerSettingsWidgets(Mantid::API::IAlgorithm_sptr alg) {
  registerInstrumentSettingsWidgets(alg);
}

void InstrumentView::registerInstrumentSettingsWidgets(
    Mantid::API::IAlgorithm_sptr alg) {
  connectSettingsChange(*m_ui.instSettingsGroup);
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
  registerSettingWidget(*m_ui.I0MonIndexEdit, "I0MonitorIndex", alg);
  registerSettingWidget(*m_ui.detectorCorrectionTypeComboBox,
                        "DetectorCorrectionType", alg);
  registerSettingWidget(*m_ui.correctDetectorsCheckBox, "CorrectDetectors",
                        alg);
}

void InstrumentView::enableReductionType() {
  m_ui.reductionTypeComboBox->setEnabled(true);
}

void InstrumentView::disableReductionType() {
  m_ui.reductionTypeComboBox->setEnabled(false);
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

// void InstrumentView::setText(QTableWidget &table,
//                             std::string const &propertyName,
//                             boost::optional<double> value) {
//  if (value)
//    setText(table, propertyName, value.get());
//}
//
// void InstrumentView::setText(QTableWidget &table,
//                             std::string const &propertyName, double value) {
//  auto valueAsString = QString::number(value);
//  setText(table, propertyName, valueAsString);
//}
//
// void InstrumentView::setText(QTableWidget &table,
//                             std::string const &propertyName,
//                             boost::optional<std::string> text) {
//  if (text && !text->empty())
//    setText(table, propertyName, text.get());
//}
//
// void InstrumentView::setText(QTableWidget &table,
//                             std::string const &propertyName,
//                             std::string const &text) {
//  auto textAsQString = QString::fromStdString(text);
//  setText(table, propertyName, textAsQString);
//}
//
// void InstrumentView::setText(QTableWidget &table,
//                             std::string const &propertyName,
//                             const QString &value) {
//  // Find the column with this property name
//  const auto columnIt =
//      std::find(m_columnProperties.begin(), m_columnProperties.end(),
//                QString::fromStdString(propertyName));
//  // Do nothing if column was not found
//  if (columnIt == m_columnProperties.end())
//    return;
//
//  const auto column = columnIt - m_columnProperties.begin();
//
//  // Set the value in this column for the first row. (We don't really know
//  // which row(s) the user might want updated so for now keep it simple.)
//  constexpr int row = 0;
//  auto cell = table.item(row, column);
//  if (!cell) {
//    cell = new QTableWidgetItem();
//    table.setItem(row, column, cell);
//  }
//  cell->setText(value);
//}

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
  auto missingNamesCsv =
      toCsv(missingValues,
            [](const MissingInstrumentParameterValue &missingValue)
                -> std::string { return missingValue.parameterName(); });

  return QString::fromStdString(missingNamesCsv) +
         QString(missingValues.size() == 1 ? " is" : " are") +
         " not set in the instrument parameter file but should be.\n";
}

void InstrumentView::showOptionLoadErrors(
    std::vector<InstrumentParameterTypeMissmatch> const &typeErrors,
    std::vector<MissingInstrumentParameterValue> const &missingValues) {
  auto message = QString(
      "Unable to retrieve default values for the following parameters:\n");

  if (!missingValues.empty())
    message += messageFor(missingValues);

  for (auto &typeError : typeErrors)
    message += messageFor(typeError);

  QMessageBox::warning(
      this, "Failed to load one or more defaults from parameter file", message);
}

std::string
InstrumentView::textFromCell(QTableWidgetItem const *maybeNullItem) const {
  if (maybeNullItem != nullptr) {
    return maybeNullItem->text().toStdString();
  } else {
    return std::string();
  }
}

void showOptionLoadErrors(
    std::vector<InstrumentParameterTypeMissmatch> const &typeErrors,
    std::vector<MissingInstrumentParameterValue> const &missingValues);

} // namespace CustomInterfaces
} // namespace Mantid
