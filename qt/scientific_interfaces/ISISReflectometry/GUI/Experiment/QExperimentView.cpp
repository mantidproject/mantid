// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "QExperimentView.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Common/AlgorithmHintStrategy.h"
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

void showAsInvalid(QLineEdit &lineEdit) {
  auto palette = lineEdit.palette();
  palette.setColor(QPalette::Base, QColor("#ffb8ad"));
  lineEdit.setPalette(palette);
}

void showAsValid(QLineEdit &lineEdit) {
  auto palette = lineEdit.palette();
  palette.setColor(QPalette::Base, Qt::transparent);
  lineEdit.setPalette(palette);
}
} // namespace

/** Constructor
 * @param algorithmForTooltips :: [input] An algorithm that will be
 * used to find tooltips for the input properties
 * @param parent :: [input] The parent of this widget
 */
QExperimentView::QExperimentView(
    Mantid::API::IAlgorithm_sptr algorithmForTooltips, QWidget *parent)
    : QWidget(parent) {
  initLayout();
  registerSettingsWidgets(algorithmForTooltips);
}

void QExperimentView::onRemovePerThetaDefaultsRequested() {
  auto index = m_ui.optionsTable->currentIndex();
  if (index.isValid()) {
    m_notifyee->notifyRemovePerAngleDefaultsRequested(index.row());
  }
}

void QExperimentView::showAllPerAngleOptionsAsValid() {
  for (auto row = 0; row < m_ui.optionsTable->rowCount(); ++row)
    showPerAngleOptionsAsValid(row);
}

void QExperimentView::showPerAngleThetasNonUnique(double tolerance) {
  QMessageBox::critical(
      this, "Invalid theta combination!",
      "Cannot have multiple defaults with theta values less than " +
          QString::number(tolerance) + " apart.");
}

void QExperimentView::showStitchParametersValid() {
  showAsValid(stitchOptionsLineEdit());
}

void QExperimentView::showStitchParametersInvalid() {
  showAsInvalid(stitchOptionsLineEdit());
}

void QExperimentView::subscribe(ExperimentViewSubscriber *notifyee) {
  m_notifyee = notifyee;
}

/**
Initialise the Interface
*/
void QExperimentView::initLayout() {
  m_ui.setupUi(this);
  m_deleteShortcut = std::make_unique<QShortcut>(QKeySequence(tr("Delete")),
                                                 m_ui.optionsTable);
  connect(m_deleteShortcut.get(), SIGNAL(activated()), this,
          SLOT(onRemovePerThetaDefaultsRequested()));
  initOptionsTable();
  initFloodControls();

  auto blacklist =
      std::vector<std::string>({"InputWorkspaces", "OutputWorkspace"});
  MantidWidgets::AlgorithmHintStrategy strategy("Stitch1DMany", blacklist);
  createStitchHints(strategy.createHints());

  m_ui.startOverlapEdit->setSpecialValueText("Unset");
  m_ui.endOverlapEdit->setSpecialValueText("Unset");

  connect(m_ui.getExpDefaultsButton, SIGNAL(clicked()), this,
          SLOT(onRestoreDefaultsRequested()));
  connect(m_ui.addPerAngleOptionsButton, SIGNAL(clicked()), this,
          SLOT(onNewPerThetaDefaultsRowRequested()));
}

void QExperimentView::initializeTableItems(QTableWidget &table) {
  for (auto row = 0; row < table.rowCount(); ++row)
    initializeTableRow(table, row);
}

void QExperimentView::initializeTableRow(QTableWidget &table, int row) {
  m_ui.optionsTable->blockSignals(true);
  for (auto column = 0; column < table.columnCount(); ++column)
    table.setItem(row, column, new QTableWidgetItem());
  m_ui.optionsTable->blockSignals(false);
}

void QExperimentView::initializeTableRow(
    QTableWidget &table, int row, PerThetaDefaults::ValueArray rowValues) {
  m_ui.optionsTable->blockSignals(true);
  for (auto column = 0; column < table.columnCount(); ++column)
    table.setItem(
        row, column,
        new QTableWidgetItem(QString::fromStdString(rowValues[column])));
  m_ui.optionsTable->blockSignals(false);
}

void QExperimentView::initOptionsTable() {
  auto table = m_ui.optionsTable;

  // Set angle and scale columns to a small width so everything fits
  table->resizeColumnsToContents();
  table->setColumnCount(PerThetaDefaults::OPTIONS_TABLE_COLUMN_COUNT);
  table->setRowCount(1);
  initializeTableItems(*table);

  auto header = table->horizontalHeader();
  int totalRowHeight = 0;
  for (auto i = 0; i < table->rowCount(); ++i) {
    totalRowHeight += table->rowHeight(i);
  }

  const int padding = 20;
  table->setMinimumHeight(totalRowHeight + header->height() + padding);
}

void QExperimentView::initFloodControls() {
  m_ui.floodWorkspaceWsSelector->setOptional(true);
  m_ui.floodWorkspaceWsSelector->setWorkspaceTypes({"Workspace2D"});
}

void QExperimentView::connectSettingsChange(QLineEdit &edit) {
  connect(&edit, SIGNAL(textChanged(QString const &)), this,
          SLOT(onSettingsChanged()));
}

void QExperimentView::connectSettingsChange(QDoubleSpinBox &edit) {
  connect(&edit, SIGNAL(valueChanged(QString const &)), this,
          SLOT(onSettingsChanged()));
}

void QExperimentView::connectSettingsChange(QComboBox &edit) {
  connect(&edit, SIGNAL(currentIndexChanged(int)), this,
          SLOT(onSettingsChanged()));
}

void QExperimentView::connectSettingsChange(QCheckBox &edit) {
  connect(&edit, SIGNAL(stateChanged(int)), this, SLOT(onSettingsChanged()));
}

void QExperimentView::connectSettingsChange(QTableWidget &edit) {
  connect(&edit, SIGNAL(cellChanged(int, int)), this,
          SLOT(onPerAngleDefaultsChanged(int, int)));
}

void QExperimentView::disconnectSettingsChange(QLineEdit &edit) {
  disconnect(&edit, SIGNAL(textChanged(QString const &)), 0, 0);
}

void QExperimentView::disconnectSettingsChange(QDoubleSpinBox &edit) {
  disconnect(&edit, SIGNAL(valueChanged(QString const &)), 0, 0);
}

void QExperimentView::disconnectSettingsChange(QComboBox &edit) {
  disconnect(&edit, SIGNAL(currentIndexChanged(int)), 0, 0);
}

void QExperimentView::disconnectSettingsChange(QCheckBox &edit) {
  disconnect(&edit, SIGNAL(stateChanged(int)), 0, 0);
}

void QExperimentView::disconnectSettingsChange(QTableWidget &edit) {
  disconnect(&edit, SIGNAL(cellChanged(int, int)), 0, 0);
}

void QExperimentView::onSettingsChanged() {
  m_notifyee->notifySettingsChanged();
}

void QExperimentView::setEnabledStateForAllWidgets(bool enabled) {
  m_ui.optionsTable->setEnabled(enabled);
  m_ui.analysisModeComboBox->setEnabled(enabled);
  m_ui.startOverlapEdit->setEnabled(enabled);
  m_ui.endOverlapEdit->setEnabled(enabled);
  m_ui.transStitchParamsEdit->setEnabled(enabled);
  m_ui.transScaleRHSCheckBox->setEnabled(enabled);
  m_ui.polCorrCheckBox->setEnabled(enabled);
  stitchOptionsLineEdit().setEnabled(enabled);
  m_ui.reductionTypeComboBox->setEnabled(enabled);
  m_ui.summationTypeComboBox->setEnabled(enabled);
  m_ui.includePartialBinsCheckBox->setEnabled(enabled);
  m_ui.floodCorComboBox->setEnabled(enabled);
  m_ui.floodWorkspaceWsSelector->setEnabled(enabled);
  m_ui.debugCheckBox->setEnabled(enabled);
}

void QExperimentView::disableAll() { setEnabledStateForAllWidgets(false); }

void QExperimentView::enableAll() { setEnabledStateForAllWidgets(true); }

void QExperimentView::registerSettingsWidgets(
    Mantid::API::IAlgorithm_sptr alg) {
  registerExperimentSettingsWidgets(alg);
  connectExperimentSettingsWidgets();
}

void QExperimentView::registerExperimentSettingsWidgets(
    Mantid::API::IAlgorithm_sptr alg) {
  registerSettingWidget(*m_ui.analysisModeComboBox, "AnalysisMode", alg);
  registerSettingWidget(*m_ui.startOverlapEdit, "StartOverlap", alg);
  registerSettingWidget(*m_ui.endOverlapEdit, "EndOverlap", alg);
  registerSettingWidget(*m_ui.transStitchParamsEdit, "Params", alg);
  registerSettingWidget(*m_ui.transScaleRHSCheckBox, "ScaleRHSWorkspace", alg);
  registerSettingWidget(*m_ui.polCorrCheckBox, "PolarizationAnalysis", alg);
  registerSettingWidget(stitchOptionsLineEdit(), "Params", alg);
  registerSettingWidget(*m_ui.reductionTypeComboBox, "ReductionType", alg);
  registerSettingWidget(*m_ui.summationTypeComboBox, "SummationType", alg);
  registerSettingWidget(*m_ui.includePartialBinsCheckBox, "IncludePartialBins",
                        alg);
  registerSettingWidget(*m_ui.floodCorComboBox, "FloodCorrection", alg);
  registerSettingWidget(*m_ui.floodWorkspaceWsSelector, "FloodWorkspace", alg);
  registerSettingWidget(*m_ui.debugCheckBox, "Debug", alg);
}

void QExperimentView::connectExperimentSettingsWidgets() {
  connect(m_ui.summationTypeComboBox, SIGNAL(currentIndexChanged(int)), this,
          SLOT(onSummationTypeChanged(int)));
  connectSettingsChange(*m_ui.optionsTable);
  connectSettingsChange(*m_ui.analysisModeComboBox);
  connectSettingsChange(*m_ui.startOverlapEdit);
  connectSettingsChange(*m_ui.endOverlapEdit);
  connectSettingsChange(*m_ui.transStitchParamsEdit);
  connectSettingsChange(*m_ui.transScaleRHSCheckBox);
  connectSettingsChange(*m_ui.polCorrCheckBox);
  connectSettingsChange(stitchOptionsLineEdit());
  connectSettingsChange(*m_ui.reductionTypeComboBox);
  connectSettingsChange(*m_ui.includePartialBinsCheckBox);
  connectSettingsChange(*m_ui.floodCorComboBox);
  connectSettingsChange(*m_ui.floodWorkspaceWsSelector);
  connectSettingsChange(*m_ui.debugCheckBox);
}

void QExperimentView::disconnectExperimentSettingsWidgets() {
  disconnectSettingsChange(*m_ui.summationTypeComboBox);
  disconnectSettingsChange(*m_ui.optionsTable);
  disconnectSettingsChange(*m_ui.analysisModeComboBox);
  disconnectSettingsChange(*m_ui.startOverlapEdit);
  disconnectSettingsChange(*m_ui.endOverlapEdit);
  disconnectSettingsChange(*m_ui.transStitchParamsEdit);
  disconnectSettingsChange(*m_ui.transScaleRHSCheckBox);
  disconnectSettingsChange(*m_ui.polCorrCheckBox);
  disconnectSettingsChange(stitchOptionsLineEdit());
  disconnectSettingsChange(*m_ui.reductionTypeComboBox);
  disconnectSettingsChange(*m_ui.includePartialBinsCheckBox);
  disconnectSettingsChange(*m_ui.floodCorComboBox);
  disconnectSettingsChange(*m_ui.floodWorkspaceWsSelector);
  disconnectSettingsChange(*m_ui.debugCheckBox);
}

void QExperimentView::onRestoreDefaultsRequested() {
  m_notifyee->notifyRestoreDefaultsRequested();
}

void QExperimentView::onSummationTypeChanged(int reductionTypeIndex) {
  UNUSED_ARG(reductionTypeIndex);
  m_notifyee->notifySummationTypeChanged();
}

void QExperimentView::enableReductionType() {
  m_ui.reductionTypeComboBox->setEnabled(true);
}

void QExperimentView::disableReductionType() {
  m_ui.reductionTypeComboBox->setEnabled(false);
}

void QExperimentView::enableIncludePartialBins() {
  m_ui.includePartialBinsCheckBox->setEnabled(true);
}

void QExperimentView::disableIncludePartialBins() {
  m_ui.includePartialBinsCheckBox->setEnabled(false);
}

template <typename Widget>
void QExperimentView::registerSettingWidget(Widget &widget,
                                            std::string const &propertyName,
                                            Mantid::API::IAlgorithm_sptr alg) {
  setToolTipAsPropertyDocumentation(widget, propertyName, alg);
}

void QExperimentView::setToolTipAsPropertyDocumentation(
    QWidget &widget, std::string const &propertyName,
    Mantid::API::IAlgorithm_sptr alg) {
  widget.setToolTip(QString::fromStdString(
      alg->getPointerToProperty(propertyName)->documentation()));
}

void QExperimentView::setSelected(QComboBox &box, std::string const &str) {
  auto const index = box.findText(QString::fromStdString(str));
  if (index != -1)
    box.setCurrentIndex(index);
}

void QExperimentView::setText(QLineEdit &lineEdit,
                              boost::optional<double> value) {
  if (value)
    setText(lineEdit, value.get());
}

void QExperimentView::setText(QLineEdit &lineEdit, boost::optional<int> value) {
  if (value)
    setText(lineEdit, value.get());
}

void QExperimentView::setText(QLineEdit &lineEdit,
                              boost::optional<std::string> const &text) {
  if (text && !text->empty())
    setText(lineEdit, text);
}

void QExperimentView::setText(QLineEdit &lineEdit, double value) {
  auto valueAsString = QString::number(value);
  lineEdit.setText(valueAsString);
}

void QExperimentView::setText(QLineEdit &lineEdit, int value) {
  auto valueAsString = QString::number(value);
  lineEdit.setText(valueAsString);
}

void QExperimentView::setText(QLineEdit &lineEdit, std::string const &text) {
  auto textAsQString = QString::fromStdString(text);
  lineEdit.setText(textAsQString);
}

// void QExperimentView::setText(QTableWidget &table,
//                             std::string const &propertyName,
//                             boost::optional<double> value) {
//  if (value)
//    setText(table, propertyName, value.get());
//}
//
// void QExperimentView::setText(QTableWidget &table,
//                             std::string const &propertyName, double value) {
//  auto valueAsString = QString::number(value);
//  setText(table, propertyName, valueAsString);
//}
//
// void QExperimentView::setText(QTableWidget &table,
//                             std::string const &propertyName,
//                             boost::optional<std::string> text) {
//  if (text && !text->empty())
//    setText(table, propertyName, text.get());
//}
//
// void QExperimentView::setText(QTableWidget &table,
//                             std::string const &propertyName,
//                             std::string const &text) {
//  auto textAsQString = QString::fromStdString(text);
//  setText(table, propertyName, textAsQString);
//}
//
// void QExperimentView::setText(QTableWidget &table,
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

void QExperimentView::setChecked(QCheckBox &checkBox, bool checked) {
  auto checkedAsCheckState = checked ? Qt::Checked : Qt::Unchecked;
  checkBox.setCheckState(checkedAsCheckState);
}

void QExperimentView::enablePolarizationCorrections() {
  m_ui.polCorrCheckBox->setEnabled(true);
  m_ui.polCorrLabel->setEnabled(true);
}

void QExperimentView::disablePolarizationCorrections() {
  m_ui.polCorrCheckBox->setEnabled(false);
  m_ui.polCorrLabel->setEnabled(false);
}

void QExperimentView::disableFloodCorrectionInputs() {
  m_ui.floodWorkspaceWsSelector->setEnabled(false);
  m_ui.floodWorkspaceWsSelectorLabel->setEnabled(false);
}

void QExperimentView::enableFloodCorrectionInputs() {
  m_ui.floodWorkspaceWsSelector->setEnabled(true);
  m_ui.floodWorkspaceWsSelectorLabel->setEnabled(true);
}

void QExperimentView::onPerAngleDefaultsChanged(int row, int column) {
  m_notifyee->notifyPerAngleDefaultsChanged(row, column);
}

/** Add a new row to the transmission runs table **/
void QExperimentView::onNewPerThetaDefaultsRowRequested() {
  m_notifyee->notifyNewPerAngleDefaultsRequested();
}

void QExperimentView::addPerThetaDefaultsRow() {
  auto newRowIndex = m_ui.optionsTable->rowCount();
  // Select the first cell in the new row
  m_ui.optionsTable->insertRow(newRowIndex);
  initializeTableRow(*m_ui.optionsTable, newRowIndex);
  m_ui.optionsTable->setCurrentCell(newRowIndex, 0);
}

void QExperimentView::removePerThetaDefaultsRow(int rowIndex) {
  m_ui.optionsTable->removeRow(rowIndex);
}

std::string QExperimentView::getText(QLineEdit const &lineEdit) const {
  return lineEdit.text().toStdString();
}

std::string QExperimentView::getText(QComboBox const &box) const {
  return box.currentText().toStdString();
}

QString QExperimentView::messageFor(
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

QString QExperimentView::messageFor(
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

void QExperimentView::showOptionLoadErrors(
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

QLineEdit &QExperimentView::stitchOptionsLineEdit() const {
  return *static_cast<QLineEdit *>(m_stitchEdit);
}

/** Creates hints for 'Stitch1DMany'
 * @param hints :: Hints as a map
 */
void QExperimentView::createStitchHints(
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

std::string QExperimentView::getFloodCorrectionType() const {
  return getText(*m_ui.floodCorComboBox);
}

void QExperimentView::setFloodCorrectionType(std::string const &type) {
  setSelected(*m_ui.floodCorComboBox, type);
}

std::string QExperimentView::getFloodWorkspace() const {
  return getText(*m_ui.floodWorkspaceWsSelector);
}

void QExperimentView::setFloodWorkspace(std::string const &workspace) {
  setSelected(*m_ui.floodWorkspaceWsSelector, workspace);
}

std::string QExperimentView::getAnalysisMode() const {
  return getText(*m_ui.analysisModeComboBox);
}

void QExperimentView::setAnalysisMode(std::string const &analysisMode) {
  setSelected(*m_ui.analysisModeComboBox, analysisMode);
}

std::string QExperimentView::getSummationType() const {
  return getText(*m_ui.summationTypeComboBox);
}

void QExperimentView::setSummationType(std::string const &summationType) {
  return setSelected(*m_ui.summationTypeComboBox, summationType);
}

std::string QExperimentView::getReductionType() const {
  return getText(*m_ui.reductionTypeComboBox);
}

bool QExperimentView::getIncludePartialBins() const {
  return m_ui.includePartialBinsCheckBox->isChecked();
}

void QExperimentView::setIncludePartialBins(bool enable) {
  setChecked(*m_ui.includePartialBinsCheckBox, enable);
}

bool QExperimentView::getDebugOption() const {
  return m_ui.debugCheckBox->isChecked();
}

void QExperimentView::setDebugOption(bool enable) {
  setChecked(*m_ui.debugCheckBox, enable);
}

void QExperimentView::setReductionType(std::string const &reductionType) {
  return setSelected(*m_ui.reductionTypeComboBox, reductionType);
}

std::string
QExperimentView::textFromCell(QTableWidgetItem const *maybeNullItem) const {
  if (maybeNullItem != nullptr) {
    return maybeNullItem->text().toStdString();
  } else {
    return std::string();
  }
}

// The missing braces warning is a false positive -
// https://llvm.org/bugs/show_bug.cgi?id=21629
GNU_DIAG_OFF("missing-braces")
std::vector<PerThetaDefaults::ValueArray>
QExperimentView::getPerAngleOptions() const {
  auto const &table = *m_ui.optionsTable;
  auto rows = std::vector<PerThetaDefaults::ValueArray>();
  rows.reserve(table.rowCount());
  for (auto row = 0; row < table.rowCount(); ++row) {
    rows.emplace_back(PerThetaDefaults::ValueArray{
        textFromCell(table.item(row, 0)), textFromCell(table.item(row, 1)),
        textFromCell(table.item(row, 2)), textFromCell(table.item(row, 3)),
        textFromCell(table.item(row, 4)), textFromCell(table.item(row, 5)),
        textFromCell(table.item(row, 6)), textFromCell(table.item(row, 7)),
        textFromCell(table.item(row, 8))});
  }
  return rows;
}
GNU_DIAG_ON("missing-braces")

void QExperimentView::setPerAngleOptions(
    std::vector<PerThetaDefaults::ValueArray> rows) {
  auto &table = *m_ui.optionsTable;
  table.blockSignals(true);
  auto numberOfRows = static_cast<int>(rows.size());
  table.setRowCount(numberOfRows);
  for (auto row = 0; row < numberOfRows; ++row) {
    initializeTableRow(table, row, rows[row]);
  }
  table.resizeColumnsToContents();
  table.blockSignals(false);
}

void QExperimentView::showPerAngleOptionsAsInvalid(int row, int column) {
  m_ui.optionsTable->blockSignals(true);
  m_ui.optionsTable->item(row, column)->setBackground(QColor("#ffb8ad"));
  m_ui.optionsTable->blockSignals(false);
}

void QExperimentView::showPerAngleOptionsAsValid(int row) {
  m_ui.optionsTable->blockSignals(true);
  for (auto column = 0; column < m_ui.optionsTable->columnCount(); ++column)
    m_ui.optionsTable->item(row, column)->setBackground(Qt::transparent);
  m_ui.optionsTable->blockSignals(false);
}

double QExperimentView::getTransmissionStartOverlap() const {
  return m_ui.startOverlapEdit->value();
}

void QExperimentView::setTransmissionStartOverlap(double start) {
  m_ui.startOverlapEdit->setValue(start);
}

double QExperimentView::getTransmissionEndOverlap() const {
  return m_ui.endOverlapEdit->value();
}

void QExperimentView::setTransmissionEndOverlap(double end) {
  m_ui.endOverlapEdit->setValue(end);
}

std::string QExperimentView::getTransmissionStitchParams() const {
  return getText(*m_ui.transStitchParamsEdit);
}

void QExperimentView::setTransmissionStitchParams(std::string const &params) {
  setText(*m_ui.transStitchParamsEdit, params);
}

bool QExperimentView::getTransmissionScaleRHSWorkspace() const {
  return m_ui.transScaleRHSCheckBox->isChecked();
}

void QExperimentView::setTransmissionScaleRHSWorkspace(bool enable) {
  setChecked(*m_ui.transScaleRHSCheckBox, enable);
}

void QExperimentView::showTransmissionRangeInvalid() {
  showAsInvalid(*m_ui.startOverlapEdit);
  showAsInvalid(*m_ui.endOverlapEdit);
}

void QExperimentView::showTransmissionRangeValid() {
  showAsValid(*m_ui.startOverlapEdit);
  showAsValid(*m_ui.endOverlapEdit);
}

void QExperimentView::showTransmissionStitchParamsValid() {
  showAsValid(*m_ui.transStitchParamsEdit);
}

void QExperimentView::showTransmissionStitchParamsInvalid() {
  showAsInvalid(*m_ui.transStitchParamsEdit);
}

void QExperimentView::setPolarizationCorrectionOption(bool enable) {
  setChecked(*m_ui.polCorrCheckBox, enable);
}

bool QExperimentView::getPolarizationCorrectionOption() const {
  return m_ui.polCorrCheckBox->isChecked();
}

std::string QExperimentView::getStitchOptions() const {
  return getText(stitchOptionsLineEdit());
}

void QExperimentView::setStitchOptions(std::string const &stitchOptions) {
  setText(stitchOptionsLineEdit(), stitchOptions);
}

void showOptionLoadErrors(
    std::vector<InstrumentParameterTypeMissmatch> const &typeErrors,
    std::vector<MissingInstrumentParameterValue> const &missingValues);

} // namespace CustomInterfaces
} // namespace MantidQt
