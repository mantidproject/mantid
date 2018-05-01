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
    : IndirectFitAnalysisTab(parent), m_uiForm(new Ui::ConvFit),
      m_confitResFileType() {
  m_uiForm->setupUi(parent);
  IndirectFitAnalysisTab::addPropertyBrowserToUI(m_uiForm.get());
}

void ConvFit::setup() {
  // Create Property Managers
  setMinimumSpectrum(0);
  setMaximumSpectrum(0);
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

  addFunctionGroupToComboBox("One Lorentzian", {lorentzian});
  addFunctionGroupToComboBox("Two Lorentzians", {lorentzian, lorentzian});
  addFunctionGroupToComboBox("InelasticDiffSphere", {inelasticDiffSphere});
  addFunctionGroupToComboBox("InelasticDiffRotDiscreteCircle",
                             {inelasticDiffRotDiscCircle});
  addFunctionGroupToComboBox("ElasticDiffSphere", {elasticDiffSphere});
  addFunctionGroupToComboBox("ElasticDiffRotDiscreteCircle",
                             {elasticDiffRotDiscCircle});
  addFunctionGroupToComboBox("StretchedExpFT", {stretchedExpFT});

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
  connect(m_uiForm->dsSampleInput, SIGNAL(dataReady(const QString &)), this,
          SLOT(extendResolutionWorkspace()));
  connect(m_uiForm->dsResInput, SIGNAL(dataReady(const QString &)), this,
          SLOT(extendResolutionWorkspace()));
  connect(m_uiForm->dsResInput, SIGNAL(dataReady(const QString &)), this,
          SLOT(updateGuessPlots()));

  connect(m_uiForm->spSpectraMin, SIGNAL(valueChanged(int)), this,
          SLOT(specMinChanged(int)));
  connect(m_uiForm->spSpectraMax, SIGNAL(valueChanged(int)), this,
          SLOT(specMaxChanged(int)));

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

int ConvFit::minimumSpectrum() const { return m_uiForm->spSpectraMin->value(); }

int ConvFit::maximumSpectrum() const { return m_uiForm->spSpectraMax->value(); }

void ConvFit::runFitAlgorithm(Mantid::API::IAlgorithm_sptr fitAlgorithm) {
  m_uiForm->ckPlotGuess->setChecked(false);
  m_usedTemperature = boolSettingValue("UseTempCorrection");
  m_temperature = doubleSettingValue("TempCorrection");
  IndirectFitAnalysisTab::runFitAlgorithm(fitAlgorithm);
}

bool ConvFit::canPlotGuess() const {
  return m_uiForm->dsResInput->isValid() &&
         IndirectFitAnalysisTab::canPlotGuess();
}

bool ConvFit::doPlotGuess() const {
  return m_uiForm->ckPlotGuess->isEnabled() &&
         m_uiForm->ckPlotGuess->isChecked();
}

void ConvFit::addFunctionGroupToComboBox(
    const QString &groupName, const std::vector<IFunction_sptr> &functions) {
  m_fitTypeToFunction[groupName] = functions[0]->name();
  addComboBoxFunctionGroup(groupName, functions);
}

IFunction_sptr ConvFit::fitFunction() const {
  CompositeFunction_sptr comp(new CompositeFunction);
  auto modelFunction = model();

  if (!(modelFunction &&
        AnalysisDataService::Instance().doesExist("__ConvFit_Resolution")))
    return CompositeFunction_sptr(new CompositeFunction);

  auto backgroundFunction = background();
  if (backgroundFunction)
    comp->addFunction(backgroundFunction);

  auto conv = boost::dynamic_pointer_cast<CompositeFunction>(
      FunctionFactory::Instance().createFunction("Convolution"));
  conv->addFunction(createResolutionFunction());

  auto compositeModel =
      boost::dynamic_pointer_cast<CompositeFunction>(modelFunction);

  if (boolSettingValue("UseTempCorrection")) {
    if (compositeModel)
      modelFunction = addTemperatureCorrection(compositeModel);
    else
      modelFunction = addTemperatureCorrection(modelFunction);
  }

  conv->addFunction(modelFunction);

  if (backgroundFunction)
    comp->addFunction(conv);
  else
    comp = conv;
  comp->applyTies();
  return comp;
}

QHash<QString, QString>
ConvFit::functionNameChanges(IFunction_sptr model) const {
  QHash<QString, QString> changes;
  auto compositeModel = boost::dynamic_pointer_cast<CompositeFunction>(model);

  QString prefixPrefix = "f1.";
  const auto backIndex = backgroundIndex();

  if (backIndex)
    prefixPrefix += "f1.";

  QString prefixSuffix = "";

  if (boolSettingValue("UseTempCorrection"))
    prefixSuffix = "f1.";

  if (compositeModel) {
    const auto &offset = backIndex ? 1 : 0;
    const auto &index = backIndex.get_value_or(0);
    addFunctionNameChanges(compositeModel, prefixPrefix, prefixSuffix, 0, index,
                           0, changes);
    addFunctionNameChanges(compositeModel, prefixPrefix, prefixSuffix, index,
                           static_cast<int>(compositeModel->nFunctions()),
                           offset, changes);
  } else
    addFunctionNameChanges(model, "", prefixPrefix + prefixSuffix, changes);
  return changes;
}

void ConvFit::addFunctionNameChanges(
    IFunction_sptr model, const QString &modelPrefix, const QString &newPrefix,
    QHash<QString, QString> &functionNameChanges) const {

  for (const auto &parameter : model->getParameterNames()) {
    auto parameterName = QString::fromStdString(parameter);
    functionNameChanges[modelPrefix + parameterName] =
        newPrefix + parameterName;
  }
}

void ConvFit::addFunctionNameChanges(
    CompositeFunction_sptr model, const QString &prefixPrefix,
    const QString &prefixSuffix, size_t fromIndex, size_t toIndex, int offset,
    QHash<QString, QString> &functionNameChanges) const {

  for (auto i = fromIndex; i < toIndex; ++i) {
    const auto &functionPrefix = "f" + QString::number(i) + ".";
    const auto &offsetPrefix = "f" + QString::number(i + offset) + ".";
    const auto &function = model->getFunction(static_cast<size_t>(i));
    QString prefix = prefixPrefix + functionPrefix;

    if (function->name() != "DeltaFunction")
      prefix += prefixSuffix;

    addFunctionNameChanges(function, offsetPrefix, prefix, functionNameChanges);
  }
}

CompositeFunction_sptr
ConvFit::addTemperatureCorrection(IFunction_sptr model) const {
  return applyTemperatureCorrection(model, createTemperatureCorrection());
}

CompositeFunction_sptr
ConvFit::addTemperatureCorrection(CompositeFunction_sptr model) const {
  auto correction = createTemperatureCorrection();

  for (size_t i = 0u; i < model->nFunctions(); ++i) {
    auto function = model->getFunction(i);

    if (function->name() != "DeltaFunction") {
      auto corrected = applyTemperatureCorrection(function, correction);
      model->replaceFunction(i, corrected);
    }
  }
  return model;
}

CompositeFunction_sptr
ConvFit::applyTemperatureCorrection(IFunction_sptr function,
                                    IFunction_sptr correction) const {
  auto product = boost::dynamic_pointer_cast<CompositeFunction>(
      FunctionFactory::Instance().createFunction("ProductFunction"));
  product->addFunction(correction);
  product->addFunction(function);
  product->tie("f0.Temp", std::to_string(doubleSettingValue("TempCorrection")));
  product->applyTies();
  return product;
}

std::string ConvFit::createSingleFitOutputName() const {
  return constructBaseName() + std::to_string(selectedSpectrum());
}

std::string ConvFit::createSequentialFitOutputName() const {
  const auto specMin = std::to_string(minimumSpectrum());
  const auto specMax = std::to_string(maximumSpectrum());
  return constructBaseName() + specMin + "_to_" + specMax;
}

std::string ConvFit::constructBaseName() const {
  auto outputName = inputWorkspace()->getName();

  // Remove _red
  const auto cutIndex = outputName.find_last_of('_');
  if (cutIndex != std::string::npos)
    outputName = outputName.substr(0, cutIndex);

  const auto background = backgroundString(backgroundType()).toStdString();
  return outputName + "_conv_" + fitTypeString() + background;
}

IAlgorithm_sptr ConvFit::singleFitAlgorithm() const {
  const auto spectrum = static_cast<int>(selectedSpectrum());
  return sequentialFit(spectrum, spectrum);
}

IAlgorithm_sptr ConvFit::sequentialFitAlgorithm() const {
  const auto specMin = static_cast<int>(minimumSpectrum());
  const auto specMax = static_cast<int>(maximumSpectrum());
  return sequentialFit(specMin, specMax);
}

IAlgorithm_sptr ConvFit::sequentialFit(const int &specMin,
                                       const int &specMax) const {
  const auto outputResultName =
      outputWorkspaceName(boost::numeric_cast<size_t>(specMin)) + "_Result";

  auto cfs = AlgorithmManager::Instance().create("ConvolutionFitSequential");
  cfs->initialize();
  cfs->setProperty("PassWSIndexToFunction", true);
  cfs->setProperty("BackgroundType", backgroundType().toStdString());
  cfs->setProperty("SpecMin", specMin);
  cfs->setProperty("SpecMax", specMax);
  cfs->setProperty("ExtractMembers", boolSettingValue("ExtractMembers"));
  cfs->setProperty("OutputWorkspace", outputResultName);
  return cfs;
}

QString ConvFit::backgroundType() const {
  const auto backgroundString = backgroundName();

  if (backgroundString == "None") {
    return "Fixed Flat";
  } else if (backgroundString == "LinearBackground") {
    return "Fit Linear";
  } else if (backgroundString == "FlatBackground") {
    if (background()->getTie(0) == nullptr) {
      return "Fit Flat";
    } else {
      return "Fixed Flat";
    }
  }
  return "";
}

/**
 * Handles saving the workspace when save is clicked
 */
void ConvFit::saveClicked() {
  IndirectFitAnalysisTab::saveResult(outputWorkspaceName() + "_Result");
}

/**
 * Handles plotting the workspace when plot is clicked
 */
void ConvFit::plotClicked() {
  IndirectFitAnalysisTab::plotResult(outputWorkspaceName() + "_Result",
                                     m_uiForm->cbPlotType->currentText());
}

void ConvFit::updatePlotOptions() {
  IndirectFitAnalysisTab::updatePlotOptions(m_uiForm->cbPlotType);
}

/**
 * Handles completion of the ConvolutionFitSequential algorithm.
 *
 * @param error True if the algorithm was stopped due to error, false otherwise
 */
void ConvFit::algorithmComplete(bool error) {

  if (error) {
    return;
  }

  const auto outputPrefix = outputWorkspaceName();
  const auto resultName = outputPrefix + "_Result";
  // Name for GroupWorkspace
  const auto groupName = outputPrefix + "_Workspaces";
  // Add Sample logs for ResolutionFiles
  const auto resFile = m_uiForm->dsResInput->getCurrentDataName().toStdString();
  addSampleLogsToWorkspace(resultName, "resolution_filename", resFile,
                           "String");
  addSampleLogsToWorkspace(groupName, "resolution_filename", resFile, "String");

  // Check if temperature is used and is valid
  if (m_usedTemperature) {
    const auto temperature = QString::number(m_temperature);

    if (m_temperature != 0.0) {
      // Add sample logs for temperature
      const auto temperatureStr = temperature.toStdString();
      addSampleLogsToWorkspace(resultName, "temperature_correction", "true",
                               "String");
      addSampleLogsToWorkspace(groupName, "temperature_correction", "true",
                               "String");
      addSampleLogsToWorkspace(resultName, "temperature_value", temperatureStr,
                               "Number");
      addSampleLogsToWorkspace(resultName, "temperature_value", temperatureStr,
                               "Number");
    }
  }
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(sampleLogsAdded()));
  m_batchAlgoRunner->executeBatchAsync();
}

void ConvFit::sampleLogsAdded() {
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
             SLOT(sampleLogsAdded()));
  IndirectFitAnalysisTab::fitAlgorithmComplete(outputWorkspaceName() +
                                               "_Parameters");
}

/**
 * Sets up and add an instance of the AddSampleLog algorithm to the batch
 * algorithm runner
 * @param workspaceName	:: The name of the workspace to add the log to
 * @param logName		:: The title of the log input
 * @param logText		:: The information to match the title
 * @param logType		:: The type of information (String, Number)
 */
void ConvFit::addSampleLogsToWorkspace(const std::string &workspaceName,
                                       const std::string &logName,
                                       const std::string &logText,
                                       const std::string &logType) {

  auto addSampleLog = AlgorithmManager::Instance().create("AddSampleLog");
  addSampleLog->setLogging(false);
  addSampleLog->setProperty("Workspace", workspaceName);
  addSampleLog->setProperty("LogName", logName);
  addSampleLog->setProperty("LogText", logText);
  addSampleLog->setProperty("LogType", logType);
  m_batchAlgoRunner->addAlgorithm(addSampleLog);
}

/**
 * Validates the user's inputs in the ConvFit tab.
 * @return If the validation was successful
 */
bool ConvFit::validate() {
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

  const int maxWsIndex =
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

  // ConvolutionFitSequential requires the last functions to be the ones
  // provided as custom groups in the interface.
  if (model())
    moveCustomFunctionsToEnd();
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
 * Create a resolution workspace with the same number of histograms as in the
 * sample, if the resolution and sample differ in their number of histograms.
 *
 * Needed to allow DiffSphere and DiffRotDiscreteCircle fit functions to work as
 * they need to have the WorkspaceIndex attribute set.
 */
void ConvFit::extendResolutionWorkspace() {
  const auto inputWs = inputWorkspace();
  const auto resName = "__ConvFit_Resolution";

  if (inputWs && m_uiForm->dsResInput->isValid()) {
    const std::string resWsName =
        m_uiForm->dsResInput->getCurrentDataName().toStdString();
    // Check spectra consistency between resolution and sample
    auto resolutionInputWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(resWsName);
    size_t resolutionNumHist = resolutionInputWS->getNumberHistograms();
    size_t numHist = inputWs->getNumberHistograms();
    if (resolutionNumHist != 1 && resolutionNumHist != numHist) {
      std::string msg(
          "Resolution must have either one or as many spectra as the sample");
      throw std::runtime_error(msg);
    }
    // Clone resolution workspace
    cloneAlgorithm(resolutionInputWS, resName)->execute();
    auto resolutionWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(resName);

    // Append to cloned workspace if necessary
    if (resolutionNumHist == 1 && numHist > 1) {
      auto appendAlg = appendAlgorithm(resolutionWS, resolutionInputWS,
                                       static_cast<int>(numHist - 1), resName);
      appendAlg->execute();
    }
  }
}

/**
 * Creates an algorithm for cloning a workspace.
 *
 * @param inputWS       The workspace to clone.
 * @param outputWSName  The name to assign to the output workspace.
 * @return              The created algorithm.
 */
IAlgorithm_sptr ConvFit::cloneAlgorithm(MatrixWorkspace_sptr inputWS,
                                        const std::string &outputWSName) const {
  IAlgorithm_sptr cloneAlg =
      AlgorithmManager::Instance().create("CloneWorkspace");
  cloneAlg->setLogging(false);
  cloneAlg->initialize();
  cloneAlg->setProperty("InputWorkspace", inputWS);
  cloneAlg->setProperty("OutputWorkspace", outputWSName);
  return cloneAlg;
}

/**
 * Creates an algorithm for appending the spectrum of one workspace,
 * to another.
 *
 * @param leftWS        The workspace to append spectra to.
 * @param rightWS       The workspace whose spectra to append.
 * @param numHistograms The number of spectra to append.
 * @param outputWSName  The name to assign to the output workspace.
 * @return              The created algorithm.
 */
IAlgorithm_sptr
ConvFit::appendAlgorithm(MatrixWorkspace_sptr leftWS,
                         MatrixWorkspace_sptr rightWS, int numHistograms,
                         const std::string &outputWSName) const {
  IAlgorithm_sptr appendAlg =
      AlgorithmManager::Instance().create("AppendSpectra");
  appendAlg->setLogging(false);
  appendAlg->initialize();
  appendAlg->setProperty("InputWorkspace1", leftWS);
  appendAlg->setProperty("InputWorkspace2", rightWS);
  appendAlg->setProperty("Number", numHistograms);
  appendAlg->setProperty("OutputWorkspace", outputWSName);
  return appendAlg;
}

/**
 * @return  The correction function for the temperature.
 */
IFunction_sptr ConvFit::createTemperatureCorrection() const {
  // create temperature correction function to multiply with the lorentzians
  IFunction_sptr tempFunc;
  QString temperature = QString::number(doubleSettingValue("TempCorrection"));

  // create user function for the exponential correction
  // (x*temp) / 1-exp(-(x*temp))
  tempFunc = FunctionFactory::Instance().createFunction("UserFunction");
  // 11.606 is the conversion factor from meV to K
  std::string formula = "((x*11.606)/Temp) / (1 - exp(-((x*11.606)/Temp)))";
  IFunction::Attribute att(formula);
  tempFunc->setAttribute("Formula", att);
  tempFunc->setParameter("Temp", temperature.toDouble());
  return tempFunc;
}

IFunction_sptr ConvFit::createResolutionFunction() const {
  auto func = FunctionFactory::Instance().createFunction("Resolution");
  // add resolution file
  IFunction::Attribute attr("__ConvFit_Resolution");
  func->setAttribute("Workspace", attr);
  return func;
}

/**
 * Obtains the instrument resolution from the provided workspace
 * @param workspaceName The workspaces which holds the instrument
 *                      resolution
 * @return The resolution of the instrument. returns 0 if no resolution data
 * could be found
 */
double ConvFit::getInstrumentResolution(MatrixWorkspace_sptr workspace) const {
  using namespace Mantid::API;

  double resolution = 0.0;
  try {
    Mantid::Geometry::Instrument_const_sptr inst = workspace->getInstrument();
    std::vector<std::string> analysers = inst->getStringParameter("analyser");
    if (analysers.empty()) {
      g_log.warning("Could not load instrument resolution from parameter file");
      return 0.0;
    }

    std::string analyser = analysers[0];
    std::string idfDirectory =
        Mantid::Kernel::ConfigService::Instance().getString(
            "instrumentDefinition.directory");

    // If the analyser component is not already in the data file then load it
    // from the parameter file
    if (inst->getComponentByName(analyser) == nullptr ||
        inst->getComponentByName(analyser)
                ->getNumberParameter("resolution")
                .size() == 0) {
      std::string reflection = inst->getStringParameter("reflection")[0];
      std::string paramFile = idfDirectory + inst->getName() + "_" + analyser +
                              "_" + reflection + "_Parameters.xml";

      IAlgorithm_sptr loadParamFile =
          loadParameterFileAlgorithm(workspace, paramFile);
      loadParamFile->execute();

      if (!loadParamFile->isExecuted()) {
        g_log.warning("Could not load parameter file, ensure instrument "
                      "directory is in data search paths.");
        return 0.0;
      }

      inst = workspace->getInstrument();
    }
    if (inst->getComponentByName(analyser) != nullptr) {
      resolution = inst->getComponentByName(analyser)
                       ->getNumberParameter("resolution")[0];
    } else {
      resolution = inst->getNumberParameter("resolution")[0];
    }
  } catch (Mantid::Kernel::Exception::NotFoundError &e) {
    UNUSED_ARG(e);

    g_log.warning("Could not load instrument resolution from parameter file");
    resolution = 0.0;
  }

  return resolution;
}

IAlgorithm_sptr
ConvFit::loadParameterFileAlgorithm(MatrixWorkspace_sptr workspace,
                                    const std::string &filename) const {
  IAlgorithm_sptr loadParamFile =
      AlgorithmManager::Instance().create("LoadParameterFile");
  loadParamFile->setChild(true);
  loadParamFile->initialize();
  loadParamFile->setProperty("Workspace", workspace);
  loadParamFile->setProperty("Filename", filename);
  return loadParamFile;
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
 * Generate a string to describe the background selected by the user.
 * Used when naming the resultant workspaces.
 *
 * @param backgroundType  The background type used in the fit.
 * @returns               The generated QString.
 */
QString ConvFit::backgroundString(const QString &backgroundType) const {

  if (backgroundType == "Fixed Flat")
    return "FixF_s";
  else if (backgroundType == "Fit Flat")
    return "FitF_s";
  else if (backgroundType == "Fit Linear")
    return "FitL_s";
  else
    return "";
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

void ConvFit::enablePlotResult() { m_uiForm->pbPlot->setEnabled(true); }

void ConvFit::disablePlotResult() { m_uiForm->pbPlot->setEnabled(false); }

void ConvFit::enableSaveResult() { m_uiForm->pbSave->setEnabled(true); }

void ConvFit::disableSaveResult() { m_uiForm->pbSave->setEnabled(false); }

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

/**
 * Runs the single fit algorithm
 */
void ConvFit::singleFit() { executeSingleFit(); }

/**
 * Handles the user entering a new minimum spectrum index.
 *
 * Prevents the user entering an overlapping spectra range.
 *
 * @param value Minimum spectrum index
 */
void ConvFit::specMinChanged(int value) {
  m_uiForm->spSpectraMax->setMinimum(value);
}

/**
 * Handles the user entering a new maximum spectrum index.
 *
 * Prevents the user entering an overlapping spectra range.
 *
 * @param value Maximum spectrum index
 */
void ConvFit::specMaxChanged(int value) {
  m_uiForm->spSpectraMin->setMaximum(value);
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
  setDefaultPropertyValue("FWHM", fwhm);
  setParameterValue("Lorentzian", "FWHM", fwhm);
}

void ConvFit::backgLevel(double val) {
  setDefaultPropertyValue("A0", val);
  setParameterValue("LinearBackground", "A0", val);
  setParameterValue("FlatBackground", "A0", val);
}

void ConvFit::updateHWHMFromResolution() {
  auto resolution = getInstrumentResolution(inputWorkspace());

  if (resolution > 0 && selectedFitType().contains("Lorentzian"))
    hwhmMaxChanged(resolution / 2.0);
}

/**
 * Populates the default parameter map with the initial default values
 */
QHash<QString, double> ConvFit::createDefaultValues() const {
  QHash<QString, double> defaultValues;
  defaultValues["PeakCentre"] = 0.0;
  defaultValues["Centre"] = 0.0;
  // Reset all parameters to default of 1
  defaultValues["Amplitude"] = 1.0;
  defaultValues["beta"] = 1.0;
  defaultValues["Decay"] = 1.0;
  defaultValues["Diffusion"] = 1.0;
  defaultValues["height"] = 1.0; // Lower case in StretchedExp - this can be
                                 // improved with a case insensitive check
  defaultValues["Height"] = 1.0;
  defaultValues["Intensity"] = 1.0;
  defaultValues["Radius"] = 1.0;
  defaultValues["tau"] = 1.0;

  defaultValues["FWHM"] = getInstrumentResolution(inputWorkspace());
  return defaultValues;
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
