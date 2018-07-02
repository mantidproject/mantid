#include "ExperimentView.h"
#include <boost/algorithm/string/join.hpp>
#include "MantidQtWidgets/Common/AlgorithmHintStrategy.h"
#include <QMessageBox>
#include <QScrollBar>

namespace MantidQt {
namespace CustomInterfaces {

/** Constructor
* @param parent :: [input] The parent of this widget
*/
ExperimentView::ExperimentView(
    Mantid::API::IAlgorithm_sptr algorithmForTooltips, QWidget *parent) {
  UNUSED_ARG(parent);
  initLayout();
  registerSettingsWidgets(algorithmForTooltips);
}

void ExperimentView::subscribe(ExperimentViewSubscriber *notifyee) {
  m_notifyee = notifyee;
}

/**
Initialise the Interface
*/
void ExperimentView::initLayout() {
  m_ui.setupUi(this);

  auto blacklist =
      std::vector<std::string>({"InputWorkspaces", "OutputWorkspace"});
  MantidWidgets::AlgorithmHintStrategy strategy("Stitch1DMany", blacklist);
  createStitchHints(strategy.createHints());

  initOptionsTable();

  connect(m_ui.getExpDefaultsButton, SIGNAL(clicked()), this,
          SLOT(requestExpDefaults()));
  connect(m_ui.expSettingsGrid, SIGNAL(clicked(bool)), this,
          SLOT(setPolarisationOptionsEnabled(bool)));
  connect(m_ui.summationTypeComboBox, SIGNAL(currentIndexChanged(int)), this,
          SLOT(summationTypeChanged(int)));
  connect(m_ui.addPerAngleOptionsButton, SIGNAL(clicked()), this,
          SLOT(addPerAngleOptionsTableRow()));
}

void ExperimentView::initOptionsTable() {
  auto table = m_ui.optionsTable;

  // Set angle and scale columns to a small width so everything fits
  table->resizeColumnsToContents();

  auto header = table->horizontalHeader();
  int totalRowHeight = 0;
  for (int i = 0; i < table->rowCount(); ++i) {
    totalRowHeight += table->rowHeight(i);
  }

  const int padding = 2;
  table->setMinimumHeight(totalRowHeight + header->height() + padding);
}

void ExperimentView::connectSettingsChange(QLineEdit &edit) {
  connect(&edit, SIGNAL(textChanged(QString const &)), this,
          SLOT(notifySettingsChanged()));
}

void ExperimentView::connectSettingsChange(QDoubleSpinBox &edit) {
  connect(&edit, SIGNAL(valueChanged(QString const &)), this,
          SLOT(notifySettingsChanged()));
}

void ExperimentView::connectSettingsChange(QComboBox &edit) {
  connect(&edit, SIGNAL(currentIndexChanged(int)), this,
          SLOT(notifySettingsChanged()));
}

void ExperimentView::connectSettingsChange(QCheckBox &edit) {
  connect(&edit, SIGNAL(stateChanged(int)), this,
          SLOT(notifySettingsChanged()));
}

void ExperimentView::connectSettingsChange(QTableWidget &edit) {
  connect(&edit, SIGNAL(cellChanged(int, int)), this,
          SLOT(notifySettingsChanged()));
}

void ExperimentView::disableAll() { m_ui.expSettingsGrid->setEnabled(false); }

void ExperimentView::enableAll() { m_ui.expSettingsGrid->setEnabled(true); }

void ExperimentView::registerSettingsWidgets(Mantid::API::IAlgorithm_sptr alg) {
  registerExperimentSettingsWidgets(alg);
}

void ExperimentView::registerExperimentSettingsWidgets(
    Mantid::API::IAlgorithm_sptr alg) {
  connectSettingsChange(*m_ui.optionsTable);
  registerSettingWidget(*m_ui.analysisModeComboBox, "AnalysisMode", alg);
  registerSettingWidget(*m_ui.startOverlapEdit, "StartOverlap", alg);
  registerSettingWidget(*m_ui.endOverlapEdit, "EndOverlap", alg);
  registerSettingWidget(*m_ui.polCorrComboBox, "PolarizationAnalysis", alg);
  registerSettingWidget(*m_ui.CRhoEdit, "CRho", alg);
  registerSettingWidget(*m_ui.CAlphaEdit, "CAlpha", alg);
  registerSettingWidget(*m_ui.CApEdit, "CAp", alg);
  registerSettingWidget(*m_ui.CPpEdit, "CPp", alg);
  registerSettingWidget(stitchOptionsLineEdit(), "Params", alg);
}

void ExperimentView::notifySettingsChanged() {
  m_notifyee->notifySettingsChanged();
}

void ExperimentView::summationTypeChanged(int reductionTypeIndex) {
  UNUSED_ARG(reductionTypeIndex);
  m_notifyee->notifySettingsChanged();
  m_notifyee->notifySummationTypeChanged();
}

void ExperimentView::enableReductionType() {
  m_ui.reductionTypeComboBox->setEnabled(true);
}

void ExperimentView::disableReductionType() {
  m_ui.reductionTypeComboBox->setEnabled(false);
}

template <typename Widget>
void ExperimentView::registerSettingWidget(Widget &widget,
                                           std::string const &propertyName,
                                           Mantid::API::IAlgorithm_sptr alg) {
  connectSettingsChange(widget);
  setToolTipAsPropertyDocumentation(widget, propertyName, alg);
}

void ExperimentView::setToolTipAsPropertyDocumentation(
    QWidget &widget, std::string const &propertyName,
    Mantid::API::IAlgorithm_sptr alg) {
  widget.setToolTip(QString::fromStdString(
      alg->getPointerToProperty(propertyName)->documentation()));
}

void ExperimentView::setSelected(QComboBox &box, std::string const &str) {
  auto const index = box.findText(QString::fromStdString(str));
  if (index != -1)
    box.setCurrentIndex(index);
}

void ExperimentView::setText(QLineEdit &lineEdit,
                             boost::optional<double> value) {
  if (value)
    setText(lineEdit, value.get());
}

void ExperimentView::setText(QLineEdit &lineEdit, boost::optional<int> value) {
  if (value)
    setText(lineEdit, value.get());
}

void ExperimentView::setText(QLineEdit &lineEdit,
                             boost::optional<std::string> const &text) {
  if (text && !text->empty())
    setText(lineEdit, text);
}

void ExperimentView::setText(QLineEdit &lineEdit, double value) {
  auto valueAsString = QString::number(value);
  lineEdit.setText(valueAsString);
}

void ExperimentView::setText(QLineEdit &lineEdit, int value) {
  auto valueAsString = QString::number(value);
  lineEdit.setText(valueAsString);
}

void ExperimentView::setText(QLineEdit &lineEdit, std::string const &text) {
  auto textAsQString = QString::fromStdString(text);
  lineEdit.setText(textAsQString);
}

// void ExperimentView::setText(QTableWidget &table,
//                             std::string const &propertyName,
//                             boost::optional<double> value) {
//  if (value)
//    setText(table, propertyName, value.get());
//}
//
// void ExperimentView::setText(QTableWidget &table,
//                             std::string const &propertyName, double value) {
//  auto valueAsString = QString::number(value);
//  setText(table, propertyName, valueAsString);
//}
//
// void ExperimentView::setText(QTableWidget &table,
//                             std::string const &propertyName,
//                             boost::optional<std::string> text) {
//  if (text && !text->empty())
//    setText(table, propertyName, text.get());
//}
//
// void ExperimentView::setText(QTableWidget &table,
//                             std::string const &propertyName,
//                             std::string const &text) {
//  auto textAsQString = QString::fromStdString(text);
//  setText(table, propertyName, textAsQString);
//}
//
// void ExperimentView::setText(QTableWidget &table,
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

void ExperimentView::setChecked(QCheckBox &checkBox, bool checked) {
  auto checkedAsCheckState = checked ? Qt::Checked : Qt::Unchecked;
  checkBox.setCheckState(checkedAsCheckState);
}

void ExperimentView::enablePolarisationCorrections() {
  m_ui.polCorrComboBox->setEnabled(true);
  m_ui.CRhoEdit->setEnabled(true);
  m_ui.CAlphaEdit->setEnabled(true);
  m_ui.CApEdit->setEnabled(true);
  m_ui.CPpEdit->setEnabled(true);
}

void ExperimentView::disablePolarisationCorrections() {
  m_ui.polCorrComboBox->setEnabled(false);
  m_ui.CRhoEdit->setEnabled(false);
  m_ui.CAlphaEdit->setEnabled(false);
  m_ui.CApEdit->setEnabled(false);
  m_ui.CPpEdit->setEnabled(false);

  // Set polarisation corrections text to 'None' when disabled
  setSelected(*m_ui.polCorrComboBox, "None");
  // Clear all parameters as well
  m_ui.CRhoEdit->clear();
  m_ui.CAlphaEdit->clear();
  m_ui.CApEdit->clear();
  m_ui.CPpEdit->clear();
}

/** Add a new row to the transmission runs table
 * */
void ExperimentView::addPerAngleOptionsTableRow() {
  auto numRows = m_ui.optionsTable->rowCount() + 1;
  m_ui.optionsTable->setRowCount(numRows);
  // Select the first cell in the new row
  m_ui.optionsTable->setCurrentCell(numRows - 1, 0);
}

std::string ExperimentView::getText(QLineEdit const &lineEdit) const {
  return lineEdit.text().toStdString();
}

std::string ExperimentView::getText(QComboBox const &box) const {
  return box.currentText().toStdString();
}

QString ExperimentView::messageFor(
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

QString ExperimentView::messageFor(
    std::vector<MissingInstrumentParameterValue> const &missingValues) const {
  auto missingNamesCsv =
      toCsv(missingValues,
            [](const MissingInstrumentParameterValue &missingValue)
                -> std::string { return missingValue.parameterName(); });

  return QString::fromStdString(missingNamesCsv) +
         QString(missingValues.size() == 1 ? " is" : " are") +
         " not set in the instrument parameter file but should be.\n";
}

void ExperimentView::showOptionLoadErrors(
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

QLineEdit &ExperimentView::stitchOptionsLineEdit() const {
  return *static_cast<QLineEdit *>(m_stitchEdit);
}

/** Creates hints for 'Stitch1DMany'
* @param hints :: Hints as a map
*/
void ExperimentView::createStitchHints(
    const std::vector<MantidWidgets::Hint> &hints) {

  // We want to add the stitch params box next to the stitch
  // label, so first find the label's position
  auto stitchLabelIndex = m_ui.expSettingsGrid->indexOf(m_ui.stitchLabel);
  int row, col, rowSpan, colSpan;
  m_ui.expSettingsGrid->getItemPosition(stitchLabelIndex, &row, &col, &rowSpan,
                                        &colSpan);
  // Create the new edit box and add it to the right of the label
  m_stitchEdit = new MantidWidgets::HintingLineEdit(this, hints);
  m_ui.expSettingsGrid->addWidget(m_stitchEdit, row, col + colSpan, 1, 3);
}

double ExperimentView::getCRho() const { return m_ui.CRhoEdit->value(); }

void ExperimentView::setCRho(double cRho) { m_ui.CRhoEdit->setValue(cRho); }

double ExperimentView::getCAlpha() const { return m_ui.CAlphaEdit->value(); }

void ExperimentView::setCAlpha(double cAlpha) {
  m_ui.CAlphaEdit->setValue(cAlpha);
}

double ExperimentView::getCAp() const { return m_ui.CApEdit->value(); }

void ExperimentView::setCAp(double cAp) { m_ui.CApEdit->setValue(cAp); }

double ExperimentView::getCPp() const { return m_ui.CPpEdit->value(); }

void ExperimentView::setCPp(double cPp) { m_ui.CPpEdit->setValue(cPp); }

std::string ExperimentView::getAnalysisMode() const {
  return getText(*m_ui.analysisModeComboBox);
}

void ExperimentView::setAnalysisMode(std::string const &analysisMode) {
  setSelected(*m_ui.analysisModeComboBox, analysisMode);
}

std::string ExperimentView::getSummationType() const {
  return getText(*m_ui.summationTypeComboBox);
}

void ExperimentView::setSummationType(std::string const &summationType) {
  return setSelected(*m_ui.summationTypeComboBox, summationType);
}

std::string ExperimentView::getReductionType() const {
  return getText(*m_ui.reductionTypeComboBox);
}

void ExperimentView::setReductionType(std::string const &reductionType) {
  return setSelected(*m_ui.reductionTypeComboBox, reductionType);
}

std::vector<std::array<std::string, 6>>
ExperimentView::getPerAngleOptions() const {
  auto const &table = *m_ui.optionsTable;
  auto rows = std::vector<std::array<std::string, 6>>();
  rows.reserve(table.rowCount());
  for (auto row = 0; row < table.rowCount(); ++row) {
    rows.emplace_back(
        std::array<std::string, 6>{table.item(row, 0)->text().toStdString(),
                                   table.item(row, 1)->text().toStdString(),
                                   table.item(row, 2)->text().toStdString(),
                                   table.item(row, 3)->text().toStdString(),
                                   table.item(row, 4)->text().toStdString(),
                                   table.item(row, 5)->text().toStdString()});
  }
  return rows;
}

void ExperimentView::showPerAngleOptionsAsInvalid(int row, int column) {
  m_ui.optionsTable->item(row, column)->setBackground(Qt::red);
}

double ExperimentView::getTransmissionStartOverlap() const {
  return m_ui.startOverlapEdit->value();
}

void ExperimentView::setTransmissionStartOverlap(double start) {
  m_ui.startOverlapEdit->setValue(start);
}

double ExperimentView::getTransmissionEndOverlap() const {
  return m_ui.endOverlapEdit->value();
}

void ExperimentView::setTransmissionEndOverlap(double end) {
  m_ui.endOverlapEdit->setValue(end);
}

void ExperimentView::setPolarisationCorrectionType(std::string const &type) {
  setSelected(*m_ui.polCorrComboBox, type);
}

std::string ExperimentView::getPolarisationCorrectionType() const {
  return getText(*m_ui.polCorrComboBox);
}

std::string ExperimentView::getStitchOptions() const {
  return getText(stitchOptionsLineEdit());
}

void ExperimentView::setStitchOptions(std::string const &stitchOptions) {
  setText(stitchOptionsLineEdit(), stitchOptions);
}

void showOptionLoadErrors(
    std::vector<InstrumentParameterTypeMissmatch> const &typeErrors,
    std::vector<MissingInstrumentParameterValue> const &missingValues);

} // namespace CustomInterfaces
} // namespace Mantid
