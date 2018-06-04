#include "IqtFit.h"

#include "../General/UserInputValidator.h"

#include "MantidQtWidgets/Common/SignalBlocker.h"
#include "MantidQtWidgets/LegacyQwt/RangeSelector.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/WorkspaceGroup.h"

#include <QMenu>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>

#include <boost/algorithm/string/predicate.hpp>

using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("IqtFit");
}

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IqtFit::IqtFit(QWidget *parent)
    : IndirectFitAnalysisTab(parent), m_uiForm(new Ui::IqtFit) {
  m_uiForm->setupUi(parent);
  IndirectFitAnalysisTab::addPropertyBrowserToUI(m_uiForm.get());
}

void IqtFit::setup() {
  setMinimumSpectrum(0);
  setMaximumSpectrum(0);

  m_uiForm->ckPlotGuess->setChecked(false);
  disablePlotGuess();
  disablePlotPreview();

  // Create custom function groups
  const auto exponential =
      FunctionFactory::Instance().createFunction("ExpDecay");
  const auto stretchedExponential =
      FunctionFactory::Instance().createFunction("StretchExp");
  addSpinnerFunctionGroup("Exponential", {exponential}, 0, 2);
  addCheckBoxFunctionGroup("Stretched Exponential", {stretchedExponential});

  // Add custom settings
  addBoolCustomSetting("ConstrainIntensities", "Constrain Intensities");
  addBoolCustomSetting("ConstrainBeta", "Make Beta Global");
  addBoolCustomSetting("ExtractMembers", "Extract Members");
  setCustomSettingEnabled("ConstrainBeta", false);
  setCustomSettingEnabled("ConstrainIntensities", false);

  // Set available background options
  setBackgroundOptions({"None", "FlatBackground"});

  auto fitRangeSelector = m_uiForm->ppPlotTop->addRangeSelector("IqtFitRange");
  connect(fitRangeSelector, SIGNAL(minValueChanged(double)), this,
          SLOT(xMinSelected(double)));
  connect(fitRangeSelector, SIGNAL(maxValueChanged(double)), this,
          SLOT(xMaxSelected(double)));

  auto backRangeSelector = m_uiForm->ppPlotTop->addRangeSelector(
      "IqtFitBackRange", MantidWidgets::RangeSelector::YSINGLE);
  backRangeSelector->setVisible(false);
  backRangeSelector->setColour(Qt::darkGreen);
  backRangeSelector->setRange(0.0, 1.0);
  connect(backRangeSelector, SIGNAL(minValueChanged(double)), this,
          SLOT(backgroundSelectorChanged(double)));

  // Signal/slot ui connections
  connect(m_uiForm->dsSampleInput, SIGNAL(dataReady(const QString &)), this,
          SLOT(newDataLoaded(const QString &)));
  connect(m_uiForm->pbSingle, SIGNAL(clicked()), this, SLOT(singleFit()));

  // Update plot when fit type changes
  connect(m_uiForm->spPlotSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(setSelectedSpectrum(int)));
  connect(m_uiForm->spPlotSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(updatePreviewPlots()));

  connect(m_uiForm->spSpectraMin, SIGNAL(valueChanged(int)), this,
          SLOT(specMinChanged(int)));
  connect(m_uiForm->spSpectraMax, SIGNAL(valueChanged(int)), this,
          SLOT(specMaxChanged(int)));

  connect(m_uiForm->pbPlot, SIGNAL(clicked()), this, SLOT(plotWorkspace()));
  connect(m_uiForm->pbSave, SIGNAL(clicked()), this, SLOT(saveResult()));
  connect(m_uiForm->pbPlotPreview, SIGNAL(clicked()), this,
          SLOT(plotCurrentPreview()));

  connect(m_uiForm->ckPlotGuess, SIGNAL(stateChanged(int)), this,
          SLOT(updatePlotGuess()));

  connect(this, SIGNAL(parameterChanged(const Mantid::API::IFunction *)), this,
          SLOT(parameterUpdated(const Mantid::API::IFunction *)));
  connect(this, SIGNAL(functionChanged()), this, SLOT(fitFunctionChanged()));
  connect(this, SIGNAL(customBoolChanged(const QString &, bool)), this,
          SLOT(customBoolUpdated(const QString &, bool)));
}

int IqtFit::minimumSpectrum() const { return m_uiForm->spSpectraMin->value(); }

int IqtFit::maximumSpectrum() const { return m_uiForm->spSpectraMax->value(); }

void IqtFit::fitFunctionChanged() {
  auto backRangeSelector =
      m_uiForm->ppPlotTop->getRangeSelector("IqtFitBackRange");

  if (backgroundName() == "None")
    backRangeSelector->setVisible(false);
  else
    backRangeSelector->setVisible(true);

  if (numberOfCustomFunctions("StretchExp") > 0) {
    setCustomSettingEnabled("ConstrainBeta", true);
  } else {
    setCustomBoolSetting("ConstrainBeta", false);
    setCustomSettingEnabled("ConstrainBeta", false);
  }
  updateIntensityTie();
}

void IqtFit::customBoolUpdated(const QString &key, bool value) {
  if (key == "Constrain Intensities") {
    if (value)
      updateIntensityTie();
    else
      removeTie(m_tiedParameter);
  }
}

void IqtFit::updateIntensityTie() {
  const auto function = fitFunction();

  if (function) {
    removeTie(m_tiedParameter);
    const auto tie = QString::fromStdString(createIntensityTie(fitFunction()));
    updateIntensityTie(tie);
  } else {
    setCustomBoolSetting("ConstrainIntensities", false);
    setCustomSettingEnabled("ConstrainIntensities", false);
  }
}

void IqtFit::updateIntensityTie(const QString &intensityTie) {

  if (intensityTie.isEmpty()) {
    setCustomBoolSetting("ConstrainIntensities", false);
    setCustomSettingEnabled("ConstrainIntensities", false);
  } else {
    setCustomSettingEnabled("ConstrainIntensities", true);

    if (boolSettingValue("ConstrainIntensities")) {
      m_tiedParameter = intensityTie.split("=")[0];
      addTie(intensityTie);
    }
  }
}

bool IqtFit::doPlotGuess() const {
  return m_uiForm->ckPlotGuess->isEnabled() &&
         m_uiForm->ckPlotGuess->isChecked();
}

std::string IqtFit::fitTypeString() const {
  const auto numberOfExponential = numberOfCustomFunctions("ExpDecay");
  const auto numberOfStretched = numberOfCustomFunctions("StretchExp");

  if (numberOfExponential > 0)
    return std::to_string(numberOfExponential) + "E";

  if (numberOfStretched > 0)
    return std::to_string(numberOfStretched) + "S";

  return "";
}

MatrixWorkspace_sptr IqtFit::fitWorkspace() const {
  auto replaceAlg = replaceInfinityAndNaN(inputWorkspace());
  replaceAlg->execute();
  return replaceAlg->getProperty("OutputWorkspace");
}

std::string IqtFit::constructBaseName() const {
  auto outputName = inputWorkspace()->getName();

  // Remove _red
  const auto cutIndex = outputName.find_last_of('_');
  if (cutIndex != std::string::npos)
    outputName = outputName.substr(0, cutIndex);

  outputName += "_IqtFit_" + fitTypeString();

  if (boolSettingValue("ConstrainBeta"))
    outputName += "mult";
  return outputName + "_s";
}

std::string IqtFit::createSingleFitOutputName() const {
  return constructBaseName() + std::to_string(selectedSpectrum());
}

std::string IqtFit::createSequentialFitOutputName() const {
  const auto minSpectrum = std::to_string(minimumSpectrum());
  const auto maxSpectrum = std::to_string(maximumSpectrum());
  return constructBaseName() + minSpectrum + "_to_" + maxSpectrum;
}

IAlgorithm_sptr IqtFit::singleFitAlgorithm() const {
  size_t specNo = static_cast<size_t>(m_uiForm->spPlotSpectrum->value());
  return iqtFitAlgorithm(specNo, specNo);
}

IAlgorithm_sptr IqtFit::sequentialFitAlgorithm() const {
  size_t specMin = static_cast<size_t>(m_uiForm->spSpectraMin->value());
  size_t specMax = static_cast<size_t>(m_uiForm->spSpectraMax->value());
  return iqtFitAlgorithm(specMin, specMax);
}

IAlgorithm_sptr IqtFit::iqtFitAlgorithm(const size_t &specMin,
                                        const size_t &specMax) const {
  const auto outputName = outputWorkspaceName(specMin);
  const bool constrainBeta = boolSettingValue("ConstrainBeta");
  const bool extractMembers = boolSettingValue("ExtractMembers");

  IAlgorithm_sptr iqtFitAlg;

  if (constrainBeta)
    iqtFitAlg = AlgorithmManager::Instance().create("IqtFitMultiple");
  else
    iqtFitAlg = AlgorithmManager::Instance().create("IqtFitSequential");

  iqtFitAlg->initialize();
  iqtFitAlg->setProperty("SpecMin", static_cast<int>(specMin));
  iqtFitAlg->setProperty("SpecMax", static_cast<int>(specMax));
  iqtFitAlg->setProperty("ExtractMembers", extractMembers);
  iqtFitAlg->setProperty("OutputWorkspace", outputName + "_Result");
  return iqtFitAlg;
}

std::string IqtFit::createIntensityTie(IFunction_sptr function) const {
  std::string tieString = "1";
  const auto backIndex = backgroundIndex();
  const auto intensityParameters = getParameters(function, "Height");

  if (backIndex && !intensityParameters.empty())
    tieString += "-f" + std::to_string(backIndex.get()) + ".A0";
  else if (intensityParameters.size() < 2)
    return "";

  for (auto i = 1u; i < intensityParameters.size(); ++i)
    tieString += "-" + intensityParameters[i];
  return intensityParameters[0] + "=" + tieString;
}

std::vector<std::string>
IqtFit::getParameters(IFunction_sptr function,
                      const std::string &shortParameterName) const {
  std::vector<std::string> parameters;

  for (const auto &longName : function->getParameterNames()) {
    if (boost::algorithm::ends_with(longName, shortParameterName))
      parameters.push_back(longName);
  }
  return parameters;
}

void IqtFit::updatePlotOptions() {
  IndirectFitAnalysisTab::updatePlotOptions(m_uiForm->cbPlotType);
}

void IqtFit::enablePlotResult() { m_uiForm->pbPlot->setEnabled(true); }

void IqtFit::disablePlotResult() { m_uiForm->pbPlot->setEnabled(false); }

void IqtFit::enableSaveResult() { m_uiForm->pbSave->setEnabled(true); }

void IqtFit::disableSaveResult() { m_uiForm->pbSave->setEnabled(false); }

void IqtFit::enablePlotPreview() { m_uiForm->pbPlotPreview->setEnabled(true); }

void IqtFit::disablePlotPreview() {
  m_uiForm->pbPlotPreview->setEnabled(false);
}

/**
 * Plot workspace based on user input
 */
void IqtFit::plotWorkspace() {
  IndirectFitAnalysisTab::plotResult(outputWorkspaceName() + "_Result",
                                     m_uiForm->cbPlotType->currentText());
}

/**
 * Save the result of the algorithm
 */
void IqtFit::saveResult() {
  IndirectFitAnalysisTab::saveResult(outputWorkspaceName() + "_Result");
}

/**
 * Handles completion of the IqtFitMultiple algorithm.
 * @param error True if the algorithm was stopped due to error, false otherwise
 * @param outputWsName The name of the output workspace
 */
void IqtFit::algorithmComplete(bool error) {

  if (error) {
    QString msg =
        "There was an error executing the fitting algorithm. Please see the "
        "Results Log pane for more details.";
    showMessageBox(msg);
    return;
  }

  IndirectFitAnalysisTab::fitAlgorithmComplete(outputWorkspaceName() +
                                               "_Parameters");
  m_uiForm->pbPlot->setEnabled(true);
  m_uiForm->pbSave->setEnabled(true);
  m_uiForm->cbPlotType->setEnabled(true);
}

bool IqtFit::validate() {
  UserInputValidator uiv;

  uiv.checkDataSelectorIsValid("Sample Input", m_uiForm->dsSampleInput);

  if (isEmptyModel())
    uiv.addErrorMessage("No fit function has been selected");

  if (inputWorkspace()->getXMin() < 0) {
    uiv.addErrorMessage("Error in input workspace: All X data must be "
                        "greater than or equal to 0.");
  }

  auto error = uiv.generateErrorMessage();
  emit showMessageBox(error);
  return error.isEmpty();
}

void IqtFit::loadSettings(const QSettings &settings) {
  m_uiForm->dsSampleInput->readSettings(settings.group());
}

/**
 * Called when new data has been loaded by the data selector.
 *
 * Configures ranges for spin boxes before raw plot is done.
 *
 * @param wsName Name of new workspace loaded
 */
void IqtFit::newDataLoaded(const QString wsName) {
  IndirectFitAnalysisTab::newInputDataLoaded(wsName);

  const auto maxWsIndex =
      static_cast<int>(inputWorkspace()->getNumberHistograms()) - 1;

  m_uiForm->spPlotSpectrum->setMaximum(maxWsIndex);
  m_uiForm->spPlotSpectrum->setMinimum(0);
  m_uiForm->spPlotSpectrum->setValue(0);

  m_uiForm->spSpectraMin->setMaximum(maxWsIndex);
  m_uiForm->spSpectraMin->setMinimum(0);

  m_uiForm->spSpectraMax->setMaximum(maxWsIndex);
  m_uiForm->spSpectraMax->setMinimum(0);
  m_uiForm->spSpectraMax->setValue(maxWsIndex);
}

void IqtFit::backgroundSelectorChanged(double val) {
  setDefaultPropertyValue("A0", val);
  setParameterValue("LinearBackground", "A0", val);
  setParameterValue("FlatBackground", "A0", val);
}

void IqtFit::parameterUpdated(const Mantid::API::IFunction *function) {
  if (function == nullptr)
    return;

  if (background() && function->asString() == background()->asString()) {
    auto rangeSelector =
        m_uiForm->ppPlotTop->getRangeSelector("IqtFitBackRange");
    MantidQt::API::SignalBlocker<QObject> blocker(rangeSelector);
    rangeSelector->setMinimum(function->getParameter("A0"));
  }
}

void IqtFit::updatePreviewPlots() {
  IndirectFitAnalysisTab::updatePlots(m_uiForm->ppPlotTop,
                                      m_uiForm->ppPlotBottom);
}

void IqtFit::updatePlotRange() {
  auto rangeSelector = m_uiForm->ppPlotTop->getRangeSelector("IqtFitRange");
  if (m_uiForm->ppPlotTop->hasCurve("Sample")) {
    const auto range = m_uiForm->ppPlotTop->getCurveRange("Sample");
    rangeSelector->setRange(range.first, range.second);
  }
}

QHash<QString, double> IqtFit::createDefaultValues() const {
  QHash<QString, double> defaultValues;
  // intensity is always 1-background
  double height = 1.0;
  if (background() != nullptr)
    height -= background()->getParameter("A0");
  defaultValues["Height"] = height;

  auto inputWs = inputWorkspace();
  double tau = 0;

  if (inputWs) {
    auto x = inputWs->x(0);
    auto y = inputWs->y(0);

    if (x.size() > 4) {
      tau = -x[4] / log(y[4]);
    }
  }

  defaultValues["Lifetime"] = tau;
  defaultValues["Stretching"] = 1.0;
  defaultValues["A0"] = 0.0;
  return defaultValues;
}

/**
 * Handles the user entering a new minimum spectrum index.
 *
 * Prevents the user entering an overlapping spectra range.
 *
 * @param value Minimum spectrum index
 */
void IqtFit::specMinChanged(int value) {
  m_uiForm->spSpectraMax->setMinimum(value);
}

/**
 * Handles the user entering a new maximum spectrum index.
 *
 * Prevents the user entering an overlapping spectra range.
 *
 * @param value Maximum spectrum index
 */
void IqtFit::specMaxChanged(int value) {
  m_uiForm->spSpectraMin->setMaximum(value);
}

void IqtFit::startXChanged(double startX) {
  auto rangeSelector = m_uiForm->ppPlotTop->getRangeSelector("IqtFitRange");
  MantidQt::API::SignalBlocker<QObject> blocker(rangeSelector);
  rangeSelector->setMinimum(startX);
}

void IqtFit::endXChanged(double endX) {
  auto rangeSelector = m_uiForm->ppPlotTop->getRangeSelector("IqtFitRange");
  MantidQt::API::SignalBlocker<QObject> blocker(rangeSelector);
  rangeSelector->setMaximum(endX);
}

void IqtFit::singleFit() { executeSingleFit(); }

void IqtFit::disablePlotGuess() { m_uiForm->ckPlotGuess->setEnabled(false); }

void IqtFit::enablePlotGuess() { m_uiForm->ckPlotGuess->setEnabled(true); }

void IqtFit::addGuessPlot(MatrixWorkspace_sptr workspace) {
  m_uiForm->ppPlotTop->addSpectrum("Guess", workspace, 0, Qt::green);
}

void IqtFit::removeGuessPlot() {
  m_uiForm->ppPlotTop->removeSpectrum("Guess");
  m_uiForm->ckPlotGuess->setChecked(false);
}

IAlgorithm_sptr
IqtFit::replaceInfinityAndNaN(MatrixWorkspace_sptr inputWS) const {
  auto replaceAlg = AlgorithmManager::Instance().create("ReplaceSpecialValues");
  replaceAlg->setChild(true);
  replaceAlg->initialize();
  replaceAlg->setProperty("InputWorkspace", inputWS);
  replaceAlg->setProperty("NaNValue", 0.0);
  replaceAlg->setProperty("InfinityError", 0.0);
  replaceAlg->setProperty("OutputWorkspace", inputWS->getName() + "_nospecial");
  return replaceAlg;
}
} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
