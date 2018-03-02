#include "QtReflSettingsView.h"
#include "ReflSettingsPresenter.h"
#include "MantidQtWidgets/Common/HintingLineEdit.h"
#include <QMessageBox>
#include <boost/algorithm/string/join.hpp>
#include "MantidKernel/System.h"

namespace MantidQt {
namespace CustomInterfaces {

using namespace MantidQt::MantidWidgets;

//----------------------------------------------------------------------------------------------
/** Constructor
* @param parent :: [input] The parent of this widget
* @param group :: The number of the group this settings view's settings
* correspond to.
*/
QtReflSettingsView::QtReflSettingsView(int group, QWidget *parent) {

  UNUSED_ARG(parent);
  initLayout();
  m_presenter = Mantid::Kernel::make_unique<ReflSettingsPresenter>(this, group);
  auto alg = m_presenter->createReductionAlg();
  registerSettingsWidgets(alg);
}

//----------------------------------------------------------------------------------------------
/** Destructor
*/
QtReflSettingsView::~QtReflSettingsView() {}

/**
Initialise the Interface
*/
void QtReflSettingsView::initLayout() {
  m_ui.setupUi(this);

  connect(m_ui.getExpDefaultsButton, SIGNAL(clicked()), this,
          SLOT(requestExpDefaults()));
  connect(m_ui.getInstDefaultsButton, SIGNAL(clicked()), this,
          SLOT(requestInstDefaults()));
  connect(m_ui.expSettingsGroup, SIGNAL(clicked(bool)), this,
          SLOT(setPolarisationOptionsEnabled(bool)));
  connect(m_ui.summationTypeComboBox, SIGNAL(currentIndexChanged(int)), this,
          SLOT(summationTypeChanged(int)));
  connect(m_ui.correctDetectorsCheckBox, SIGNAL(clicked(bool)), this,
          SLOT(setDetectorCorrectionEnabled(bool)));
}

void QtReflSettingsView::connectSettingsChange(QLineEdit &edit) {
  connect(&edit, SIGNAL(textChanged(QString const &)), this,
          SLOT(notifySettingsChanged()));
}

void QtReflSettingsView::connectSettingsChange(QComboBox &edit) {
  connect(&edit, SIGNAL(currentIndexChanged(int)), this,
          SLOT(notifySettingsChanged()));
}

void QtReflSettingsView::connectSettingsChange(QCheckBox &edit) {
  connect(&edit, SIGNAL(stateChanged(int)), this,
          SLOT(notifySettingsChanged()));
}

void QtReflSettingsView::connectSettingsChange(QGroupBox &edit) {
  connect(&edit, SIGNAL(toggled(bool)), this, SLOT(notifySettingsChanged()));
}

void QtReflSettingsView::disableAll() {
  m_ui.instSettingsGroup->setEnabled(false);
  m_ui.expSettingsGroup->setEnabled(false);
}

void QtReflSettingsView::enableAll() {
  m_ui.instSettingsGroup->setEnabled(true);
  m_ui.expSettingsGroup->setEnabled(true);
}

void QtReflSettingsView::registerSettingsWidgets(
    Mantid::API::IAlgorithm_sptr alg) {
  registerExperimentSettingsWidgets(alg);
  registerInstrumentSettingsWidgets(alg);
}

void QtReflSettingsView::registerInstrumentSettingsWidgets(
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
  registerSettingWidget(*m_ui.procInstEdit, "ProcessingInstructions", alg);
  registerSettingWidget(*m_ui.detectorCorrectionTypeComboBox,
                        "DetectorCorrectionType", alg);
  registerSettingWidget(*m_ui.correctDetectorsCheckBox, "CorrectDetectors",
                        alg);
  registerSettingWidget(*m_ui.reductionTypeComboBox, "ReductionType", alg);
  registerSettingWidget(*m_ui.summationTypeComboBox, "SummationType", alg);
}

void QtReflSettingsView::registerExperimentSettingsWidgets(
    Mantid::API::IAlgorithm_sptr alg) {
  connectSettingsChange(*m_ui.expSettingsGroup);
  registerSettingWidget(*m_ui.analysisModeComboBox, "AnalysisMode", alg);
  registerSettingWidget(*m_ui.transmissionRunsEdit, "FirstTransmissionRun",
                        alg);
  registerSettingWidget(*m_ui.startOverlapEdit, "StartOverlap", alg);
  registerSettingWidget(*m_ui.endOverlapEdit, "EndOverlap", alg);
  registerSettingWidget(*m_ui.polCorrComboBox, "PolarizationAnalysis", alg);
  registerSettingWidget(*m_ui.CRhoEdit, "CRho", alg);
  registerSettingWidget(*m_ui.CAlphaEdit, "CAlpha", alg);
  registerSettingWidget(*m_ui.CApEdit, "CAp", alg);
  registerSettingWidget(*m_ui.CPpEdit, "CPp", alg);
  registerSettingWidget(*m_ui.momentumTransferStepEdit, "MomentumTransferStep",
                        alg);
  registerSettingWidget(*m_ui.scaleFactorEdit, "ScaleFactor", alg);
  registerSettingWidget(stitchOptionsLineEdit(), "Params", alg);
}

void QtReflSettingsView::notifySettingsChanged() {
  m_presenter->notify(IReflSettingsPresenter::SettingsChangedFlag);
}

void QtReflSettingsView::summationTypeChanged(int reductionTypeIndex) {
  UNUSED_ARG(reductionTypeIndex);
  m_presenter->notify(IReflSettingsPresenter::Flag::SummationTypeChanged);
}

void QtReflSettingsView::setReductionTypeEnabled(bool enable) {
  m_ui.reductionTypeComboBox->setEnabled(enable);
}

template <typename Widget>
void QtReflSettingsView::registerSettingWidget(
    Widget &widget, std::string const &propertyName,
    Mantid::API::IAlgorithm_sptr alg) {
  connectSettingsChange(widget);
  setToolTipAsPropertyDocumentation(widget, propertyName, alg);
}

void QtReflSettingsView::setToolTipAsPropertyDocumentation(
    QWidget &widget, std::string const &propertyName,
    Mantid::API::IAlgorithm_sptr alg) {
  widget.setToolTip(QString::fromStdString(
      alg->getPointerToProperty(propertyName)->documentation()));
}

/** Returns the presenter managing this view
* @return :: A pointer to the presenter
*/
IReflSettingsPresenter *QtReflSettingsView::getPresenter() const {
  return m_presenter.get();
}

/** This slot notifies the presenter to fill experiment settings with default
* values.
*/
void QtReflSettingsView::requestExpDefaults() const {
  m_presenter->notify(IReflSettingsPresenter::ExpDefaultsFlag);
}

/** This slot notifies the presenter to fill instrument settings with default
* values.
*/
void QtReflSettingsView::requestInstDefaults() const {
  m_presenter->notify(IReflSettingsPresenter::InstDefaultsFlag);
}

/** This slot sets the value of 'm_isPolCorrEnabled' - whether polarisation
* corrections should be enabled or not.
* @param enable :: Value of experiment settings enable status
*/
void QtReflSettingsView::setIsPolCorrEnabled(bool enable) const {
  m_isPolCorrEnabled = enable;
}

/* Sets default values for all experiment settings given a list of default
* values.
*/
void QtReflSettingsView::setExpDefaults(ExperimentOptionDefaults defaults) {
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
  setText(*m_ui.momentumTransferStepEdit, defaults.MomentumTransferStep);
  setText(*m_ui.scaleFactorEdit, defaults.ScaleFactor);
  setText(stitchOptionsLineEdit(), defaults.StitchParams);
}

void QtReflSettingsView::setSelected(QComboBox &box, std::string const &str) {
  auto const index = box.findText(QString::fromStdString(str));
  if (index != -1)
    box.setCurrentIndex(index);
}

void QtReflSettingsView::setText(QLineEdit &lineEdit,
                                 boost::optional<double> value) {
  if (value)
    setText(lineEdit, value.get());
  else
    setText(lineEdit, "");
}

void QtReflSettingsView::setText(QLineEdit &lineEdit,
                                 boost::optional<int> value) {
  if (value)
    setText(lineEdit, value.get());
  else
    setText(lineEdit, "");
}

void QtReflSettingsView::setText(QLineEdit &lineEdit,
                                 boost::optional<std::string> const &text) {
  setText(lineEdit, value_or(text, ""));
}

void QtReflSettingsView::setText(QLineEdit &lineEdit, double value) {
  auto valueAsString = QString::number(value);
  lineEdit.setText(valueAsString);
}

void QtReflSettingsView::setText(QLineEdit &lineEdit, int value) {
  auto valueAsString = QString::number(value);
  lineEdit.setText(valueAsString);
}

void QtReflSettingsView::setText(QLineEdit &lineEdit, std::string const &text) {
  auto textAsQString = QString::fromStdString(text);
  lineEdit.setText(textAsQString);
}

void QtReflSettingsView::setChecked(QCheckBox &checkBox, bool checked) {
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
void QtReflSettingsView::setInstDefaults(InstrumentOptionDefaults defaults) {
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
  setText(*m_ui.procInstEdit, defaults.ProcessingInstructions);
  setChecked(*m_ui.correctDetectorsCheckBox, defaults.CorrectDetectors);
}

void QtReflSettingsView::setDetectorCorrectionEnabled(bool enabled) {
  m_ui.detectorCorrectionTypeComboBox->setEnabled(enabled);
}

/* Sets the enabled status of polarisation corrections and parameters
* @param enable :: [input] bool to enable options or not
*/
void QtReflSettingsView::setPolarisationOptionsEnabled(bool enable) {

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

std::string QtReflSettingsView::getText(QLineEdit const &lineEdit) const {
  return lineEdit.text().toStdString();
}

std::string QtReflSettingsView::getText(QComboBox const &box) const {
  return box.currentText().toStdString();
}

QString QtReflSettingsView::messageFor(
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

QString QtReflSettingsView::messageFor(
    std::vector<MissingInstrumentParameterValue> const &missingValues) const {
  auto missingNamesCsv =
      toCsv(missingValues,
            [](const MissingInstrumentParameterValue &missingValue)
                -> std::string { return missingValue.parameterName(); });

  return QString::fromStdString(missingNamesCsv) +
         QString(missingValues.size() == 1 ? " is" : " are") +
         " not set in the instrument parameter file but should be.\n";
}

void QtReflSettingsView::showOptionLoadErrors(
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
std::string QtReflSettingsView::getStitchOptions() const {
  return getText(stitchOptionsLineEdit());
}

QLineEdit &QtReflSettingsView::stitchOptionsLineEdit() const {
  auto widget = m_ui.expSettingsLayout0->itemAtPosition(7, 1)->widget();
  return *static_cast<QLineEdit *>(widget);
}

/** Creates hints for 'Stitch1DMany'
* @param hints :: Hints as a map
*/
void QtReflSettingsView::createStitchHints(
    const std::map<std::string, std::string> &hints) {

  m_ui.expSettingsLayout0->addWidget(new HintingLineEdit(this, hints), 7, 1, 1,
                                     3);
}

/** Return selected analysis mode
* @return :: selected analysis mode
*/
std::string QtReflSettingsView::getAnalysisMode() const {
  return getText(*m_ui.analysisModeComboBox);
}

/** Return selected transmission run(s)
* @return :: selected transmission run(s)
*/
std::string QtReflSettingsView::getTransmissionRuns() const {
  return getText(*m_ui.transmissionRunsEdit);
}

/** Return start overlap
* @return :: start overlap
*/
std::string QtReflSettingsView::getStartOverlap() const {
  return getText(*m_ui.startOverlapEdit);
}

/** Return end overlap
* @return :: end overlap
*/
std::string QtReflSettingsView::getEndOverlap() const {
  return getText(*m_ui.endOverlapEdit);
}

/** Return selected polarisation corrections
* @return :: selected polarisation corrections
*/
std::string QtReflSettingsView::getPolarisationCorrections() const {
  return getText(*m_ui.polCorrComboBox);
}

/** Return CRho
* @return :: polarization correction CRho
*/
std::string QtReflSettingsView::getCRho() const {
  return getText(*m_ui.CRhoEdit);
}

/** Return CAlpha
* @return :: polarization correction CAlpha
*/
std::string QtReflSettingsView::getCAlpha() const {
  return getText(*m_ui.CAlphaEdit);
}

/** Return CAp
* @return :: polarization correction CAp
*/
std::string QtReflSettingsView::getCAp() const {
  return getText(*m_ui.CApEdit);
}

/** Return CPp
* @return :: polarization correction CPp
*/
std::string QtReflSettingsView::getCPp() const {
  return getText(*m_ui.CPpEdit);
}

/** Return momentum transfer limits
* @return :: momentum transfer limits
*/
std::string QtReflSettingsView::getMomentumTransferStep() const {
  return getText(*m_ui.momentumTransferStepEdit);
}

/** Return scale factor
* @return :: scale factor
*/
std::string QtReflSettingsView::getScaleFactor() const {
  return getText(*m_ui.scaleFactorEdit);
}

/** Return integrated monitors option
* @return :: integrated monitors check
*/
std::string QtReflSettingsView::getIntMonCheck() const {
  return m_ui.intMonCheckBox->isChecked() ? "1" : "0";
}

/** Return monitor integral wavelength min
* @return :: monitor integral min
*/
std::string QtReflSettingsView::getMonitorIntegralMin() const {
  return getText(*m_ui.monIntMinEdit);
}

/** Return monitor integral wavelength max
* @return :: monitor integral max
*/
std::string QtReflSettingsView::getMonitorIntegralMax() const {
  return getText(*m_ui.monIntMaxEdit);
}

/** Return monitor background wavelength min
* @return :: monitor background min
*/
std::string QtReflSettingsView::getMonitorBackgroundMin() const {
  return getText(*m_ui.monBgMinEdit);
}

/** Return monitor background wavelength max
* @return :: monitor background max
*/
std::string QtReflSettingsView::getMonitorBackgroundMax() const {
  return getText(*m_ui.monBgMaxEdit);
}

/** Return wavelength min
* @return :: lambda min
*/
std::string QtReflSettingsView::getLambdaMin() const {
  return getText(*m_ui.lamMinEdit);
}

/** Return wavelength max
* @return :: lambda max
*/
std::string QtReflSettingsView::getLambdaMax() const {
  return getText(*m_ui.lamMaxEdit);
}

/** Return I0MonitorIndex
* @return :: I0MonitorIndex
*/
std::string QtReflSettingsView::getI0MonitorIndex() const {
  return getText(*m_ui.I0MonIndexEdit);
}

/** Return processing instructions
* @return :: processing instructions
*/
std::string QtReflSettingsView::getProcessingInstructions() const {
  return getText(*m_ui.procInstEdit);
}

std::string QtReflSettingsView::getReductionType() const {
  return getText(*m_ui.reductionTypeComboBox);
}

std::string QtReflSettingsView::getSummationType() const {
  return getText(*m_ui.summationTypeComboBox);
}

/** Return selected correction type
* @return :: selected correction type
*/
std::string QtReflSettingsView::getDetectorCorrectionType() const {
  return getText(*m_ui.detectorCorrectionTypeComboBox);
}

bool QtReflSettingsView::detectorCorrectionEnabled() const {
  return m_ui.correctDetectorsCheckBox->isChecked();
}

/** Returns the status of experiment settings group
* @return :: the status of the checkable group
*/
bool QtReflSettingsView::experimentSettingsEnabled() const {
  return m_ui.expSettingsGroup->isChecked();
}

/** Returns the status of instrument settings group
* @return :: the status of the checkable group
*/
bool QtReflSettingsView::instrumentSettingsEnabled() const {
  return m_ui.instSettingsGroup->isChecked();
}

} // namespace CustomInterfaces
} // namespace Mantid
