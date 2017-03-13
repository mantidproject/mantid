#include "MantidQtCustomInterfaces/Reflectometry/QtReflSettingsView.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflSettingsPresenter.h"
#include "MantidQtMantidWidgets/HintingLineEdit.h"

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

/* Sets the enabled status of polarisation corrections and parameters
* @param enable :: [input] bool to enable options or not
*/
void QtReflSettingsView::setPolarisationOptionsEnabled(bool enable) const {
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
  std::string stitchOptionsText;
  if (widget->isEnabled())
    stitchOptionsText =
        static_cast<HintingLineEdit *>(widget)->text().toStdString();

  return stitchOptionsText;
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

  std::string analysisModeText;
  if (m_ui.analysisModeComboBox->isEnabled())
    analysisModeText = m_ui.analysisModeComboBox->currentText().toStdString();

  return analysisModeText;
}

/** Return direct beam
* @return :: direct beam range
*/
std::string QtReflSettingsView::getDirectBeam() const {

  std::string directBeamText;
  if (m_ui.directBeamEdit->isEnabled())
    directBeamText = m_ui.directBeamEdit->text().toStdString();

  return directBeamText;
}

/** Return selected transmission run(s)
* @return :: selected transmission run(s)
*/
std::string QtReflSettingsView::getTransmissionRuns() const {

  std::string transmissionRunsText;
  if (m_ui.transmissionRunsEdit->isEnabled())
    transmissionRunsText = m_ui.transmissionRunsEdit->text().toStdString();

  return transmissionRunsText;
}

/** Return start overlap
* @return :: start overlap
*/
std::string QtReflSettingsView::getStartOverlap() const {

  std::string startOverlapText;
  if (m_ui.startOverlapEdit->isEnabled())
    startOverlapText = m_ui.startOverlapEdit->text().toStdString();

  return startOverlapText;
}

/** Return end overlap
* @return :: end overlap
*/
std::string QtReflSettingsView::getEndOverlap() const {

  std::string endOverlapText;
  if (m_ui.endOverlapEdit->isEnabled())
    endOverlapText = m_ui.endOverlapEdit->text().toStdString();

  return endOverlapText;
}

/** Return selected polarisation corrections
* @return :: selected polarisation corrections
*/
std::string QtReflSettingsView::getPolarisationCorrections() const {

  std::string polCorrText;
  if (m_ui.polCorrComboBox->isEnabled())
    polCorrText = m_ui.polCorrComboBox->currentText().toStdString();

  return polCorrText;
}

/** Return CRho
* @return :: polarization correction CRho
*/
std::string QtReflSettingsView::getCRho() const {

  std::string CRhoText;
  if (m_ui.CRhoEdit->isEnabled())
    CRhoText = m_ui.CRhoEdit->text().toStdString();

  return CRhoText;
}

/** Return CAlpha
* @return :: polarization correction CAlpha
*/
std::string QtReflSettingsView::getCAlpha() const {

  std::string CAlphaText;
  if (m_ui.CAlphaEdit->isEnabled())
    CAlphaText = m_ui.CAlphaEdit->text().toStdString();

  return CAlphaText;
}

/** Return CAp
* @return :: polarization correction CAp
*/
std::string QtReflSettingsView::getCAp() const {

  std::string CApText;
  if (m_ui.CApEdit->isEnabled())
    CApText = m_ui.CAlphaEdit->text().toStdString();

  return CApText;
}

/** Return CPp
* @return :: polarization correction CPp
*/
std::string QtReflSettingsView::getCPp() const {

  std::string CPpText;
  if (m_ui.CPpEdit->isEnabled())
    CPpText = m_ui.CPpEdit->text().toStdString();

  return CPpText;
}

/** Return momentum transfer limits
* @return :: momentum transfer limits
*/
std::string QtReflSettingsView::getMomentumTransferStep() const {

  std::string momentumTransferText;
  if (m_ui.momentumTransferStepEdit->isEnabled())
    momentumTransferText = m_ui.momentumTransferStepEdit->text().toStdString();

  return momentumTransferText;
}

/** Return scale factor
* @return :: scale factor
*/
std::string QtReflSettingsView::getScaleFactor() const {

  std::string scaleFactorText;
  if (m_ui.scaleFactorEdit->isEnabled())
    scaleFactorText = m_ui.scaleFactorEdit->text().toStdString();

  return scaleFactorText;
}

/** Return integrated monitors option
* @return :: integrated monitors check
*/
std::string QtReflSettingsView::getIntMonCheck() const {

  std::string intMonCheckText;
  if (m_ui.intMonCheckBox->isEnabled())
    intMonCheckText = m_ui.intMonCheckBox->isChecked() ? "1" : "0";

  return intMonCheckText;
}

/** Return monitor integral wavelength min
* @return :: monitor integral min
*/
std::string QtReflSettingsView::getMonitorIntegralMin() const {

  std::string monIntMinText;
  if (m_ui.monIntMinEdit->isEnabled())
    monIntMinText = m_ui.monIntMinEdit->text().toStdString();

  return monIntMinText;
}

/** Return monitor integral wavelength max
* @return :: monitor integral max
*/
std::string QtReflSettingsView::getMonitorIntegralMax() const {

  std::string monIntMaxText;
  if (m_ui.monIntMaxEdit->isEnabled())
    monIntMaxText = m_ui.monIntMaxEdit->text().toStdString();

  return monIntMaxText;
}

/** Return monitor background wavelength min
* @return :: monitor background min
*/
std::string QtReflSettingsView::getMonitorBackgroundMin() const {

  std::string monBgMinText;
  if (m_ui.monBgMinEdit->isEnabled())
    monBgMinText = m_ui.monBgMinEdit->text().toStdString();

  return monBgMinText;
}

/** Return monitor background wavelength max
* @return :: monitor background max
*/
std::string QtReflSettingsView::getMonitorBackgroundMax() const {

  std::string monBgMaxText;
  if (m_ui.monBgMaxEdit->isEnabled())
    monBgMaxText = m_ui.monBgMaxEdit->text().toStdString();

  return monBgMaxText;
}

/** Return wavelength min
* @return :: lambda min
*/
std::string QtReflSettingsView::getLambdaMin() const {

  std::string lamMinText;
  if (m_ui.lamMinEdit->isEnabled())
    lamMinText = m_ui.lamMinEdit->text().toStdString();

  return lamMinText;
}

/** Return wavelength max
* @return :: lambda max
*/
std::string QtReflSettingsView::getLambdaMax() const {

  std::string lamMaxText;
  if (m_ui.lamMaxEdit->isEnabled())
    lamMaxText = m_ui.lamMaxEdit->text().toStdString();

  return lamMaxText;
}

/** Return I0MonitorIndex
* @return :: I0MonitorIndex
*/
std::string QtReflSettingsView::getI0MonitorIndex() const {

  std::string I0MonIndexText;
  if (m_ui.I0MonIndexEdit->isEnabled())
    I0MonIndexText = m_ui.I0MonIndexEdit->text().toStdString();

  return I0MonIndexText;
}

/** Return processing instructions
* @return :: processing instructions
*/
std::string QtReflSettingsView::getProcessingInstructions() const {

  std::string procInstText;
  if (m_ui.procInstEdit->isEnabled())
    procInstText = m_ui.procInstEdit->text().toStdString();

  return procInstText;
}

/** Return selected correction type
* @return :: selected correction type
*/
std::string QtReflSettingsView::getDetectorCorrectionType() const {

  std::string detCorrText;
  if (m_ui.detectorCorrectionTypeComboBox->isEnabled())
    detCorrText =
        m_ui.detectorCorrectionTypeComboBox->currentText().toStdString();

  return detCorrText;
}

} // namespace CustomInterfaces
} // namespace Mantid
