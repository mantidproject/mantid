// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "QtExperimentView.h"
#include "MantidKernel/UsageService.h"
#include "MantidQtWidgets/Common/AlgorithmHintStrategy.h"
#include "Reduction/LookupRow.h"
#include <QMessageBox>
#include <QScrollBar>
#include <boost/algorithm/string/join.hpp>
#include <utility>

namespace MantidQt::CustomInterfaces::ISISReflectometry {

namespace {
// Map of column number to hard-coded tooltips (used for lookup criteria columns)
std::unordered_map<int, std::string> ColumnTooltips{
    {LookupRow::THETA,
     "Theta lookup: runs with theta within 0.01 of this value will use the settings specified in this row"},
    {LookupRow::TITLE,
     "Title lookup: runs with a title matching this regex will use the settings specified in this row"}};

// Map of column number to algorithm property name for columns where we want to get the tooltip from the algorithm
std::unordered_map<int, std::string> ColumnPropertyNames{
    {LookupRow::FIRST_TRANS, "FirstTransmissionRunList"},
    {LookupRow::SECOND_TRANS, "SecondTransmissionRunList"},
    {LookupRow::TRANS_SPECTRA, "TransmissionProcessingInstructions"},
    {LookupRow::QMIN, "MomentumTransferMin"},
    {LookupRow::QMAX, "MomentumTransferMax"},
    {LookupRow::QSTEP, "MomentumTransferStep"},
    {LookupRow::SCALE, "ScaleFactor"},
    {LookupRow::RUN_SPECTRA, "ProcessingInstructions"},
    {LookupRow::BACKGROUND_SPECTRA, "BackgroundProcessingInstructions"},
    {LookupRow::ROI_DETECTOR_IDS, "ROIDetectorIDs"}};

// Changing the palette for spin boxes doesn't work but we can
// change the background colour with a style sheet. This also changes
// the font slightly on Ubuntu so there may be a better way to do this,
// but it's not a big issue so this should be fine for now.
void showAsInvalid(QDoubleSpinBox &spinBox) { spinBox.setStyleSheet("QDoubleSpinBox { background-color: #ffb8ad; }"); }

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
constexpr auto POL_CORR_SELECTOR_ROW = 12;
constexpr auto POL_CORR_SELECTOR_COL = 3;

constexpr auto FLOOD_SELECTOR_ROW = 14;
constexpr auto FLOOD_SELECTOR_COL = 3;
} // namespace

/** Constructor
 * @param algorithmForTooltips :: [input] An algorithm that will be
 * used to find tooltips for the input properties
 * @param parent :: [input] The parent of this widget
 */
QtExperimentView::QtExperimentView(const Mantid::API::IAlgorithm_sptr &algorithmForTooltips, QWidget *parent)
    : QWidget(parent), m_stitchEdit(nullptr), m_deleteShortcut(), m_notifyee(nullptr), m_columnToolTips() {
  initLayout(algorithmForTooltips);
  registerSettingsWidgets(algorithmForTooltips);
}

void QtExperimentView::onRemoveLookupRowRequested() {
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      Mantid::Kernel::FeatureType::Feature, {"ISIS Reflectometry", "ExperimentTab", "RemoveLookupRow"}, false);
  auto index = m_ui.optionsTable->currentIndex();
  if (index.isValid()) {
    m_notifyee->notifyRemoveLookupRowRequested(index.row());
  }
}

void QtExperimentView::showAllLookupRowsAsValid() {
  for (auto row = 0; row < m_ui.optionsTable->rowCount(); ++row) {
    showLookupRowAsValid(row);
  }
}

void QtExperimentView::showStitchParametersValid() { showAsValid(stitchOptionsLineEdit()); }

void QtExperimentView::showStitchParametersInvalid() { showAsInvalid(stitchOptionsLineEdit()); }

void QtExperimentView::showPolCorrFilePathValid() { showAsValid(*m_polCorrEfficienciesLineEdit); }

void QtExperimentView::showPolCorrFilePathInvalid() { showAsInvalid(*m_polCorrEfficienciesLineEdit); }

void QtExperimentView::showFloodCorrFilePathValid() { showAsValid(*m_floodCorrLineEdit); }

void QtExperimentView::showFloodCorrFilePathInvalid() { showAsInvalid(*m_floodCorrLineEdit); }

void QtExperimentView::subscribe(ExperimentViewSubscriber *notifyee) { m_notifyee = notifyee; }

/**
Initialise the Interface
*/
void QtExperimentView::initLayout(const Mantid::API::IAlgorithm_sptr &algorithmForTooltips) {
  m_ui.setupUi(this);
  m_deleteShortcut = std::make_unique<QShortcut>(QKeySequence(tr("Delete")), m_ui.optionsTable);
  connect(m_deleteShortcut.get(), SIGNAL(activated()), this, SLOT(onRemoveLookupRowRequested()));
  initOptionsTable(algorithmForTooltips);
  initFloodControls();
  initPolCorrEfficienciesControls();

  auto blacklist = std::vector<std::string>({"InputWorkspaces", "OutputWorkspace", "ScaleRHSWorkspace"});
  MantidWidgets::AlgorithmHintStrategy strategy("Stitch1DMany", blacklist);
  createStitchHints(strategy.createHints());

  m_ui.startOverlapEdit->setSpecialValueText("Unset");
  m_ui.endOverlapEdit->setSpecialValueText("Unset");

  connect(m_ui.getExpDefaultsButton, SIGNAL(clicked()), this, SLOT(onRestoreDefaultsRequested()));
  connect(m_ui.addPerAngleOptionsButton, SIGNAL(clicked()), this, SLOT(onNewLookupRowRequested()));
}

/** Set a column tooltip from a map, if it exists
 *
 * @param column : the column index
 * @param tooltips : a map of column index to tooltip
 * @return : true if the tooltip was set, false if not found
 */
bool QtExperimentView::setTooltipFromMap(int column, std::unordered_map<int, std::string> const &tooltips) {
  auto tooltipIt = tooltips.find(column);
  if (tooltipIt == tooltips.end()) {
    return false;
  }
  m_columnToolTips[column] = QString::fromStdString(tooltipIt->second);
  return true;
}

/** Set a column tooltip from an algorithm property. Does nothing if the property is not found
 *
 * @param column : the column index
 * @param properties : a map of column index to algorithm property name
 */
void QtExperimentView::setTooltipFromAlgorithm(int column, std::unordered_map<int, std::string> const &properties,
                                               const Mantid::API::IAlgorithm_sptr &algorithmForTooltips) {
  auto propertyIt = properties.find(column);
  if (propertyIt == properties.end()) {
    return;
  }
  // Get the tooltip for this column based on the algorithm property of the same name
  auto const toolTip =
      QString::fromStdString(algorithmForTooltips->getPointerToProperty(propertyIt->second)->documentation());
  // We could set the tooltip for the column header here using
  // horizontalHeaderItem(column)->setToolTip(). However, then we lose the
  // tooltip about the purpose of the table as a whole. So we set the tooltip
  // on the table cells instead. They are created dynamically, so for now
  // just cache the tooltip.
  m_columnToolTips[column] = toolTip;
}

void QtExperimentView::setTooltip(int row, int column, std::string const &text) {
  m_ui.optionsTable->blockSignals(true);
  m_ui.optionsTable->item(row, column)->setToolTip(QString::fromStdString(text));
  m_ui.optionsTable->blockSignals(false);
}

void QtExperimentView::initializeTableColumns(QTableWidget &table,
                                              const Mantid::API::IAlgorithm_sptr &algorithmForTooltips) {
  for (auto column = 0; column < table.columnCount(); ++column) {
    // First check if there's a tooltip for the column
    if (!setTooltipFromMap(column, ColumnTooltips)) {
      // Otherwise, get the tooltip from the algorithm property
      setTooltipFromAlgorithm(column, ColumnPropertyNames, algorithmForTooltips);
    }
  }
}

void QtExperimentView::initializeTableItems(QTableWidget &table) {
  for (auto row = 0; row < table.rowCount(); ++row)
    initializeTableRow(table, row);
}

void QtExperimentView::initializeTableRow(QTableWidget &table, int row) {
  m_ui.optionsTable->blockSignals(true);
  for (auto column = 0; column < table.columnCount(); ++column) {
    auto item = new QTableWidgetItem();
    table.setItem(row, column, item);
    item->setToolTip(m_columnToolTips[column]);
  }
  m_ui.optionsTable->blockSignals(false);
}

void QtExperimentView::initializeTableRow(QTableWidget &table, int row, LookupRow::ValueArray rowValues) {
  m_ui.optionsTable->blockSignals(true);
  for (auto column = 0; column < table.columnCount(); ++column) {
    auto item = new QTableWidgetItem(QString::fromStdString(rowValues[column]));
    table.setItem(row, column, item);
    item->setToolTip(m_columnToolTips[column]);
  }
  m_ui.optionsTable->blockSignals(false);
}

void QtExperimentView::initOptionsTable(const Mantid::API::IAlgorithm_sptr &algorithmForTooltips) {
  auto table = m_ui.optionsTable;

  // Set angle and scale columns to a small width so everything fits
  table->resizeColumnsToContents();
  table->setColumnCount(LookupRow::OPTIONS_TABLE_COLUMN_COUNT);
  table->setRowCount(1);
  initializeTableColumns(*table, algorithmForTooltips);
  initializeTableItems(*table);

  auto header = table->horizontalHeader();
  int totalRowHeight = 0;
  for (auto i = 0; i < table->rowCount(); ++i) {
    totalRowHeight += table->rowHeight(i);
  }

  const int padding = 20;
  table->setMinimumHeight(totalRowHeight + header->height() + padding);
}

void QtExperimentView::initPolCorrEfficienciesControls() {
  m_polCorrEfficienciesWsSelector =
      std::make_unique<MantidWidgets::WorkspaceSelector>(new MantidWidgets::WorkspaceSelector);
  m_polCorrEfficienciesLineEdit = std::make_unique<QLineEdit>(new QLineEdit());
  m_ui.expSettingsGrid->addWidget(m_polCorrEfficienciesWsSelector.get(), POL_CORR_SELECTOR_ROW, POL_CORR_SELECTOR_COL);
  m_polCorrEfficienciesWsSelector->setOptional(true);
  m_polCorrEfficienciesWsSelector->setWorkspaceTypes({"Workspace2D"});
}

void QtExperimentView::initFloodControls() {
  m_floodCorrWsSelector = std::make_unique<MantidWidgets::WorkspaceSelector>(new MantidWidgets::WorkspaceSelector);
  m_floodCorrLineEdit = std::make_unique<QLineEdit>(new QLineEdit());
  m_ui.expSettingsGrid->addWidget(m_floodCorrWsSelector.get(), FLOOD_SELECTOR_ROW, FLOOD_SELECTOR_COL);
  m_floodCorrWsSelector->setOptional(true);
  m_floodCorrWsSelector->setWorkspaceTypes({"Workspace2D"});
}

void QtExperimentView::connectSettingsChange(QLineEdit &edit) {
  connect(&edit, SIGNAL(textChanged(QString const &)), this, SLOT(onSettingsChanged()));
}

void QtExperimentView::connectSettingsChange(QSpinBox &edit) {
  connect(&edit, SIGNAL(valueChanged(QString const &)), this, SLOT(onSettingsChanged()));
}

void QtExperimentView::connectSettingsChange(QDoubleSpinBox &edit) {
  connect(&edit, SIGNAL(valueChanged(QString const &)), this, SLOT(onSettingsChanged()));
}

void QtExperimentView::connectSettingsChange(QComboBox &edit) {
  connect(&edit, SIGNAL(currentIndexChanged(int)), this, SLOT(onSettingsChanged()));
}

void QtExperimentView::connectSettingsChange(QCheckBox &edit) {
  connect(&edit, SIGNAL(stateChanged(int)), this, SLOT(onSettingsChanged()));
}

void QtExperimentView::connectSettingsChange(QTableWidget &edit) {
  connect(&edit, SIGNAL(cellChanged(int, int)), this, SLOT(onLookupRowChanged(int, int)));
}

void QtExperimentView::disconnectSettingsChange(QLineEdit &edit) {
  disconnect(&edit, SIGNAL(textChanged(QString const &)), 0, 0);
}

void QtExperimentView::disconnectSettingsChange(QSpinBox &edit) {
  disconnect(&edit, SIGNAL(valueChanged(QString const &)), 0, 0);
}

void QtExperimentView::disconnectSettingsChange(QDoubleSpinBox &edit) {
  disconnect(&edit, SIGNAL(valueChanged(QString const &)), 0, 0);
}

void QtExperimentView::disconnectSettingsChange(QComboBox &edit) {
  disconnect(&edit, SIGNAL(currentIndexChanged(int)), 0, 0);
}

void QtExperimentView::disconnectSettingsChange(QCheckBox &edit) { disconnect(&edit, SIGNAL(stateChanged(int)), 0, 0); }

void QtExperimentView::disconnectSettingsChange(QTableWidget &edit) {
  disconnect(&edit, SIGNAL(cellChanged(int, int)), 0, 0);
}

void QtExperimentView::onSettingsChanged() { m_notifyee->notifySettingsChanged(); }

void QtExperimentView::setEnabledStateForAllWidgets(bool enabled) {
  m_ui.optionsTable->setEnabled(enabled);
  m_ui.analysisModeComboBox->setEnabled(enabled);
  m_ui.startOverlapEdit->setEnabled(enabled);
  m_ui.endOverlapEdit->setEnabled(enabled);
  m_ui.transStitchParamsEdit->setEnabled(enabled);
  m_ui.transScaleRHSCheckBox->setEnabled(enabled);
  m_polCorrEfficienciesWsSelector->setEnabled(enabled);
  m_polCorrEfficienciesLineEdit->setEnabled(enabled);
  m_ui.polCorrFredrikzeSpinStateEdit->setEnabled(enabled);
  stitchOptionsLineEdit().setEnabled(enabled);
  m_ui.reductionTypeComboBox->setEnabled(enabled);
  m_ui.summationTypeComboBox->setEnabled(enabled);
  m_ui.includePartialBinsCheckBox->setEnabled(enabled);
  m_ui.floodCorComboBox->setEnabled(enabled);
  m_floodCorrWsSelector->setEnabled(enabled);
  m_floodCorrLineEdit->setEnabled(enabled);
  m_ui.debugCheckBox->setEnabled(enabled);
  m_ui.subtractBackgroundCheckBox->setEnabled(enabled);
  m_ui.backgroundMethodComboBox->setEnabled(enabled);
  m_ui.polynomialDegreeSpinBox->setEnabled(enabled);
  m_ui.costFunctionComboBox->setEnabled(enabled);
  m_ui.addPerAngleOptionsButton->setEnabled(enabled);
}

void QtExperimentView::disableAll() { setEnabledStateForAllWidgets(false); }

void QtExperimentView::enableAll() { setEnabledStateForAllWidgets(true); }

void QtExperimentView::registerSettingsWidgets(const Mantid::API::IAlgorithm_sptr &alg) {
  registerExperimentSettingsWidgets(alg);
  connectExperimentSettingsWidgets();
}

void QtExperimentView::registerExperimentSettingsWidgets(const Mantid::API::IAlgorithm_sptr &alg) {
  registerSettingWidget(*m_ui.analysisModeComboBox, "AnalysisMode", alg);
  registerSettingWidget(*m_ui.startOverlapEdit, "StartOverlap", alg);
  registerSettingWidget(*m_ui.endOverlapEdit, "EndOverlap", alg);
  registerSettingWidget(*m_ui.transStitchParamsEdit, "Params", alg);
  registerSettingWidget(*m_ui.transScaleRHSCheckBox, "ScaleRHSWorkspace", alg);
  registerSettingWidget(*m_ui.polCorrComboBox, "PolarizationAnalysis", alg);
  registerSettingWidget(*m_polCorrEfficienciesWsSelector, "PolarizationEfficiencies", alg);
  registerSettingWidget(*m_polCorrEfficienciesLineEdit, "PolarizationEfficiencies", alg);
  registerSettingWidget(*m_ui.polCorrFredrikzeSpinStateEdit, "FredrikzePolarizationEfficienciesSpinStateOrder", alg);
  registerSettingWidget(*m_ui.reductionTypeComboBox, "ReductionType", alg);
  registerSettingWidget(*m_ui.summationTypeComboBox, "SummationType", alg);
  registerSettingWidget(*m_ui.includePartialBinsCheckBox, "IncludePartialBins", alg);
  registerSettingWidget(*m_ui.floodCorComboBox, "FloodCorrection", alg);
  registerSettingWidget(*m_floodCorrWsSelector, "FloodWorkspace", alg);
  registerSettingWidget(*m_floodCorrLineEdit, "FloodWorkspace", alg);
  registerSettingWidget(*m_ui.debugCheckBox, "Debug", alg);
  registerSettingWidget(*m_ui.subtractBackgroundCheckBox, "SubtractBackground", alg);
  registerSettingWidget(*m_ui.backgroundMethodComboBox, "BackgroundCalculationMethod", alg);
  registerSettingWidget(*m_ui.polynomialDegreeSpinBox, "DegreeOfPolynomial", alg);
  registerSettingWidget(*m_ui.costFunctionComboBox, "CostFunction", alg);

  registerSettingWidget(stitchOptionsLineEdit(), "Properties to use for stitching the output workspaces "
                                                 "in Q. Only required for groups containing multiple "
                                                 "rows. Start typing to see property hints or see "
                                                 "Stitch1DMany for details.");
}

void QtExperimentView::connectExperimentSettingsWidgets() {
  connect(m_ui.summationTypeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onSummationTypeChanged(int)));
  connectSettingsChange(*m_ui.optionsTable);
  connectSettingsChange(*m_ui.analysisModeComboBox);
  connectSettingsChange(*m_ui.startOverlapEdit);
  connectSettingsChange(*m_ui.endOverlapEdit);
  connectSettingsChange(*m_ui.transStitchParamsEdit);
  connectSettingsChange(*m_ui.transScaleRHSCheckBox);
  connectSettingsChange(*m_ui.polCorrComboBox);
  connectSettingsChange(*m_polCorrEfficienciesWsSelector);
  connectSettingsChange(*m_polCorrEfficienciesLineEdit);
  connectSettingsChange(*m_ui.polCorrFredrikzeSpinStateEdit);
  connectSettingsChange(stitchOptionsLineEdit());
  connectSettingsChange(*m_ui.reductionTypeComboBox);
  connectSettingsChange(*m_ui.includePartialBinsCheckBox);
  connectSettingsChange(*m_ui.floodCorComboBox);
  connectSettingsChange(*m_floodCorrWsSelector);
  connectSettingsChange(*m_floodCorrLineEdit);
  connectSettingsChange(*m_ui.debugCheckBox);
  connectSettingsChange(*m_ui.subtractBackgroundCheckBox);
  connectSettingsChange(*m_ui.backgroundMethodComboBox);
  connectSettingsChange(*m_ui.polynomialDegreeSpinBox);
  connectSettingsChange(*m_ui.costFunctionComboBox);
}

void QtExperimentView::disconnectExperimentSettingsWidgets() {
  disconnectSettingsChange(*m_ui.summationTypeComboBox);
  disconnectSettingsChange(*m_ui.optionsTable);
  disconnectSettingsChange(*m_ui.analysisModeComboBox);
  disconnectSettingsChange(*m_ui.startOverlapEdit);
  disconnectSettingsChange(*m_ui.endOverlapEdit);
  disconnectSettingsChange(*m_ui.transStitchParamsEdit);
  disconnectSettingsChange(*m_ui.transScaleRHSCheckBox);
  disconnectSettingsChange(*m_ui.polCorrComboBox);
  disconnectSettingsChange(*m_polCorrEfficienciesWsSelector);
  disconnectSettingsChange(*m_polCorrEfficienciesLineEdit);
  disconnectSettingsChange(*m_ui.polCorrFredrikzeSpinStateEdit);
  disconnectSettingsChange(stitchOptionsLineEdit());
  disconnectSettingsChange(*m_ui.reductionTypeComboBox);
  disconnectSettingsChange(*m_ui.includePartialBinsCheckBox);
  disconnectSettingsChange(*m_ui.floodCorComboBox);
  disconnectSettingsChange(*m_floodCorrWsSelector);
  disconnectSettingsChange(*m_floodCorrLineEdit);
  disconnectSettingsChange(*m_ui.debugCheckBox);
  disconnectSettingsChange(*m_ui.subtractBackgroundCheckBox);
  disconnectSettingsChange(*m_ui.backgroundMethodComboBox);
  disconnectSettingsChange(*m_ui.polynomialDegreeSpinBox);
  disconnectSettingsChange(*m_ui.costFunctionComboBox);
}

void QtExperimentView::onRestoreDefaultsRequested() {
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      Mantid::Kernel::FeatureType::Feature, {"ISIS Reflectometry", "ExperimentTab", "RestoreDefaults"}, false);
  m_notifyee->notifyRestoreDefaultsRequested();
}

void QtExperimentView::onSummationTypeChanged(int reductionTypeIndex) {
  UNUSED_ARG(reductionTypeIndex);
  m_notifyee->notifySummationTypeChanged();
}

void QtExperimentView::enableReductionType() { m_ui.reductionTypeComboBox->setEnabled(true); }

void QtExperimentView::disableReductionType() { m_ui.reductionTypeComboBox->setEnabled(false); }

void QtExperimentView::enableIncludePartialBins() { m_ui.includePartialBinsCheckBox->setEnabled(true); }

void QtExperimentView::disableIncludePartialBins() { m_ui.includePartialBinsCheckBox->setEnabled(false); }

template <typename Widget>
void QtExperimentView::registerSettingWidget(Widget &widget, std::string const &propertyName,
                                             const Mantid::API::IAlgorithm_sptr &alg) {
  setToolTipAsPropertyDocumentation(widget, propertyName, alg);
}

template <typename Widget> void QtExperimentView::registerSettingWidget(Widget &widget, std::string const &tooltip) {
  widget.setToolTip(QString::fromStdString(tooltip));
}

void QtExperimentView::setToolTipAsPropertyDocumentation(QWidget &widget, std::string const &propertyName,
                                                         const Mantid::API::IAlgorithm_sptr &alg) {
  widget.setToolTip(QString::fromStdString(alg->getPointerToProperty(propertyName)->documentation()));
}

void QtExperimentView::setSelected(QComboBox &box, std::string const &str) {
  auto const index = box.findText(QString::fromStdString(str));
  if (index != -1)
    box.setCurrentIndex(index);
}

void QtExperimentView::setText(QLineEdit &lineEdit, boost::optional<double> value) {
  if (value)
    setText(lineEdit, value.get());
}

void QtExperimentView::setText(QLineEdit &lineEdit, std::optional<int> value) {
  if (value)
    setText(lineEdit, value.value());
}

void QtExperimentView::setText(QLineEdit &lineEdit, boost::optional<std::string> const &text) {
  if (text && !text->empty())
    setText(lineEdit, text);
}

void QtExperimentView::setText(QLineEdit &lineEdit, double value) {
  auto valueAsString = QString::number(value);
  lineEdit.setText(valueAsString);
}

void QtExperimentView::setText(QLineEdit &lineEdit, int value) {
  auto valueAsString = QString::number(value);
  lineEdit.setText(valueAsString);
}

void QtExperimentView::setText(QLineEdit &lineEdit, std::string const &text) {
  auto textAsQString = QString::fromStdString(text);
  lineEdit.setText(textAsQString);
}

// void QtExperimentView::setText(QTableWidget &table,
//                             std::string const &propertyName,
//                             boost::optional<double> value) {
//  if (value)
//    setText(table, propertyName, value.get());
//}
//
// void QtExperimentView::setText(QTableWidget &table,
//                             std::string const &propertyName, double value) {
//  auto valueAsString = QString::number(value);
//  setText(table, propertyName, valueAsString);
//}
//
// void QtExperimentView::setText(QTableWidget &table,
//                             std::string const &propertyName,
//                             boost::optional<std::string> text) {
//  if (text && !text->empty())
//    setText(table, propertyName, text.get());
//}
//
// void QtExperimentView::setText(QTableWidget &table,
//                             std::string const &propertyName,
//                             std::string const &text) {
//  auto textAsQString = QString::fromStdString(text);
//  setText(table, propertyName, textAsQString);
//}
//
// void QtExperimentView::setText(QTableWidget &table,
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

void QtExperimentView::setChecked(QCheckBox &checkBox, bool checked) {
  auto checkedAsCheckState = checked ? Qt::Checked : Qt::Unchecked;
  checkBox.setCheckState(checkedAsCheckState);
}

bool QtExperimentView::getSubtractBackground() const { return m_ui.subtractBackgroundCheckBox->isChecked(); }

void QtExperimentView::setSubtractBackground(bool enable) { setChecked(*m_ui.subtractBackgroundCheckBox, enable); }

std::string QtExperimentView::getBackgroundSubtractionMethod() const { return getText(*m_ui.backgroundMethodComboBox); }

void QtExperimentView::setBackgroundSubtractionMethod(std::string const &method) {
  return setSelected(*m_ui.backgroundMethodComboBox, method);
}

void QtExperimentView::enableBackgroundSubtractionMethod() { m_ui.backgroundMethodComboBox->setEnabled(true); }

void QtExperimentView::disableBackgroundSubtractionMethod() { m_ui.backgroundMethodComboBox->setEnabled(false); }

int QtExperimentView::getPolynomialDegree() const { return m_ui.polynomialDegreeSpinBox->value(); }

void QtExperimentView::setPolynomialDegree(int polynomialDegree) {
  m_ui.polynomialDegreeSpinBox->setValue(polynomialDegree);
}

void QtExperimentView::enablePolynomialDegree() { m_ui.polynomialDegreeSpinBox->setEnabled(true); }

void QtExperimentView::disablePolynomialDegree() { m_ui.polynomialDegreeSpinBox->setEnabled(false); }

std::string QtExperimentView::getCostFunction() const { return getText(*m_ui.costFunctionComboBox); }

void QtExperimentView::setCostFunction(std::string const &costFunction) {
  setSelected(*m_ui.costFunctionComboBox, costFunction);
}

void QtExperimentView::enableCostFunction() { m_ui.costFunctionComboBox->setEnabled(true); }

void QtExperimentView::disableCostFunction() { m_ui.costFunctionComboBox->setEnabled(false); }

void QtExperimentView::enablePolarizationCorrections() {
  m_ui.polCorrComboBox->setEnabled(true);
  m_ui.polCorrComboLabel->setEnabled(true);
}

void QtExperimentView::disablePolarizationCorrections() {
  m_ui.polCorrComboBox->setEnabled(false);
  m_ui.polCorrComboLabel->setEnabled(false);
}

void QtExperimentView::enablePolarizationEfficiencies() {
  m_polCorrEfficienciesWsSelector->setEnabled(true);
  m_polCorrEfficienciesLineEdit->setEnabled(true);
  m_ui.polCorrEfficienciesLabel->setEnabled(true);
}

void QtExperimentView::disablePolarizationEfficiencies() {
  m_polCorrEfficienciesWsSelector->setEnabled(false);
  m_polCorrEfficienciesLineEdit->setEnabled(false);
  m_ui.polCorrEfficienciesLabel->setEnabled(false);
}

void QtExperimentView::enableFredrikzeSpinStateOrder() {
  m_ui.polCorrFredrikzeSpinStateLabel->setEnabled(true);
  m_ui.polCorrFredrikzeSpinStateLabel->setEnabled(true);
}

void QtExperimentView::disableFredrikzeSpinStateOrder() {
  m_ui.polCorrFredrikzeSpinStateLabel->setEnabled(false);
  m_ui.polCorrFredrikzeSpinStateLabel->setEnabled(false);
}

void QtExperimentView::disableFloodCorrectionInputs() {
  m_floodCorrWsSelector->setEnabled(false);
  m_floodCorrLineEdit->setEnabled(false);
  m_ui.floodWorkspaceWsSelectorLabel->setEnabled(false);
}

void QtExperimentView::enableFloodCorrectionInputs() {
  m_floodCorrWsSelector->setEnabled(true);
  m_floodCorrLineEdit->setEnabled(true);
  m_ui.floodWorkspaceWsSelectorLabel->setEnabled(true);
}

void QtExperimentView::onLookupRowChanged(int row, int column) { m_notifyee->notifyLookupRowChanged(row, column); }

/** Add a new row to the transmission runs table **/
void QtExperimentView::onNewLookupRowRequested() {
  Mantid::Kernel::UsageService::Instance().registerFeatureUsage(
      Mantid::Kernel::FeatureType::Feature, {"ISIS Reflectometry", "ExperimentTab", "AddLookupRow"}, false);
  m_notifyee->notifyNewLookupRowRequested();
}

void QtExperimentView::addLookupRow() {
  auto newRowIndex = m_ui.optionsTable->rowCount();
  // Select the first cell in the new row
  m_ui.optionsTable->insertRow(newRowIndex);
  initializeTableRow(*m_ui.optionsTable, newRowIndex);
  m_ui.optionsTable->setCurrentCell(newRowIndex, 0);
}

void QtExperimentView::removeLookupRow(int rowIndex) { m_ui.optionsTable->removeRow(rowIndex); }

std::string QtExperimentView::getText(QLineEdit const &lineEdit) const { return lineEdit.text().toStdString(); }

std::string QtExperimentView::getText(QComboBox const &box) const { return box.currentText().toStdString(); }

QString QtExperimentView::messageFor(InstrumentParameterTypeMissmatch const &typeError) const {
  return QString::fromStdString(typeError.parameterName()) + " should hold an " +
         QString::fromStdString(typeError.expectedType()) + " value but does not.\n";
}

template <typename T, typename StringConverter>
std::string toCsv(std::vector<T> const &values, StringConverter toString) {
  std::vector<std::string> valuesAsStrings;
  valuesAsStrings.reserve(values.size());
  std::transform(values.cbegin(), values.cend(), std::back_inserter(valuesAsStrings), toString);
  return boost::algorithm::join(valuesAsStrings, ", ");
}

QString QtExperimentView::messageFor(std::vector<MissingInstrumentParameterValue> const &missingValues) const {
  auto missingNamesCsv = toCsv(missingValues, [](const MissingInstrumentParameterValue &missingValue) -> std::string {
    return missingValue.parameterName();
  });

  return QString::fromStdString(missingNamesCsv) + QString(missingValues.size() == 1 ? " is" : " are") +
         " not set in the instrument parameter file but should be.\n";
}

QLineEdit &QtExperimentView::stitchOptionsLineEdit() const { return *static_cast<QLineEdit *>(m_stitchEdit); }

/** Creates hints for 'Stitch1DMany'
 * @param hints :: Hints as a map
 */
void QtExperimentView::createStitchHints(const std::vector<MantidWidgets::Hint> &hints) {

  // We want to add the stitch params box next to the stitch
  // label, so first find the label's position
  auto stitchLabelIndex = m_ui.expSettingsGrid->indexOf(m_ui.stitchLabel);
  int row, col, rowSpan, colSpan;
  m_ui.expSettingsGrid->getItemPosition(stitchLabelIndex, &row, &col, &rowSpan, &colSpan);
  // Create the new edit box and add it to the right of the label
  m_stitchEdit = new MantidWidgets::HintingLineEdit(this, hints);
  m_ui.expSettingsGrid->addWidget(m_stitchEdit, row, col + colSpan, 1, 3);
}

std::string QtExperimentView::getFloodCorrectionType() const { return getText(*m_ui.floodCorComboBox); }

void QtExperimentView::setFloodCorrectionType(std::string const &type) { setSelected(*m_ui.floodCorComboBox, type); }

void QtExperimentView::setFloodCorrectionWorkspaceMode() {
  m_ui.expSettingsGrid->removeItem(m_ui.expSettingsGrid->itemAtPosition(FLOOD_SELECTOR_ROW, FLOOD_SELECTOR_COL));
  m_floodCorrWsSelector->show();
  m_floodCorrLineEdit->hide();
  m_ui.expSettingsGrid->addWidget(m_floodCorrWsSelector.get(), FLOOD_SELECTOR_ROW, FLOOD_SELECTOR_COL);
}

void QtExperimentView::setFloodCorrectionFilePathMode() {
  m_ui.expSettingsGrid->removeItem(m_ui.expSettingsGrid->itemAtPosition(FLOOD_SELECTOR_ROW, FLOOD_SELECTOR_COL));
  m_floodCorrWsSelector->hide();
  m_floodCorrLineEdit->show();
  m_ui.expSettingsGrid->addWidget(m_floodCorrLineEdit.get(), FLOOD_SELECTOR_ROW, FLOOD_SELECTOR_COL);
}
std::string QtExperimentView::getFloodWorkspace() const { return getText(*m_floodCorrWsSelector); }

std::string QtExperimentView::getFloodFilePath() const { return getText(*m_floodCorrLineEdit); }

void QtExperimentView::setFloodWorkspace(std::string const &workspace) {
  setSelected(*m_floodCorrWsSelector, workspace);
}

void QtExperimentView::setFloodFilePath(std::string const &filePath) { setText(*m_floodCorrLineEdit, filePath); }

std::string QtExperimentView::getAnalysisMode() const { return getText(*m_ui.analysisModeComboBox); }

void QtExperimentView::setAnalysisMode(std::string const &analysisMode) {
  setSelected(*m_ui.analysisModeComboBox, analysisMode);
}

std::string QtExperimentView::getSummationType() const { return getText(*m_ui.summationTypeComboBox); }

void QtExperimentView::setSummationType(std::string const &summationType) {
  return setSelected(*m_ui.summationTypeComboBox, summationType);
}

std::string QtExperimentView::getReductionType() const { return getText(*m_ui.reductionTypeComboBox); }

bool QtExperimentView::getIncludePartialBins() const { return m_ui.includePartialBinsCheckBox->isChecked(); }

void QtExperimentView::setIncludePartialBins(bool enable) { setChecked(*m_ui.includePartialBinsCheckBox, enable); }

bool QtExperimentView::getDebugOption() const { return m_ui.debugCheckBox->isChecked(); }

void QtExperimentView::setDebugOption(bool enable) { setChecked(*m_ui.debugCheckBox, enable); }

void QtExperimentView::setReductionType(std::string const &reductionType) {
  return setSelected(*m_ui.reductionTypeComboBox, reductionType);
}

std::string QtExperimentView::textFromCell(QTableWidgetItem const *maybeNullItem) const {
  if (maybeNullItem != nullptr) {
    return maybeNullItem->text().toStdString();
  } else {
    return std::string();
  }
}

std::vector<LookupRow::ValueArray> QtExperimentView::getLookupTable() const {
  auto const &table = *m_ui.optionsTable;
  auto rows = std::vector<LookupRow::ValueArray>();
  rows.reserve(table.rowCount());
  using Col = LookupRow::Column;
  for (auto row = 0; row < table.rowCount(); ++row) {
    rows.emplace_back(LookupRow::ValueArray{
        textFromCell(table.item(row, Col::THETA)), textFromCell(table.item(row, Col::TITLE)),
        textFromCell(table.item(row, Col::FIRST_TRANS)), textFromCell(table.item(row, Col::SECOND_TRANS)),
        textFromCell(table.item(row, Col::TRANS_SPECTRA)), textFromCell(table.item(row, Col::QMIN)),
        textFromCell(table.item(row, Col::QMAX)), textFromCell(table.item(row, Col::QSTEP)),
        textFromCell(table.item(row, Col::SCALE)), textFromCell(table.item(row, Col::RUN_SPECTRA)),
        textFromCell(table.item(row, Col::BACKGROUND_SPECTRA)), textFromCell(table.item(row, Col::ROI_DETECTOR_IDS))});
  }
  return rows;
}

void QtExperimentView::setLookupTable(std::vector<LookupRow::ValueArray> rows) {
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

void QtExperimentView::showLookupRowAsInvalid(int row, int column) {
  m_ui.optionsTable->blockSignals(true);
  m_ui.optionsTable->item(row, column)->setBackground(QColor("#ffb8ad"));
  m_ui.optionsTable->blockSignals(false);
}

void QtExperimentView::showLookupRowAsValid(int row) {
  m_ui.optionsTable->blockSignals(true);
  for (auto column = 0; column < m_ui.optionsTable->columnCount(); ++column) {
    m_ui.optionsTable->item(row, column)->setBackground(QBrush(Qt::transparent));
    m_ui.optionsTable->item(row, column)->setToolTip(m_columnToolTips[column]);
  }
  m_ui.optionsTable->blockSignals(false);
}

double QtExperimentView::getTransmissionStartOverlap() const { return m_ui.startOverlapEdit->value(); }

void QtExperimentView::setTransmissionStartOverlap(double start) { m_ui.startOverlapEdit->setValue(start); }

double QtExperimentView::getTransmissionEndOverlap() const { return m_ui.endOverlapEdit->value(); }

void QtExperimentView::setTransmissionEndOverlap(double end) { m_ui.endOverlapEdit->setValue(end); }

std::string QtExperimentView::getTransmissionStitchParams() const { return getText(*m_ui.transStitchParamsEdit); }

void QtExperimentView::setTransmissionStitchParams(std::string const &params) {
  setText(*m_ui.transStitchParamsEdit, params);
}

bool QtExperimentView::getTransmissionScaleRHSWorkspace() const { return m_ui.transScaleRHSCheckBox->isChecked(); }

void QtExperimentView::setTransmissionScaleRHSWorkspace(bool enable) {
  setChecked(*m_ui.transScaleRHSCheckBox, enable);
}

void QtExperimentView::showTransmissionRangeInvalid() {
  showAsInvalid(*m_ui.startOverlapEdit);
  showAsInvalid(*m_ui.endOverlapEdit);
}

void QtExperimentView::showTransmissionRangeValid() {
  showAsValid(*m_ui.startOverlapEdit);
  showAsValid(*m_ui.endOverlapEdit);
}

void QtExperimentView::showTransmissionStitchParamsValid() { showAsValid(*m_ui.transStitchParamsEdit); }

void QtExperimentView::showTransmissionStitchParamsInvalid() { showAsInvalid(*m_ui.transStitchParamsEdit); }

void QtExperimentView::setPolarizationCorrectionOption(std::string const &option) {
  setSelected(*m_ui.polCorrComboBox, option);
}

std::string QtExperimentView::getPolarizationCorrectionOption() const { return getText(*m_ui.polCorrComboBox); }

void QtExperimentView::setPolarizationEfficienciesWorkspaceMode() {
  m_ui.expSettingsGrid->removeItem(m_ui.expSettingsGrid->itemAtPosition(POL_CORR_SELECTOR_ROW, POL_CORR_SELECTOR_COL));
  m_polCorrEfficienciesWsSelector->show();
  m_polCorrEfficienciesLineEdit->hide();
  m_ui.expSettingsGrid->addWidget(m_polCorrEfficienciesWsSelector.get(), POL_CORR_SELECTOR_ROW, POL_CORR_SELECTOR_COL);
}

void QtExperimentView::setPolarizationEfficienciesFilePathMode() {
  m_ui.expSettingsGrid->removeItem(m_ui.expSettingsGrid->itemAtPosition(POL_CORR_SELECTOR_ROW, POL_CORR_SELECTOR_COL));
  m_polCorrEfficienciesWsSelector->hide();
  m_polCorrEfficienciesLineEdit->show();
  m_ui.expSettingsGrid->addWidget(m_polCorrEfficienciesLineEdit.get(), POL_CORR_SELECTOR_ROW, POL_CORR_SELECTOR_COL);
}

std::string QtExperimentView::getPolarizationEfficienciesWorkspace() const {
  return getText(*m_polCorrEfficienciesWsSelector);
}

std::string QtExperimentView::getPolarizationEfficienciesFilePath() const {
  return getText(*m_polCorrEfficienciesLineEdit);
}

void QtExperimentView::setPolarizationEfficienciesWorkspace(std::string const &workspace) {
  setSelected(*m_polCorrEfficienciesWsSelector, workspace);
}

void QtExperimentView::setPolarizationEfficienciesFilePath(std::string const &filePath) {
  setText(*m_polCorrEfficienciesLineEdit, filePath);
}

std::string QtExperimentView::getFredrikzeSpinStateOrder() const {
  return getText(*m_ui.polCorrFredrikzeSpinStateEdit);
}

void QtExperimentView::setFredrikzeSpinStateOrder(std::string const &spinStates) {
  setText(*m_ui.polCorrFredrikzeSpinStateEdit, spinStates);
}

std::string QtExperimentView::getStitchOptions() const { return getText(stitchOptionsLineEdit()); }

void QtExperimentView::setStitchOptions(std::string const &stitchOptions) {
  setText(stitchOptionsLineEdit(), stitchOptions);
}

} // namespace MantidQt::CustomInterfaces::ISISReflectometry
