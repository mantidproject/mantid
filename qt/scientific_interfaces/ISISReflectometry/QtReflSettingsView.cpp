#include "QtReflSettingsView.h"
#include "ReflSettingsPresenter.h"
#include "MantidQtWidgets/Common/HintingLineEdit.h"

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

  int amIndex =
      m_ui.analysisModeComboBox->findText(QString::fromStdString(defaults.AnalysisMode));
  if (amIndex != -1)
    m_ui.analysisModeComboBox->setCurrentIndex(amIndex);

  int pcIndex =
      m_ui.polCorrComboBox->findText(QString::fromStdString(defaults.PolarizationAnalysis));
  if (pcIndex != -1)
    m_ui.polCorrComboBox->setCurrentIndex(pcIndex);

  m_ui.CRhoEdit->setText(QString::fromStdString(defaults.CRho));
  m_ui.CAlphaEdit->setText(QString::fromStdString(defaults.CAlpha));
  m_ui.CApEdit->setText(QString::fromStdString(defaults.CAp));
  m_ui.CPpEdit->setText(QString::fromStdString(defaults.CPp));
  m_ui.startOverlapEdit->setText(QString::number(defaults.TransRunStartOverlap));
  m_ui.endOverlapEdit->setText(QString::number(defaults.TransRunEndOverlap));
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

void QtReflSettingsView::setText(QLineEdit &lineEdit, std::string const &text) {
  auto textAsQString = QString::fromStdString(text);
  lineEdit.setText(textAsQString);
}

/* Sets default values for all instrument settings given a list of default
* values.
*/
void QtReflSettingsView::setInstDefaults(InstrumentOptionDefaults defaults) {

  auto intMonCheckState =
      (defaults.NormalizeByIntegratedMonitors) ? Qt::Checked : Qt::Unchecked;
  m_ui.intMonCheckBox->setCheckState(intMonCheckState);

  setText(*m_ui.monIntMinEdit, defaults.MonitorIntegralMin);
  setText(*m_ui.monIntMaxEdit, defaults.MonitorIntegralMax);
  setText(*m_ui.monBgMinEdit, defaults.MonitorBackgroundMin);
  setText(*m_ui.monBgMaxEdit, defaults.MonitorBackgroundMax);
  setText(*m_ui.lamMinEdit, defaults.LambdaMin);
  setText(*m_ui.lamMaxEdit, defaults.LambdaMax);
  setText(*m_ui.I0MonIndexEdit, std::to_string(defaults.I0MonitorIndex));

  setSelected(*m_ui.detectorCorrectionTypeComboBox, defaults.DetectorCorrectionType);
  setText(stitchOptionsLineEdit(), defaults.ProcessingInstructions);
}

/* Sets the enabled status of polarisation corrections and parameters
* @param enable :: [input] bool to enable options or not
*/
void QtReflSettingsView::setPolarisationOptionsEnabled(bool enable) const {

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
  return stitchOptionsLineEdit().text().toStdString();
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

/** Return selected correction type
* @return :: selected correction type
*/
std::string QtReflSettingsView::getDetectorCorrectionType() const {

  return m_ui.detectorCorrectionTypeComboBox->currentText().toStdString();
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
