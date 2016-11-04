#include "MantidQtCustomInterfaces/Reflectometry/QtReflSettingsTabView.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflSettingsTabPresenter.h"
#include "MantidQtMantidWidgets/HintingLineEdit.h"

namespace MantidQt {
namespace CustomInterfaces {

using namespace MantidQt::MantidWidgets;

//----------------------------------------------------------------------------------------------
/** Constructor
* @param parent :: [input] The parent of this widget
*/
QtReflSettingsTabView::QtReflSettingsTabView(QWidget *parent) {

  UNUSED_ARG(parent);
  initLayout();

  m_presenter.reset(new ReflSettingsTabPresenter(this));
}

//----------------------------------------------------------------------------------------------
/** Destructor
*/
QtReflSettingsTabView::~QtReflSettingsTabView() {}

/**
Initialise the Interface
*/
void QtReflSettingsTabView::initLayout() {
  m_ui.setupUi(this);

  connect(m_ui.getExpDefaultsButton, SIGNAL(clicked()), this,
          SLOT(requestExpDefaults()));
  connect(m_ui.getInstDefaultsButton, SIGNAL(clicked()), this,
          SLOT(requestInstDefaults()));
}

/** Returns the presenter managing this view
* @return :: A pointer to the presenter
*/
IReflSettingsTabPresenter *QtReflSettingsTabView::getPresenter() const {

  return m_presenter.get();
}

/** This slot notifies the presenter to fill experiment settings with default
* values.
*/
void QtReflSettingsTabView::requestExpDefaults() const {
  m_presenter->notify(IReflSettingsTabPresenter::ExpDefaultsFlag);
}

/** This slot notifies the presenter to fill instrument settings with default
* values.
*/
void QtReflSettingsTabView::requestInstDefaults() const {
  m_presenter->notify(IReflSettingsTabPresenter::InstDefaultsFlag);
}

/* Sets default values for all experiment settings given a list of default
* values.
*/
void QtReflSettingsTabView::setExpDefaults(
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
  m_ui.scaleFactorEdit->setText(QString::fromStdString(defaults[6]));
}

/* Sets default values for all instrument settings given a list of default
* values.
*/
void QtReflSettingsTabView::setInstDefaults(
    const std::vector<double> &defaults) const {

  auto intMonCheckState = (defaults[0] != 0) ? Qt::Checked : Qt::Unchecked;
  m_ui.intMonCheckBox->setCheckState(intMonCheckState);

  m_ui.monIntMinEdit->setText(QString::number(defaults[1]));
  m_ui.monIntMaxEdit->setText(QString::number(defaults[2]));
  m_ui.monBgMinEdit->setText(QString::number(defaults[3]));
  m_ui.monBgMaxEdit->setText(QString::number(defaults[4]));
  m_ui.lamMinEdit->setText(QString::number(defaults[5]));
  m_ui.lamMaxEdit->setText(QString::number(defaults[6]));
  m_ui.I0MonIndexEdit->setText(QString::number(defaults[7]));
}

/* Sets the enabled status of polarisation corrections and parameters
* @param enable :: [input] bool to enable options or not
*/
void QtReflSettingsTabView::setPolarisationOptionsEnabled(bool enable) const {
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
std::string QtReflSettingsTabView::getStitchOptions() const {

  auto widget = m_ui.expSettingsLayout0->itemAtPosition(7, 1)->widget();
  return static_cast<HintingLineEdit *>(widget)->text().toStdString();
}

/** Creates hints for 'Stitch1DMany'
* @param hints :: Hints as a map
*/
void QtReflSettingsTabView::createStitchHints(
    const std::map<std::string, std::string> &hints) {

  m_ui.expSettingsLayout0->addWidget(new HintingLineEdit(this, hints), 7, 1, 1,
                                     3);
}

/** Return selected analysis mode
* @return :: selected analysis mode
*/
std::string QtReflSettingsTabView::getAnalysisMode() const {

  return m_ui.analysisModeComboBox->currentText().toStdString();
}

/** Return direct beam
* @return :: direct beam range
*/
std::string QtReflSettingsTabView::getDirectBeam() const {

  return m_ui.directBeamEdit->text().toStdString();
}

/** Return selected transmission run(s)
* @return :: selected transmission run(s)
*/
std::string QtReflSettingsTabView::getTransmissionRuns() const {

  return m_ui.transmissionRunsEdit->text().toStdString();
}

/** Return selected polarisation corrections
* @return :: selected polarisation corrections
*/
std::string QtReflSettingsTabView::getPolarisationCorrections() const {

  return m_ui.polCorrComboBox->currentText().toStdString();
}

/** Return CRho
* @return :: polarization correction CRho
*/
std::string QtReflSettingsTabView::getCRho() const {

  return m_ui.CRhoEdit->text().toStdString();
}

/** Return CAlpha
* @return :: polarization correction CAlpha
*/
std::string QtReflSettingsTabView::getCAlpha() const {

  return m_ui.CAlphaEdit->text().toStdString();
}

/** Return CAp
* @return :: polarization correction CAp
*/
std::string QtReflSettingsTabView::getCAp() const {

  return m_ui.CApEdit->text().toStdString();
}

/** Return CPp
* @return :: polarization correction CPp
*/
std::string QtReflSettingsTabView::getCPp() const {

  return m_ui.CPpEdit->text().toStdString();
}

/** Return momentum transfer limits
* @return :: momentum transfer limits
*/
std::string QtReflSettingsTabView::getMomentumTransferStep() const {

  return m_ui.momentumTransferStepEdit->text().toStdString();
}

/** Return scale factor
* @return :: scale factor
*/
std::string QtReflSettingsTabView::getScaleFactor() const {

  return m_ui.scaleFactorEdit->text().toStdString();
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

  return m_ui.monIntMinEdit->text().toStdString();
}

/** Return monitor integral wavelength max
* @return :: monitor integral max
*/
std::string QtReflSettingsTabView::getMonitorIntegralMax() const {

  return m_ui.monIntMaxEdit->text().toStdString();
}

/** Return monitor background wavelength min
* @return :: monitor background min
*/
std::string QtReflSettingsTabView::getMonitorBackgroundMin() const {

  return m_ui.monBgMinEdit->text().toStdString();
}

/** Return monitor background wavelength max
* @return :: monitor background max
*/
std::string QtReflSettingsTabView::getMonitorBackgroundMax() const {

  return m_ui.monBgMaxEdit->text().toStdString();
}

/** Return wavelength min
* @return :: lambda min
*/
std::string QtReflSettingsTabView::getLambdaMin() const {

  return m_ui.lamMinEdit->text().toStdString();
}

/** Return wavelength max
* @return :: lambda max
*/
std::string QtReflSettingsTabView::getLambdaMax() const {

  return m_ui.lamMaxEdit->text().toStdString();
}

/** Return I0MonitorIndex
* @return :: I0MonitorIndex
*/
std::string QtReflSettingsTabView::getI0MonitorIndex() const {

  return m_ui.I0MonIndexEdit->text().toStdString();
}

/** Return processing instructions
* @return :: processing instructions
*/
std::string QtReflSettingsTabView::getProcessingInstructions() const {

  return m_ui.procInstEdit->text().toStdString();
}

} // namespace CustomInterfaces
} // namespace Mantid
