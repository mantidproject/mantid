#include "ConvFit.h"

#include "../General/UserInputValidator.h"

#include "MantidQtWidgets/Common/SignalBlocker.h"
#include "MantidQtWidgets/LegacyQwt/RangeSelector.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument.h"

#include <QDoubleValidator>
#include <QMenu>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>

using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("ConvFit");
}

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

ConvFit::ConvFit(QWidget *parent)
    : IndirectFitAnalysisTab(new ConvFitModel, parent),
      m_uiForm(new Ui::ConvFit) {
  m_uiForm->setupUi(parent);
}

void ConvFit::setupFitTab() {
  m_convFittingModel = dynamic_cast<ConvFitModel *>(fittingModel());
  setSpectrumSelectionView(m_uiForm->svSpectrumView);
  setFitPropertyBrowser(m_uiForm->fitPropertyBrowser);

  setDefaultPeakType("Lorentzian");
  setConvolveMembers(true);

  auto fitRangeSelector = m_uiForm->ppPlotTop->addRangeSelector("ConvFitRange");
  connect(fitRangeSelector, SIGNAL(minValueChanged(double)), this,
          SLOT(xMinSelected(double)));
  connect(fitRangeSelector, SIGNAL(maxValueChanged(double)), this,
          SLOT(xMaxSelected(double)));

  auto backRangeSelector = m_uiForm->ppPlotTop->addRangeSelector(
      "ConvFitBackRange", MantidWidgets::RangeSelector::YSINGLE);
  backRangeSelector->setVisible(false);
  backRangeSelector->setColour(Qt::darkGreen);
  backRangeSelector->setRange(0.0, 1.0);

  auto hwhmRangeSelector = m_uiForm->ppPlotTop->addRangeSelector("ConvFitHWHM");
  hwhmRangeSelector->setColour(Qt::red);

  // Connections
  connect(backRangeSelector, SIGNAL(minValueChanged(double)), this,
          SLOT(backgLevel(double)));
  connect(hwhmRangeSelector, SIGNAL(minValueChanged(double)), this,
          SLOT(hwhmMinChanged(double)));
  connect(hwhmRangeSelector, SIGNAL(maxValueChanged(double)), this,
          SLOT(hwhmMaxChanged(double)));

  // Have FWHM Range linked to Fit Start/End Range
  connect(fitRangeSelector, SIGNAL(rangeChanged(double, double)),
          hwhmRangeSelector, SLOT(setRange(double, double)));
  hwhmRangeSelector->setRange(-1.0, 1.0);
  hwhmRangeSelector->setVisible(false);

  // Initialise fitTypeStrings
  m_fitStrings["None"] = "";
  m_fitStrings["One Lorentzian"] = "1L";
  m_fitStrings["Two Lorentzians"] = "2L";
  m_fitStrings["InelasticDiffSphere"] = "IDS";
  m_fitStrings["InelasticDiffRotDiscreteCircle"] = "IDC";
  m_fitStrings["ElasticDiffSphere"] = "EDS";
  m_fitStrings["ElasticDiffRotDiscreteCircle"] = "EDC";
  m_fitStrings["StretchedExpFT"] = "SFT";

  auto lorentzian = FunctionFactory::Instance().createFunction("Lorentzian");

  auto elasticDiffSphere =
      FunctionFactory::Instance().createFunction("ElasticDiffSphere");
  auto inelasticDiffSphere =
      FunctionFactory::Instance().createFunction("InelasticDiffSphere");

  auto elasticDiffRotDiscCircle = FunctionFactory::Instance().createFunction(
      "ElasticDiffRotDiscreteCircle");
  auto inelasticDiffRotDiscCircle = FunctionFactory::Instance().createFunction(
      "InelasticDiffRotDiscreteCircle");

  auto stretchedExpFT =
      FunctionFactory::Instance().createFunction("StretchedExpFT");

  auto deltaFunction =
      FunctionFactory::Instance().createFunction("DeltaFunction");

  addCheckBoxFunctionGroup("Use Delta Function", {deltaFunction});

  addComboBoxFunctionGroup("One Lorentzian", {lorentzian});
  addComboBoxFunctionGroup("Two Lorentzians", {lorentzian, lorentzian});
  addComboBoxFunctionGroup("InelasticDiffSphere", {inelasticDiffSphere});
  addComboBoxFunctionGroup("InelasticDiffRotDiscreteCircle",
                           {inelasticDiffRotDiscCircle});
  addComboBoxFunctionGroup("ElasticDiffSphere", {elasticDiffSphere});
  addComboBoxFunctionGroup("ElasticDiffRotDiscreteCircle",
                           {elasticDiffRotDiscCircle});
  addComboBoxFunctionGroup("StretchedExpFT", {stretchedExpFT});

  // Set available background options
  setBackgroundOptions({"None", "FlatBackground", "LinearBackground"});

  addBoolCustomSetting("ExtractMembers", "Extract Members");
  addOptionalDoubleSetting("TempCorrection", "Temp. Correction",
                           "UseTempCorrection", "Use Temp. Correction");
  setCustomSettingChangesFunction("TempCorrection", true);
  setCustomSettingChangesFunction("UseTempCorrection", true);

  // Instrument resolution
  m_properties["InstrumentResolution"] =
      m_dblManager->addProperty("InstrumentResolution");

  disablePlotGuess();
  disablePlotPreview();

  // Replot input automatically when file / spec no changes
  connect(m_uiForm->spPlotSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(setSelectedSpectrum(int)));

  connect(m_uiForm->dsSampleInput, SIGNAL(dataReady(const QString &)), this,
          SLOT(newDataLoaded(const QString &)));
  connect(m_uiForm->dsResInput, SIGNAL(dataReady(const QString &)), this,
          SLOT(setModelResolution(const QString &)));
  connect(m_uiForm->dsResInput, SIGNAL(dataReady(const QString &)), this,
          SLOT(updateGuessPlots()));
  connect(m_uiForm->pbSingleFit, SIGNAL(clicked()), this, SLOT(singleFit()));

  // Post Plot and Save
  connect(m_uiForm->pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));
  connect(m_uiForm->pbPlot, SIGNAL(clicked()), this, SLOT(plotClicked()));
  connect(m_uiForm->pbPlotPreview, SIGNAL(clicked()), this,
          SLOT(plotCurrentPreview()));
  connect(m_uiForm->ckPlotGuess, SIGNAL(stateChanged(int)), this,
          SLOT(updatePlotGuess()));

  connect(this, SIGNAL(parameterChanged(const Mantid::API::IFunction *)), this,
          SLOT(parameterUpdated(const Mantid::API::IFunction *)));
  connect(this, SIGNAL(functionChanged()), this, SLOT(fitFunctionChanged()));
}

void ConvFit::setupFit(Mantid::API::IAlgorithm_sptr fitAlgorithm) {
  if (boolSettingValue("UseTempCorrection"))
    m_convFittingModel->setTemperature(doubleSettingValue("TempCorrection"));
  else
    m_convFittingModel->setTemperature(boost::none);
  fitAlgorithm->setProperty("ExtractMembers",
                            boolSettingValue("ExtractMembers"));
  IndirectFitAnalysisTab::setupFit(fitAlgorithm);
}

bool ConvFit::canPlotGuess() const {
  return m_uiForm->dsResInput->isValid() &&
         IndirectFitAnalysisTab::canPlotGuess();
}

bool ConvFit::doPlotGuess() const {
  return m_uiForm->ckPlotGuess->isEnabled() &&
         m_uiForm->ckPlotGuess->isChecked();
}

void ConvFit::setModelResolution(const QString &resolutionName) {
  const auto name = resolutionName.toStdString();
  const auto resolution =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(name);
  m_convFittingModel->setResolution(resolution, 0);
  setModelFitFunction();
}

/**
 * Handles saving the workspace when save is clicked
 */
void ConvFit::saveClicked() { IndirectFitAnalysisTab::saveResult(); }

/**
 * Handles plotting the workspace when plot is clicked
 */
void ConvFit::plotClicked() {
  IndirectFitAnalysisTab::plotResult(m_uiForm->cbPlotType->currentText());
}

void ConvFit::updatePlotOptions() {
  IndirectFitAnalysisTab::updatePlotOptions(m_uiForm->cbPlotType);
}

/**
 * Validates the user's inputs in the ConvFit tab.
 * @return If the validation was successful
 */
bool ConvFit::validate() {
  if (!IndirectFitAnalysisTab::validate())
    return false;

  UserInputValidator uiv;

  uiv.checkDataSelectorIsValid("Sample Input", m_uiForm->dsSampleInput);
  uiv.checkDataSelectorIsValid("Resolution Input", m_uiForm->dsResInput);

  uiv.checkValidRange("Fitting Range", std::make_pair(startX(), endX()));

  auto compositeModel = boost::dynamic_pointer_cast<CompositeFunction>(model());
  // Enforce the rule that at least one fit is needed; either a delta function,
  // one or two Lorentzian functions,
  // or both.  (The resolution function must be convolved with a model.)
  if (isEmptyModel())
    uiv.addErrorMessage("No fit function has been selected");
  else if (compositeModel && compositeModel->nFunctions() == 1 &&
           compositeModel->getFunction(0)->name() == "DeltaFunction")
    uiv.addErrorMessage(
        "Fit function is invalid; only a Delta Function has been supplied");

  const auto error = uiv.generateErrorMessage();
  emit showMessageBox(error);
  return error.isEmpty();
}

/**
 * Reads in settings files
 * @param settings The name of the QSettings object to retrieve data from
 */
void ConvFit::loadSettings(const QSettings &settings) {
  m_uiForm->dsSampleInput->readSettings(settings.group());
  m_uiForm->dsResInput->readSettings(settings.group());
}

/**
 * Called when new data has been loaded by the data selector.
 *
 * Configures ranges for spin boxes before raw plot is done.
 *
 * @param wsName Name of new workspace loaded
 */
void ConvFit::newDataLoaded(const QString &wsName) {
  IndirectFitAnalysisTab::newInputDataLoaded(wsName);
  updateHWHMFromResolution();

  const int maxWsIndex = static_cast<int>(
      m_convFittingModel->getWorkspace(0)->getNumberHistograms() - 1);

  m_uiForm->spPlotSpectrum->setMaximum(maxWsIndex);
  m_uiForm->spPlotSpectrum->setMinimum(0);
  m_uiForm->spPlotSpectrum->setValue(0);
}

void ConvFit::fitFunctionChanged() {
  auto hwhmRangeSelector = m_uiForm->ppPlotTop->getRangeSelector("ConvFitHWHM");
  auto backRangeSelector =
      m_uiForm->ppPlotTop->getRangeSelector("ConvFitBackRange");

  if (selectedFitType().contains("Lorentzian"))
    hwhmRangeSelector->setVisible(true);
  else
    hwhmRangeSelector->setVisible(false);

  if (backgroundName() == "None")
    backRangeSelector->setVisible(false);
  else
    backRangeSelector->setVisible(true);
  m_convFittingModel->setFitTypeString(fitTypeString());
}

void ConvFit::parameterUpdated(const Mantid::API::IFunction *function) {
  if (function == nullptr)
    return;

  if (background() && function->asString() == background()->asString()) {
    auto rangeSelector =
        m_uiForm->ppPlotTop->getRangeSelector("ConvFitBackRange");
    MantidQt::API::SignalBlocker<QObject> blocker(rangeSelector);
    rangeSelector->setMinimum(function->getParameter("A0"));
  } else if (function->hasParameter("FWHM")) {
    auto rangeSelector = m_uiForm->ppPlotTop->getRangeSelector("ConvFitHWHM");
    auto peakCentre =
        lastParameterValue(function->name(), "PeakCentre").get_value_or(0);
    auto hwhm =
        lastParameterValue(function->name(), "FWHM").get_value_or(0) / 2.0;
    MantidQt::API::SignalBlocker<QObject> blocker(rangeSelector);
    rangeSelector->setMaximum(peakCentre + hwhm);
    rangeSelector->setMinimum(peakCentre - hwhm);
  }
}

/**
 * Generate a string to describe the fit type selected by the user.
 * Used when naming the resultant workspaces.
 *
 * Assertions used to guard against any future changes that don't take
 * workspace naming into account.
 *
 * @returns the generated QString.
 */
std::string ConvFit::fitTypeString() const {
  std::string fitType;

  if (numberOfCustomFunctions("DeltaFunction") > 0)
    fitType += "Delta";

  fitType += m_fitStrings[selectedFitType()];

  return fitType;
}

/**
 * Updates the plot in the GUI window
 */
void ConvFit::updatePreviewPlots() {
  IndirectFitAnalysisTab::updatePlots(m_uiForm->ppPlotTop,
                                      m_uiForm->ppPlotBottom);
}

void ConvFit::updatePlotRange() {
  auto rangeSelector = m_uiForm->ppPlotTop->getRangeSelector("ConvFitRange");
  if (m_uiForm->ppPlotTop->hasCurve("Sample")) {
    const auto range = m_uiForm->ppPlotTop->getCurveRange("Sample");
    rangeSelector->setRange(range.first, range.second);
  }
}

void ConvFit::disablePlotGuess() { m_uiForm->ckPlotGuess->setEnabled(false); }

void ConvFit::enablePlotGuess() { m_uiForm->ckPlotGuess->setEnabled(true); }

void ConvFit::setPlotResultEnabled(bool enabled) {
  m_uiForm->pbPlot->setEnabled(enabled);
  m_uiForm->cbPlotType->setEnabled(enabled);
}

void ConvFit::setSaveResultEnabled(bool enabled) {
  m_uiForm->pbSave->setEnabled(enabled);
}

void ConvFit::enablePlotPreview() { m_uiForm->pbPlotPreview->setEnabled(true); }

void ConvFit::disablePlotPreview() {
  m_uiForm->pbPlotPreview->setEnabled(false);
}

void ConvFit::addGuessPlot(Mantid::API::MatrixWorkspace_sptr workspace) {
  m_uiForm->ppPlotTop->addSpectrum("Guess", workspace, 0, Qt::green);
}

void ConvFit::removeGuessPlot() {
  m_uiForm->ppPlotTop->removeSpectrum("Guess");
  m_uiForm->ckPlotGuess->setChecked(false);
}

void ConvFit::startXChanged(double startX) {
  auto rangeSelector = m_uiForm->ppPlotTop->getRangeSelector("ConvFitRange");
  MantidQt::API::SignalBlocker<QObject> blocker(rangeSelector);
  rangeSelector->setMinimum(startX);
}

void ConvFit::endXChanged(double endX) {
  auto rangeSelector = m_uiForm->ppPlotTop->getRangeSelector("ConvFitRange");
  MantidQt::API::SignalBlocker<QObject> blocker(rangeSelector);
  rangeSelector->setMaximum(endX);
}

void ConvFit::hwhmMinChanged(double val) {
  const auto peakCentre =
      lastParameterValue("Lorentzian", "PeakCentre").get_value_or(0);
  const double difference = peakCentre - val;

  auto hwhmRangeSelector = m_uiForm->ppPlotTop->getRangeSelector("ConvFitHWHM");
  MantidQt::API::SignalBlocker<QObject> blocker(hwhmRangeSelector);
  hwhmRangeSelector->setMaximum(peakCentre + difference);
  fwhmChanged(std::fabs(difference) * 2.0);
}

void ConvFit::hwhmMaxChanged(double val) {
  const double peakCentre =
      lastParameterValue("Lorentzian", "PeakCentre").get_value_or(0);
  const double difference = val - peakCentre;

  auto hwhmRangeSelector = m_uiForm->ppPlotTop->getRangeSelector("ConvFitHWHM");
  MantidQt::API::SignalBlocker<QObject> blocker(hwhmRangeSelector);
  hwhmRangeSelector->setMinimum(peakCentre - difference);
  fwhmChanged(std::fabs(difference) * 2.0);
}

void ConvFit::fwhmChanged(double fwhm) {
  // Update the property
  m_convFittingModel->setDefaultParameterValue("FWHM", fwhm, 0);
  setParameterValue("Lorentzian", "FWHM", fwhm);
}

void ConvFit::backgLevel(double val) {
  m_convFittingModel->setDefaultParameterValue("A0", val, 0);
  setParameterValue("LinearBackground", "A0", val);
  setParameterValue("FlatBackground", "A0", val);
}

void ConvFit::updateHWHMFromResolution() {
  auto resolution =
      m_convFittingModel->getInstrumentResolution(0).get_value_or(0);
  if (resolution > 0 && selectedFitType().contains("Lorentzian"))
    hwhmMaxChanged(resolution / 2.0);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
