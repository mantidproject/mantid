#include "QtReflSettingsView.h"
#include "ReflSettingsPresenter.h"
#include "MantidQtWidgets/Common/HintingLineEdit.h"
#include <QMessageBox>

namespace MantidQt {
namespace CustomInterfaces {

using namespace MantidQt::MantidWidgets;

//----------------------------------------------------------------------------------------------
/** Constructor
* @param parent :: [input] The parent of this widget
*/
QtReflSettingsView::QtReflSettingsView(QWidget *parent) {

  UNUSED_ARG(parent);
  initLayout();

  m_presenter.reset(new ReflSettingsPresenter(this));
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
  setText(*m_ui.I0MonIndexEdit, defaults.I0MonitorIndex);
  setSelected(*m_ui.detectorCorrectionTypeComboBox,
              defaults.DetectorCorrectionType);
  setText(*m_ui.procInstEdit, defaults.ProcessingInstructions);
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

void QtReflSettingsView::showOptionLoadError(AccumulatedTypeErrors const &errors) {
  auto message =
      QString("Unable to retrieve default values for the following parameters "
              "because:\n");
  for (auto &error : errors)
    message += "'" + QString::fromStdString(error.parameterName()) + "' should hold an " +
               QString::fromStdString(error.expectedType()) + " value but does not.";
  QMessageBox::warning(this, "Failed to load default from parameter file",
                       message);
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

/** Return selected correction type
* @return :: selected correction type
*/
std::string QtReflSettingsView::getDetectorCorrectionType() const {
  return getText(*m_ui.detectorCorrectionTypeComboBox);
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
