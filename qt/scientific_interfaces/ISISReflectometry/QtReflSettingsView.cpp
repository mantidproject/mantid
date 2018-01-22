#include "QtReflSettingsView.h"
#include "ReflSettingsPresenter.h"
#include "MantidQtWidgets/Common/HintingLineEdit.h"
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

  m_presenter.reset(new ReflSettingsPresenter(this, group));
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

  connectChangeListeners();
}

void QtReflSettingsView::connectSettingsChange(QLineEdit *edit) {
  connect(edit, SIGNAL(textChanged(QString const &)), this,
          SLOT(notifySettingsChanged()));
}

void QtReflSettingsView::connectSettingsChange(QComboBox *edit) {
  connect(edit, SIGNAL(currentIndexChanged(int)), this,
          SLOT(notifySettingsChanged()));
}

void QtReflSettingsView::connectSettingsChange(QCheckBox *edit) {
  connect(edit, SIGNAL(stateChanged(int)), this, SLOT(notifySettingsChanged()));
}

void QtReflSettingsView::connectSettingsChange(QGroupBox *edit) {
  connect(edit, SIGNAL(toggled(bool)), this, SLOT(notifySettingsChanged()));
}

void QtReflSettingsView::connectChangeListeners() {
  connectExperimentSettingsChangeListeners();
  connectInstrumentSettingsChangeListeners();
}

void QtReflSettingsView::connectInstrumentSettingsChangeListeners() {
  connectSettingsChange(m_ui.instSettingsGroup);
  connectSettingsChange(m_ui.intMonCheckBox);
  connectSettingsChange(m_ui.monIntMinEdit);
  connectSettingsChange(m_ui.monIntMaxEdit);
  connectSettingsChange(m_ui.monBgMinEdit);
  connectSettingsChange(m_ui.monBgMaxEdit);
  connectSettingsChange(m_ui.lamMinEdit);
  connectSettingsChange(m_ui.lamMaxEdit);
  connectSettingsChange(m_ui.I0MonIndexEdit);
  connectSettingsChange(m_ui.procInstEdit);
  connectSettingsChange(m_ui.detectorCorrectionTypeComboBox);
  connectSettingsChange(m_ui.correctDetectorsCheckBox);
  connectSettingsChange(m_ui.reductionTypeComboBox);
  connectSettingsChange(m_ui.summationTypeComboBox);
}

void QtReflSettingsView::connectExperimentSettingsChangeListeners() {
  connectSettingsChange(m_ui.expSettingsGroup);
  connectSettingsChange(m_ui.analysisModeComboBox);
  connectSettingsChange(m_ui.transmissionRunsEdit);
  connectSettingsChange(m_ui.startOverlapEdit);
  connectSettingsChange(m_ui.endOverlapEdit);
  connectSettingsChange(m_ui.polCorrComboBox);
  connectSettingsChange(m_ui.CRhoEdit);
  connectSettingsChange(m_ui.CAlphaEdit);
  connectSettingsChange(m_ui.CApEdit);
  connectSettingsChange(m_ui.CPpEdit);
  connectSettingsChange(m_ui.momentumTransferStepEdit);
  connectSettingsChange(m_ui.scaleFactorEdit);
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
void QtReflSettingsView::setExpDefaults(
    const std::vector<std::string> &defaults) const {

  int amIndex =
      m_ui.analysisModeComboBox->findText(QString::fromStdString(defaults[0]));
  if (amIndex != -1)
    m_ui.analysisModeComboBox->setCurrentIndex(amIndex);

  int pcIndex =
      m_ui.polCorrComboBox->findText(QString::fromStdString(defaults[1]));
  if (pcIndex != -1)
    m_ui.polCorrComboBox->setCurrentIndex(pcIndex);

  m_ui.CRhoEdit->setText(QString::fromStdString(defaults[2]));
  m_ui.CAlphaEdit->setText(QString::fromStdString(defaults[3]));
  m_ui.CApEdit->setText(QString::fromStdString(defaults[4]));
  m_ui.CPpEdit->setText(QString::fromStdString(defaults[5]));
  m_ui.startOverlapEdit->setText(QString::fromStdString(defaults[6]));
  m_ui.endOverlapEdit->setText(QString::fromStdString(defaults[7]));
}

/* Sets default values for all instrument settings given a list of default
* values.
*/
void QtReflSettingsView::setInstDefaults(
    const std::vector<double> &defaults_double,
    const std::vector<std::string> &defaults_str) const {

  auto intMonCheckState =
      (defaults_double[0] != 0) ? Qt::Checked : Qt::Unchecked;
  m_ui.intMonCheckBox->setCheckState(intMonCheckState);
  m_ui.monIntMinEdit->setText(QString::number(defaults_double[1]));
  m_ui.monIntMaxEdit->setText(QString::number(defaults_double[2]));
  m_ui.monBgMinEdit->setText(QString::number(defaults_double[3]));
  m_ui.monBgMaxEdit->setText(QString::number(defaults_double[4]));
  m_ui.lamMinEdit->setText(QString::number(defaults_double[5]));
  m_ui.lamMaxEdit->setText(QString::number(defaults_double[6]));
  m_ui.I0MonIndexEdit->setText(QString::number(defaults_double[7]));

  int ctIndex = m_ui.detectorCorrectionTypeComboBox->findText(
      QString::fromStdString(defaults_str[0]));
  if (ctIndex != -1)
    m_ui.detectorCorrectionTypeComboBox->setCurrentIndex(ctIndex);
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
    int noneIndex = m_ui.polCorrComboBox->findText("None");
    if (noneIndex != -1)
      m_ui.polCorrComboBox->setCurrentIndex(noneIndex);
    // Clear all parameters as well
    m_ui.CRhoEdit->clear();
    m_ui.CAlphaEdit->clear();
    m_ui.CApEdit->clear();
    m_ui.CPpEdit->clear();
  }
}

/** Returns global options for 'Stitch1DMany'
* @return :: Global options for 'Stitch1DMany'
*/
std::string QtReflSettingsView::getStitchOptions() const {

  auto widget = m_ui.expSettingsLayout0->itemAtPosition(7, 1)->widget();
  return static_cast<HintingLineEdit *>(widget)->text().toStdString();
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

  return m_ui.analysisModeComboBox->currentText().toStdString();
}

/** Return selected transmission run(s)
* @return :: selected transmission run(s)
*/
std::string QtReflSettingsView::getTransmissionRuns() const {

  return m_ui.transmissionRunsEdit->text().toStdString();
}

/** Return start overlap
* @return :: start overlap
*/
std::string QtReflSettingsView::getStartOverlap() const {

  return m_ui.startOverlapEdit->text().toStdString();
}

/** Return end overlap
* @return :: end overlap
*/
std::string QtReflSettingsView::getEndOverlap() const {

  return m_ui.endOverlapEdit->text().toStdString();
}

/** Return selected polarisation corrections
* @return :: selected polarisation corrections
*/
std::string QtReflSettingsView::getPolarisationCorrections() const {

  return m_ui.polCorrComboBox->currentText().toStdString();
}

/** Return CRho
* @return :: polarization correction CRho
*/
std::string QtReflSettingsView::getCRho() const {

  return m_ui.CRhoEdit->text().toStdString();
}

/** Return CAlpha
* @return :: polarization correction CAlpha
*/
std::string QtReflSettingsView::getCAlpha() const {

  return m_ui.CAlphaEdit->text().toStdString();
}

/** Return CAp
* @return :: polarization correction CAp
*/
std::string QtReflSettingsView::getCAp() const {

  return m_ui.CApEdit->text().toStdString();
}

/** Return CPp
* @return :: polarization correction CPp
*/
std::string QtReflSettingsView::getCPp() const {

  return m_ui.CPpEdit->text().toStdString();
}

/** Return momentum transfer limits
* @return :: momentum transfer limits
*/
std::string QtReflSettingsView::getMomentumTransferStep() const {

  return m_ui.momentumTransferStepEdit->text().toStdString();
}

/** Return scale factor
* @return :: scale factor
*/
std::string QtReflSettingsView::getScaleFactor() const {

  return m_ui.scaleFactorEdit->text().toStdString();
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

  return m_ui.monIntMinEdit->text().toStdString();
}

/** Return monitor integral wavelength max
* @return :: monitor integral max
*/
std::string QtReflSettingsView::getMonitorIntegralMax() const {

  return m_ui.monIntMaxEdit->text().toStdString();
}

/** Return monitor background wavelength min
* @return :: monitor background min
*/
std::string QtReflSettingsView::getMonitorBackgroundMin() const {

  return m_ui.monBgMinEdit->text().toStdString();
}

/** Return monitor background wavelength max
* @return :: monitor background max
*/
std::string QtReflSettingsView::getMonitorBackgroundMax() const {

  return m_ui.monBgMaxEdit->text().toStdString();
}

/** Return wavelength min
* @return :: lambda min
*/
std::string QtReflSettingsView::getLambdaMin() const {

  return m_ui.lamMinEdit->text().toStdString();
}

/** Return wavelength max
* @return :: lambda max
*/
std::string QtReflSettingsView::getLambdaMax() const {

  return m_ui.lamMaxEdit->text().toStdString();
}

/** Return I0MonitorIndex
* @return :: I0MonitorIndex
*/
std::string QtReflSettingsView::getI0MonitorIndex() const {

  return m_ui.I0MonIndexEdit->text().toStdString();
}

/** Return processing instructions
* @return :: processing instructions
*/
std::string QtReflSettingsView::getProcessingInstructions() const {

  return m_ui.procInstEdit->text().toStdString();
}

std::string QtReflSettingsView::getReductionType() const {
  return m_ui.reductionTypeComboBox->currentText().toStdString();
}

std::string QtReflSettingsView::getSummationType() const {
  return m_ui.summationTypeComboBox->currentText().toStdString();
}

/** Return selected correction type
* @return :: selected correction type
*/
std::string QtReflSettingsView::getDetectorCorrectionType() const {

  return m_ui.detectorCorrectionTypeComboBox->currentText().toStdString();
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
