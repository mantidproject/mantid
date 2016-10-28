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

/** Returns global options for 'Plus' algorithm
* @return :: Global options for 'Plus' algorithm
*/
std::string QtReflSettingsTabView::getPlusOptions() const {

  auto widget = m_ui.optionsLayout->itemAtPosition(1, 2)->widget();
  return static_cast<HintingLineEdit *>(widget)->text().toStdString();
}

/** Returns global options for 'CreateTransmissionWorkspaceAuto'
* @return :: Global options for 'CreateTransmissionWorkspaceAuto'
*/
std::string QtReflSettingsTabView::getTransmissionOptions() const {

  auto widget = m_ui.optionsLayout->itemAtPosition(2, 2)->widget();
  return static_cast<HintingLineEdit *>(widget)->text().toStdString();
}

/** Returns global options for 'ReflectometryReductionOneAuto'
* @return :: Global options for 'ReflectometryReductionOneAuto'
*/
std::string QtReflSettingsTabView::getReductionOptions() const {

  auto widget = m_ui.optionsLayout->itemAtPosition(3, 2)->widget();
  return static_cast<HintingLineEdit *>(widget)->text().toStdString();
}

/** Returns global options for 'Stitch1DMany'
* @return :: Global options for 'Stitch1DMany'
*/
std::string QtReflSettingsTabView::getStitchOptions() const {

  auto widget = m_ui.optionsLayout->itemAtPosition(4, 2)->widget();
  return static_cast<HintingLineEdit *>(widget)->text().toStdString();
}

/** Creates hints for 'Plus'
* @param hints :: Hints as a map
*/
void QtReflSettingsTabView::createPlusHints(
    const std::map<std::string, std::string> &hints) {

  m_ui.optionsLayout->addWidget(new HintingLineEdit(this, hints), 1, 2);
}

/** Creates hints for 'CreateTransmissionWorkspaceAuto'
* @param hints :: Hints as a map
*/
void QtReflSettingsTabView::createTransmissionHints(
    const std::map<std::string, std::string> &hints) {

  m_ui.optionsLayout->addWidget(new HintingLineEdit(this, hints), 2, 2);
}

/** Creates hints for 'ReflectometryReductionOneAuto'
* @param hints :: Hints as a map
*/
void QtReflSettingsTabView::createReductionHints(
    const std::map<std::string, std::string> &hints) {

  m_ui.optionsLayout->addWidget(new HintingLineEdit(this, hints), 3, 2);
}

/** Creates hints for 'Stitch1DMany'
* @param hints :: Hints as a map
*/
void QtReflSettingsTabView::createStitchHints(
    const std::map<std::string, std::string> &hints) {

  m_ui.optionsLayout->addWidget(new HintingLineEdit(this, hints), 4, 2);
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

  m_ui.expSettingsScaleFactorEdit->setText(QString::fromStdString(defaults[2]));
}

/* Sets default values for all instrument settings given a list of default
* values.
*/
void QtReflSettingsTabView::setInstDefaults(
    const std::vector<double> &defaults) const {

  m_ui.instSettingsMonIntMinEdit->setText(QString::number(defaults[0]));
  m_ui.instSettingsMonIntMaxEdit->setText(QString::number(defaults[1]));
  m_ui.instSettingsMonBgMinEdit->setText(QString::number(defaults[2]));
  m_ui.instSettingsMonBgMaxEdit->setText(QString::number(defaults[3]));
  m_ui.instSettingsLamMinEdit->setText(QString::number(defaults[4]));
  m_ui.instSettingsLamMaxEdit->setText(QString::number(defaults[5]));
  m_ui.instSettingsI0MonIndexEdit->setText(QString::number(defaults[6]));
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

  return m_ui.expSettingsDirectBeamEdit->text().toStdString();
}

/** Return CRho
* @return :: polarization correction CRho
*/
std::string QtReflSettingsTabView::getCRho() const {

  return m_ui.expSettingsCRhoEdit->text().toStdString();
}

/** Return CAlpha
* @return :: polarization correction CAlpha
*/
std::string QtReflSettingsTabView::getCAlpha() const {

  return m_ui.expSettingsCAlphaEdit->text().toStdString();
}

/** Return CAp
* @return :: polarization correction CAp
*/
std::string QtReflSettingsTabView::getCAp() const {

  return m_ui.expSettingsCApEdit->text().toStdString();
}

/** Return CPp
* @return :: polarization correction CPp
*/
std::string QtReflSettingsTabView::getCPp() const {

  return m_ui.expSettingsCPpEdit->text().toStdString();
}

/** Return momentum transfer limits
* @return :: momentum transfer limits
*/
std::string QtReflSettingsTabView::getMomentumTransferLimits() const {

  return m_ui.expSettingsMomentumTransferLimitsEdit->text().toStdString();
}

/** Return detector limits
* @return :: detector limits
*/
std::string QtReflSettingsTabView::getDetectorLimits() const {

  return m_ui.expSettingsDetLimitsEdit->text().toStdString();
}

/** Return scale factor
* @return :: scale factor
*/
std::string QtReflSettingsTabView::getScaleFactor() const {

  return m_ui.expSettingsScaleFactorEdit->text().toStdString();
}

/** Return selected polarisation corrections
* @return :: selected polarisation corrections
*/
std::string QtReflSettingsTabView::getPolarisationCorrections() const {

  return m_ui.polCorrComboBox->currentText().toStdString();
}

/** Return monitor integral wavelength min
* @return :: monitor integral min
*/
std::string QtReflSettingsTabView::getMonitorIntegralMin() const {

  return m_ui.instSettingsMonIntMinEdit->text().toStdString();
}

/** Return monitor integral wavelength max
* @return :: monitor integral max
*/
std::string QtReflSettingsTabView::getMonitorIntegralMax() const {

  return m_ui.instSettingsMonIntMaxEdit->text().toStdString();
}

/** Return monitor background wavelength min
* @return :: monitor background min
*/
std::string QtReflSettingsTabView::getMonitorBackgroundMin() const {

  return m_ui.instSettingsMonBgMinEdit->text().toStdString();
}

/** Return monitor background wavelength max
* @return :: monitor background max
*/
std::string QtReflSettingsTabView::getMonitorBackgroundMax() const {

  return m_ui.instSettingsMonBgMaxEdit->text().toStdString();
}

/** Return wavelength min
* @return :: lambda min
*/
std::string QtReflSettingsTabView::getLambdaMin() const {

  return m_ui.instSettingsLamMinEdit->text().toStdString();
}

/** Return wavelength max
* @return :: lambda max
*/
std::string QtReflSettingsTabView::getLambdaMax() const {

  return m_ui.instSettingsLamMaxEdit->text().toStdString();
}

/** Return I0MonitorIndex
* @return :: I0MonitorIndex
*/
std::string QtReflSettingsTabView::getI0MonitorIndex() const {

  return m_ui.instSettingsI0MonIndexEdit->text().toStdString();
}

} // namespace CustomInterfaces
} // namespace Mantid
