#include "QtReflSettingsTabView.h"
#include "ReflSettingsTabPresenter.h"
#include <boost/algorithm/string/join.hpp>
#include "MantidQtWidgets/Common/DataProcessorUI/OptionsQMap.h"
#include "MantidQtWidgets/Common/AlgorithmHintStrategy.h"
#include <QMessageBox>
#include <QScrollBar>

namespace MantidQt {
namespace CustomInterfaces {

/** Constructor
* @param parent :: [input] The parent of this widget
*/
QtReflSettingsTabView::QtReflSettingsTabView(
    Mantid::API::IAlgorithm_sptr algorithmForTooltips, QWidget *parent) {
  UNUSED_ARG(parent);
  initLayout();
  registerSettingsWidgets(algorithmForTooltips);
}

void QtReflSettingsTabView::subscribe(ReflSettingsTabViewSubscriber *notifyee) {
  m_notifyee = notifyee;
}

/**
Initialise the Interface
*/
void QtReflSettingsTabView::initLayout() {
  m_ui.setupUi(this);

  auto blacklist =
      std::vector<std::string>({"InputWorkspaces", "OutputWorkspace"});
  MantidWidgets::AlgorithmHintStrategy strategy("Stitch1DMany", blacklist);
  createStitchHints(strategy.createHints());

  initOptionsTable();

  connect(m_ui.getExpDefaultsButton, SIGNAL(clicked()), this,
          SLOT(requestExpDefaults()));
  connect(m_ui.getInstDefaultsButton, SIGNAL(clicked()), this,
          SLOT(requestInstDefaults()));
  connect(m_ui.expSettingsGroup, SIGNAL(clicked(bool)), this,
          SLOT(setPolarisationOptionsEnabled(bool)));
  connect(m_ui.summationTypeComboBox, SIGNAL(currentIndexChanged(int)), this,
          SLOT(summationTypeChanged(int)));
  connect(m_ui.addPerAngleOptionsButton, SIGNAL(clicked()), this,
          SLOT(addPerAngleOptionsTableRow()));
  connect(m_ui.correctDetectorsCheckBox, SIGNAL(clicked(bool)), this,
          SLOT(setDetectorCorrectionEnabled(bool)));
}

void QtReflSettingsTabView::initOptionsTable() {
  auto table = m_ui.optionsTable;
  m_columnProperties =
      QStringList({"ThetaIn", "FirstTransmissionRun", "MomentumTransferMin",
                   "MomentumTransferMax", "MomentumTransferStep", "ScaleFactor",
                   "ProcessingInstructions"});
  if (m_columnProperties.size() != table->columnCount())
    throw std::runtime_error(
        "Error setting up properties for per-angle options table");

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

void QtReflSettingsTabView::connectSettingsChange(QLineEdit &edit) {
  connect(&edit, SIGNAL(textChanged(QString const &)), this,
          SLOT(notifySettingsChanged()));
}

void QtReflSettingsTabView::connectSettingsChange(QComboBox &edit) {
  connect(&edit, SIGNAL(currentIndexChanged(int)), this,
          SLOT(notifySettingsChanged()));
}

void QtReflSettingsTabView::connectSettingsChange(QCheckBox &edit) {
  connect(&edit, SIGNAL(stateChanged(int)), this,
          SLOT(notifySettingsChanged()));
}

void QtReflSettingsTabView::connectSettingsChange(QGroupBox &edit) {
  connect(&edit, SIGNAL(toggled(bool)), this, SLOT(notifySettingsChanged()));
}

void QtReflSettingsTabView::connectSettingsChange(QTableWidget &edit) {
  connect(&edit, SIGNAL(cellChanged(int, int)), this,
          SLOT(notifySettingsChanged()));
}

void QtReflSettingsTabView::disableAll() {
  m_ui.instSettingsGroup->setEnabled(false);
  m_ui.expSettingsGroup->setEnabled(false);
}

void QtReflSettingsTabView::enableAll() {
  m_ui.instSettingsGroup->setEnabled(true);
  m_ui.expSettingsGroup->setEnabled(true);
}

void QtReflSettingsTabView::registerSettingsWidgets(
    Mantid::API::IAlgorithm_sptr alg) {
  registerExperimentSettingsWidgets(alg);
  registerInstrumentSettingsWidgets(alg);
}

void QtReflSettingsTabView::registerInstrumentSettingsWidgets(
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
  registerSettingWidget(*m_ui.reductionTypeComboBox, "ReductionType", alg);
  registerSettingWidget(*m_ui.summationTypeComboBox, "SummationType", alg);
}

void QtReflSettingsTabView::registerExperimentSettingsWidgets(
    Mantid::API::IAlgorithm_sptr alg) {
  connectSettingsChange(*m_ui.expSettingsGroup);
  connectSettingsChange(*m_ui.optionsTable);
  registerSettingWidget(*m_ui.analysisModeComboBox, "AnalysisMode", alg);
  registerSettingWidget(*m_ui.startOverlapEdit, "StartOverlap", alg);
  registerSettingWidget(*m_ui.endOverlapEdit, "EndOverlap", alg);
  registerSettingWidget(*m_ui.polCorrComboBox, "PolarizationAnalysis", alg);
  registerSettingWidget(*m_ui.CRhoEdit, "Rho", alg);
  registerSettingWidget(*m_ui.CAlphaEdit, "Alpha", alg);
  registerSettingWidget(*m_ui.CApEdit, "Ap", alg);
  registerSettingWidget(*m_ui.CPpEdit, "Pp", alg);
  registerSettingWidget(stitchOptionsLineEdit(), "Params", alg);
}

void QtReflSettingsTabView::notifySettingsChanged() {
  m_notifyee->notifySettingsChanged();
}

void QtReflSettingsTabView::summationTypeChanged(int reductionTypeIndex) {
  UNUSED_ARG(reductionTypeIndex);
  m_notifyee->notifySummationTypeChanged();
}

void QtReflSettingsTabView::setReductionTypeEnabled(bool enable) {
  m_ui.reductionTypeComboBox->setEnabled(enable);
}

template <typename Widget>
void QtReflSettingsTabView::registerSettingWidget(
    Widget &widget, std::string const &propertyName,
    Mantid::API::IAlgorithm_sptr alg) {
  connectSettingsChange(widget);
  setToolTipAsPropertyDocumentation(widget, propertyName, alg);
}

void QtReflSettingsTabView::setToolTipAsPropertyDocumentation(
    QWidget &widget, std::string const &propertyName,
    Mantid::API::IAlgorithm_sptr alg) {
  widget.setToolTip(QString::fromStdString(
      alg->getPointerToProperty(propertyName)->documentation()));
}

/** This slot notifies the presenter to fill experiment settings with default
* values.
*/
void QtReflSettingsTabView::requestExpDefaults() const {
  m_notifyee->notifyExperimentDefaultsRequested();
}

/** This slot notifies the presenter to fill instrument settings with default
* values.
*/
void QtReflSettingsTabView::requestInstDefaults() const {
  m_notifyee->notifyInstrumentDefaultsRequested();
}

/** This slot sets the value of 'm_isPolCorrEnabled' - whether polarisation
* corrections should be enabled or not.
* @param enable :: Value of experiment settings enable status
*/
void QtReflSettingsTabView::setIsPolCorrEnabled(bool enable) const {
  m_isPolCorrEnabled = enable;
}

/* Sets default values for all experiment settings given a list of default
* values.
*/
void QtReflSettingsTabView::setExpDefaults(ExperimentOptionDefaults defaults) {
  setSelected(*m_ui.analysisModeComboBox, defaults.AnalysisMode);
  setSelected(*m_ui.reductionTypeComboBox, defaults.ReductionType);
  setSelected(*m_ui.summationTypeComboBox, defaults.SummationType);
  setText(*m_ui.startOverlapEdit, defaults.TransRunStartOverlap);
  setText(*m_ui.endOverlapEdit, defaults.TransRunEndOverlap);
  setSelected(*m_ui.polCorrComboBox, defaults.PolarizationAnalysis);
  setText(*m_ui.CRhoEdit, defaults.CRho);
  setText(*m_ui.CAlphaEdit, defaults.CAlpha);
  setText(*m_ui.CApEdit, defaults.CAp);
  setText(*m_ui.CPpEdit, defaults.CPp);
  setText(*m_ui.startOverlapEdit, defaults.TransRunStartOverlap);
  setText(*m_ui.endOverlapEdit, defaults.TransRunEndOverlap);
  setText(stitchOptionsLineEdit(), defaults.StitchParams);
  setText(*m_ui.optionsTable, "MomentumTransferMin",
          defaults.MomentumTransferMin);
  setText(*m_ui.optionsTable, "MomentumTransferMax",
          defaults.MomentumTransferMax);
  setText(*m_ui.optionsTable, "MomentumTransferStep",
          defaults.MomentumTransferStep);
  setText(*m_ui.optionsTable, "ScaleFactor", defaults.ScaleFactor);
  setText(*m_ui.optionsTable, "ProcessingInstructions",
          defaults.ProcessingInstructions);
}

void QtReflSettingsTabView::setSelected(QComboBox &box,
                                        std::string const &str) {
  auto const index = box.findText(QString::fromStdString(str));
  if (index != -1)
    box.setCurrentIndex(index);
}

void QtReflSettingsTabView::setText(QLineEdit &lineEdit,
                                    boost::optional<double> value) {
  if (value)
    setText(lineEdit, value.get());
}

void QtReflSettingsTabView::setText(QLineEdit &lineEdit,
                                    boost::optional<int> value) {
  if (value)
    setText(lineEdit, value.get());
}

void QtReflSettingsTabView::setText(QLineEdit &lineEdit,
                                    boost::optional<std::string> const &text) {
  if (text && !text->empty())
    setText(lineEdit, text);
}

void QtReflSettingsTabView::setText(QLineEdit &lineEdit, double value) {
  auto valueAsString = QString::number(value);
  lineEdit.setText(valueAsString);
}

void QtReflSettingsTabView::setText(QLineEdit &lineEdit, int value) {
  auto valueAsString = QString::number(value);
  lineEdit.setText(valueAsString);
}

void QtReflSettingsTabView::setText(QLineEdit &lineEdit,
                                    std::string const &text) {
  auto textAsQString = QString::fromStdString(text);
  lineEdit.setText(textAsQString);
}

void QtReflSettingsTabView::setText(QTableWidget &table,
                                    std::string const &propertyName,
                                    boost::optional<double> value) {
  if (value)
    setText(table, propertyName, value.get());
}

void QtReflSettingsTabView::setText(QTableWidget &table,
                                    std::string const &propertyName,
                                    double value) {
  auto valueAsString = QString::number(value);
  setText(table, propertyName, valueAsString);
}

void QtReflSettingsTabView::setText(QTableWidget &table,
                                    std::string const &propertyName,
                                    boost::optional<std::string> text) {
  if (text && !text->empty())
    setText(table, propertyName, text.get());
}

void QtReflSettingsTabView::setText(QTableWidget &table,
                                    std::string const &propertyName,
                                    std::string const &text) {
  auto textAsQString = QString::fromStdString(text);
  setText(table, propertyName, textAsQString);
}

void QtReflSettingsTabView::setText(QTableWidget &table,
                                    std::string const &propertyName,
                                    const QString &value) {
  // Find the column with this property name
  const auto columnIt =
      std::find(m_columnProperties.begin(), m_columnProperties.end(),
                QString::fromStdString(propertyName));
  // Do nothing if column was not found
  if (columnIt == m_columnProperties.end())
    return;

  const auto column = columnIt - m_columnProperties.begin();

  // Set the value in this column for the first row. (We don't really know
  // which row(s) the user might want updated so for now keep it simple.)
  constexpr int row = 0;
  auto cell = table.item(row, column);
  if (!cell) {
    cell = new QTableWidgetItem();
    table.setItem(row, column, cell);
  }
  cell->setText(value);
}

void QtReflSettingsTabView::setChecked(QCheckBox &checkBox, bool checked) {
  auto checkedAsCheckState = checked ? Qt::Checked : Qt::Unchecked;
  checkBox.setCheckState(checkedAsCheckState);
}

class SetI0MonIndex : public boost::static_visitor<> {
public:
  explicit SetI0MonIndex(QLineEdit &I0MonIndexEdit)
      : m_I0monIndexEdit(I0MonIndexEdit) {}

  void operator()(int index) const {
    m_I0monIndexEdit.setText(QString::number(index));
  }

  void operator()(double index) const {
    this->operator()(static_cast<int>(index));
  }

private:
  QLineEdit &m_I0monIndexEdit;
};

/* Sets default values for all instrument settings given a list of default
* values.
*/
void QtReflSettingsTabView::setInstDefaults(InstrumentOptionDefaults defaults) {
  setChecked(*m_ui.intMonCheckBox, defaults.NormalizeByIntegratedMonitors);
  setText(*m_ui.monIntMinEdit, defaults.MonitorIntegralMin);
  setText(*m_ui.monIntMaxEdit, defaults.MonitorIntegralMax);
  setText(*m_ui.monBgMinEdit, defaults.MonitorBackgroundMin);
  setText(*m_ui.monBgMaxEdit, defaults.MonitorBackgroundMax);
  setText(*m_ui.lamMinEdit, defaults.LambdaMin);
  setText(*m_ui.lamMaxEdit, defaults.LambdaMax);
  boost::apply_visitor(SetI0MonIndex(*m_ui.I0MonIndexEdit),
                       defaults.I0MonitorIndex);
  setSelected(*m_ui.detectorCorrectionTypeComboBox,
              defaults.DetectorCorrectionType);
  setChecked(*m_ui.correctDetectorsCheckBox, defaults.CorrectDetectors);
}

void QtReflSettingsTabView::setDetectorCorrectionEnabled(bool enabled) {
  m_ui.detectorCorrectionTypeComboBox->setEnabled(enabled);
}

/* Sets the enabled status of polarisation corrections and parameters
* @param enable :: [input] bool to enable options or not
*/
void QtReflSettingsTabView::setPolarisationOptionsEnabled(bool enable) {

  if (enable && (!m_isPolCorrEnabled || !experimentSettingsEnabled()))
    return;

  m_ui.polCorrComboBox->setEnabled(enable);
  m_ui.CRhoEdit->setEnabled(enable);
  m_ui.CAlphaEdit->setEnabled(enable);
  m_ui.CApEdit->setEnabled(enable);
  m_ui.CPpEdit->setEnabled(enable);

  if (!enable) {
    // Set polarisation corrections text to 'None' when disabled
    setSelected(*m_ui.polCorrComboBox, "None");
    // Clear all parameters as well
    m_ui.CRhoEdit->clear();
    m_ui.CAlphaEdit->clear();
    m_ui.CApEdit->clear();
    m_ui.CPpEdit->clear();
  }
}

/** Add a new row to the transmission runs table
 * */
void QtReflSettingsTabView::addPerAngleOptionsTableRow() {
  auto numRows = m_ui.optionsTable->rowCount() + 1;
  m_ui.optionsTable->setRowCount(numRows);
  // Select the first cell in the new row
  m_ui.optionsTable->setCurrentCell(numRows - 1, 0);
}

std::string QtReflSettingsTabView::getText(QLineEdit const &lineEdit) const {
  return lineEdit.text().toStdString();
}

std::string QtReflSettingsTabView::getText(QComboBox const &box) const {
  return box.currentText().toStdString();
}

QString QtReflSettingsTabView::messageFor(
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

QString QtReflSettingsTabView::messageFor(
    std::vector<MissingInstrumentParameterValue> const &missingValues) const {
  auto missingNamesCsv =
      toCsv(missingValues,
            [](const MissingInstrumentParameterValue &missingValue)
                -> std::string { return missingValue.parameterName(); });

  return QString::fromStdString(missingNamesCsv) +
         QString(missingValues.size() == 1 ? " is" : " are") +
         " not set in the instrument parameter file but should be.\n";
}

void QtReflSettingsTabView::showOptionLoadErrors(
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

/** Returns global options for 'Stitch1DMany'
* @return :: Global options for 'Stitch1DMany'
*/
std::string QtReflSettingsTabView::getStitchOptions() const {
  return getText(stitchOptionsLineEdit());
}

QLineEdit &QtReflSettingsTabView::stitchOptionsLineEdit() const {
  return *static_cast<QLineEdit *>(m_stitchEdit);
}

/** Creates hints for 'Stitch1DMany'
* @param hints :: Hints as a map
*/
void QtReflSettingsTabView::createStitchHints(const std::vector<MantidWidgets::Hint> &hints) {

  // We want to add the stitch params box next to the stitch
  // label, so first find the label's position
  auto stitchLabelIndex = m_ui.expSettingsLayout0->indexOf(m_ui.stitchLabel);
  int row, col, rowSpan, colSpan;
  m_ui.expSettingsLayout0->getItemPosition(stitchLabelIndex, &row, &col,
                                           &rowSpan, &colSpan);
  // Create the new edit box and add it to the right of the label
  m_stitchEdit = new MantidWidgets::HintingLineEdit(this, hints);
  m_ui.expSettingsLayout0->addWidget(m_stitchEdit, row, col + colSpan, 1, 3);
}

/** Return selected analysis mode
* @return :: selected analysis mode
*/
std::string QtReflSettingsTabView::getAnalysisMode() const {
  return getText(*m_ui.analysisModeComboBox);
}

/** Create the options map for a given row in the per-angle options table
 * @param row [in] : the row index
 */
MantidWidgets::DataProcessor::OptionsQMap QtReflSettingsTabView::createOptionsMapForRow(const int row) const {
  MantidWidgets::DataProcessor::OptionsQMap rowOptions;
  const auto &table = m_ui.optionsTable;

  for (int col = 1; col < table->columnCount(); ++col) {
    auto colItem = table->item(row, col);
    auto colValue = colItem ? colItem->text() : "";
    if (!colValue.isEmpty()) {
      rowOptions[m_columnProperties[col]] = colValue;
    }
  }
  return rowOptions;
}

/** Return the per-angle options
* @return :: return a map of angles to the options
*/
std::map<std::string, MantidWidgets::DataProcessor::OptionsQMap>
QtReflSettingsTabView::getPerAngleOptions() const {

  const auto &table = m_ui.optionsTable;

  // Check that we have at least 2 columns (the angle and some values)
  if (table->columnCount() < 2)
    throw std::runtime_error(
        "Per-angle options table must have at least 2 columns");

  // Return values in a map
  std::map<std::string, MantidWidgets::DataProcessor::OptionsQMap> results;

  for (auto row = 0; row < table->rowCount(); ++row) {
    auto angleItem = table->item(row, 0);
    auto angle = angleItem ? angleItem->text() : "";
    auto rowOptions = createOptionsMapForRow(row);
    const bool emptyRow = angle.isEmpty() && rowOptions.isEmpty();
    // Add the row options to the result. We could do with a better way to
    // handle duplicate keys but for now it's ok to just ignore subsequent rows
    // with the same angle
    if (!emptyRow && !results.count(angle.toStdString()))
      results[angle.toStdString()] = rowOptions;
  }
  return results;
}

/** Return start overlap
* @return :: start overlap
*/
std::string QtReflSettingsTabView::getStartOverlap() const {
  return getText(*m_ui.startOverlapEdit);
}

/** Return end overlap
* @return :: end overlap
*/
std::string QtReflSettingsTabView::getEndOverlap() const {
  return getText(*m_ui.endOverlapEdit);
}

/** Return selected polarisation corrections
* @return :: selected polarisation corrections
*/
std::string QtReflSettingsTabView::getPolarisationCorrections() const {
  return getText(*m_ui.polCorrComboBox);
}

/** Return CRho
* @return :: polarization correction CRho
*/
std::string QtReflSettingsTabView::getCRho() const {
  return getText(*m_ui.CRhoEdit);
}

/** Return CAlpha
* @return :: polarization correction CAlpha
*/
std::string QtReflSettingsTabView::getCAlpha() const {
  return getText(*m_ui.CAlphaEdit);
}

/** Return CAp
* @return :: polarization correction CAp
*/
std::string QtReflSettingsTabView::getCAp() const {
  return getText(*m_ui.CApEdit);
}

/** Return CPp
* @return :: polarization correction CPp
*/
std::string QtReflSettingsTabView::getCPp() const {
  return getText(*m_ui.CPpEdit);
}

/** Return integrated monitors option
* @return :: integrated monitors check
*/
std::string QtReflSettingsTabView::getIntMonCheck() const {
  return m_ui.intMonCheckBox->isChecked() ? "1" : "0";
}

/** Return monitor integral wavelength min
* @return :: monitor integral min
*/
std::string QtReflSettingsTabView::getMonitorIntegralMin() const {
  return getText(*m_ui.monIntMinEdit);
}

/** Return monitor integral wavelength max
* @return :: monitor integral max
*/
std::string QtReflSettingsTabView::getMonitorIntegralMax() const {
  return getText(*m_ui.monIntMaxEdit);
}

/** Return monitor background wavelength min
* @return :: monitor background min
*/
std::string QtReflSettingsTabView::getMonitorBackgroundMin() const {
  return getText(*m_ui.monBgMinEdit);
}

/** Return monitor background wavelength max
* @return :: monitor background max
*/
std::string QtReflSettingsTabView::getMonitorBackgroundMax() const {
  return getText(*m_ui.monBgMaxEdit);
}

/** Return wavelength min
* @return :: lambda min
*/
std::string QtReflSettingsTabView::getLambdaMin() const {
  return getText(*m_ui.lamMinEdit);
}

/** Return wavelength max
* @return :: lambda max
*/
std::string QtReflSettingsTabView::getLambdaMax() const {
  return getText(*m_ui.lamMaxEdit);
}

/** Return I0MonitorIndex
* @return :: I0MonitorIndex
*/
std::string QtReflSettingsTabView::getI0MonitorIndex() const {
  return getText(*m_ui.I0MonIndexEdit);
}

std::string QtReflSettingsTabView::getReductionType() const {
  return getText(*m_ui.reductionTypeComboBox);
}

std::string QtReflSettingsTabView::getSummationType() const {
  return getText(*m_ui.summationTypeComboBox);
}

/** Return selected correction type
* @return :: selected correction type
*/
std::string QtReflSettingsTabView::getDetectorCorrectionType() const {
  return getText(*m_ui.detectorCorrectionTypeComboBox);
}

bool QtReflSettingsTabView::detectorCorrectionEnabled() const {
  return m_ui.correctDetectorsCheckBox->isChecked();
}

/** Returns the status of experiment settings group
* @return :: the status of the checkable group
*/
bool QtReflSettingsTabView::experimentSettingsEnabled() const {
  return m_ui.expSettingsGroup->isChecked();
}

/** Returns the status of instrument settings group
* @return :: the status of the checkable group
*/
bool QtReflSettingsTabView::instrumentSettingsEnabled() const {
  return m_ui.instSettingsGroup->isChecked();
}

} // namespace CustomInterfaces
} // namespace MantidQt
