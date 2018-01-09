#include "ConvFit.h"

#include "../General/UserInputValidator.h"

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
    : IndirectFitAnalysisTab(parent), m_confitResFileType() {
  m_uiForm.setupUi(parent);
  m_cfTree = m_propertyTree;
}

void ConvFit::setup() {
  // Create Property Managers
  setMinimumSpectrum(0);
  setMaximumSpectrum(0);

  // Initialise fitTypeStrings
  m_fitStrings = {"", "1L", "2L", "IDS", "IDC", "EDS", "EDC", "SFT"};
  createDefaultParamsMap();

  // Initialize TreeProperty Widget
  m_uiForm.properties->addWidget(m_cfTree);

  // add factories to managers
  m_cfTree->setFactoryForManager(m_blnManager, m_blnEdFac);
  m_cfTree->setFactoryForManager(m_dblManager, m_dblEdFac);

  // Create Range Selectors
  auto fitRangeSelector = m_uiForm.ppPlotTop->addRangeSelector("ConvFitRange");
  auto backRangeSelector = m_uiForm.ppPlotTop->addRangeSelector(
      "ConvFitBackRange", MantidWidgets::RangeSelector::YSINGLE);
  auto hwhmRangeSelector = m_uiForm.ppPlotTop->addRangeSelector("ConvFitHWHM");
  backRangeSelector->setColour(Qt::darkGreen);
  backRangeSelector->setRange(0.0, 1.0);
  hwhmRangeSelector->setColour(Qt::red);

  // Populate Property Widget

  // Option to convolve members
  m_properties["Convolve"] = m_blnManager->addProperty("Convolve");
  m_cfTree->addProperty(m_properties["Convolve"]);
  m_blnManager->setValue(m_properties["Convolve"], true);

  // Option to extract members
  m_properties["ExtractMembers"] = m_blnManager->addProperty("ExtractMembers");
  m_cfTree->addProperty(m_properties["ExtractMembers"]);
  m_blnManager->setValue(m_properties["ExtractMembers"], false);

  // Max iterations option
  m_properties["MaxIterations"] = m_dblManager->addProperty("Max Iterations");
  m_dblManager->setDecimals(m_properties["MaxIterations"], 0);
  m_dblManager->setValue(m_properties["MaxIterations"], 500);
  m_cfTree->addProperty(m_properties["MaxIterations"]);

  // Fitting range
  m_properties["FitRange"] = m_grpManager->addProperty("Fitting Range");
  m_properties["StartX"] = m_dblManager->addProperty("StartX");
  m_dblManager->setDecimals(m_properties["StartX"], NUM_DECIMALS);
  m_properties["EndX"] = m_dblManager->addProperty("EndX");
  m_dblManager->setDecimals(m_properties["EndX"], NUM_DECIMALS);
  m_properties["FitRange"]->addSubProperty(m_properties["StartX"]);
  m_properties["FitRange"]->addSubProperty(m_properties["EndX"]);
  m_cfTree->addProperty(m_properties["FitRange"]);

  // FABADA
  initFABADAOptions();

  // Background type
  m_properties["Background"] = createFunctionProperty("Background");
  m_cfTree->addProperty(m_properties["Background"]);

  // Delta Function
  m_properties["Delta Function"] =
      createFunctionProperty("Delta Function", false);
  m_properties["UseDeltaFunc"] = m_blnManager->addProperty("Use");
  m_properties["Delta Function"]->addSubProperty(m_properties["UseDeltaFunc"]);
  m_cfTree->addProperty(m_properties["Delta Function"]);

  // Fit functions
  m_properties["Lorentzian 1"] = createFunctionProperty("Lorentzian 1");
  m_properties["Lorentzian 2"] = createFunctionProperty("Lorentzian 2");
  m_properties["DiffSphere"] = createFunctionProperty("DiffSphere");
  m_properties["DiffRotDiscreteCircle"] =
      createFunctionProperty("DiffRotDiscreteCircle");
  m_properties["ElasticDiffSphere"] =
      createFunctionProperty("ElasticDiffSphere");
  m_properties["ElasticDiffRotDiscreteCircle"] =
      createFunctionProperty("ElasticDiffRotDiscreteCircle");
  m_properties["InelasticDiffSphere"] =
      createFunctionProperty("InelasticDiffSphere");
  m_properties["InelasticDiffRotDiscreteCircle"] =
      createFunctionProperty("InelasticDiffRotDiscreteCircle");
  m_properties["StretchedExpFT"] = createFunctionProperty("StretchedExpFT");

  // Instrument resolution
  m_properties["InstrumentResolution"] =
      m_dblManager->addProperty("InstrumentResolution");

  // Update fit parameters in browser when function is selected
  connect(m_uiForm.cbFitType, SIGNAL(currentIndexChanged(int)), this,
          SLOT(fitFunctionSelected(int)));
  fitFunctionSelected(m_uiForm.cbFitType->currentIndex());

  m_uiForm.leTempCorrection->setValidator(new QDoubleValidator(m_parentWidget));

  // Connections
  connect(fitRangeSelector, SIGNAL(minValueChanged(double)), this,
          SLOT(minChanged(double)));
  connect(fitRangeSelector, SIGNAL(maxValueChanged(double)), this,
          SLOT(maxChanged(double)));
  connect(backRangeSelector, SIGNAL(minValueChanged(double)), this,
          SLOT(backgLevel(double)));
  connect(hwhmRangeSelector, SIGNAL(minValueChanged(double)), this,
          SLOT(hwhmChanged(double)));
  connect(hwhmRangeSelector, SIGNAL(maxValueChanged(double)), this,
          SLOT(hwhmChanged(double)));
  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(updateRS(QtProperty *, double)));
  connect(m_blnManager, SIGNAL(valueChanged(QtProperty *, bool)), this,
          SLOT(checkBoxUpdate(QtProperty *, bool)));
  connect(m_uiForm.ckTempCorrection, SIGNAL(toggled(bool)),
          m_uiForm.leTempCorrection, SLOT(setEnabled(bool)));

  enablePlotGuess();

  // Have FWHM Range linked to Fit Start/End Range
  connect(fitRangeSelector, SIGNAL(rangeChanged(double, double)),
          hwhmRangeSelector, SLOT(setRange(double, double)));
  hwhmRangeSelector->setRange(-1.0, 1.0);
  hwhmUpdateRS(0.02);

  typeSelection(m_uiForm.cbFitType->currentIndex());
  bgTypeSelection(m_uiForm.cbBackground->currentIndex());

  // Replot input automatically when file / spec no changes
  connect(m_uiForm.spPlotSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(setSelectedSpectrum(int)));
  connect(m_uiForm.spPlotSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(updatePreviewPlots()));
  connect(m_uiForm.spPlotSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(updateProperties(int)));

  // Update plot preview when fit function changes
  connect(m_uiForm.cbFitType, SIGNAL(currentIndexChanged(int)), this,
          SLOT(updatePreviewPlots()));

  connect(m_uiForm.dsSampleInput, SIGNAL(dataReady(const QString &)), this,
          SLOT(newDataLoaded(const QString &)));
  connect(m_uiForm.dsSampleInput, SIGNAL(dataReady(const QString &)), this,
          SLOT(extendResolutionWorkspace()));
  connect(m_uiForm.dsResInput, SIGNAL(dataReady(const QString &)), this,
          SLOT(extendResolutionWorkspace()));

  connect(m_uiForm.spSpectraMin, SIGNAL(valueChanged(int)), this,
          SLOT(specMinChanged(int)));
  connect(m_uiForm.spSpectraMax, SIGNAL(valueChanged(int)), this,
          SLOT(specMaxChanged(int)));

  connect(m_uiForm.cbFitType, SIGNAL(currentIndexChanged(int)), this,
          SLOT(typeSelection(int)));
  connect(m_uiForm.cbBackground, SIGNAL(currentIndexChanged(int)), this,
          SLOT(bgTypeSelection(int)));
  connect(m_uiForm.pbSingleFit, SIGNAL(clicked()), this, SLOT(singleFit()));

  // Context menu
  m_cfTree->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_cfTree, SIGNAL(customContextMenuRequested(const QPoint &)), this,
          SLOT(fitContextMenu(const QPoint &)));

  // Tie
  connect(m_uiForm.cbFitType, SIGNAL(currentIndexChanged(QString)),
          SLOT(showTieCheckbox(QString)));
  showTieCheckbox(m_uiForm.cbFitType->currentText());

  // Post Plot and Save
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));
  connect(m_uiForm.pbPlot, SIGNAL(clicked()), this, SLOT(plotClicked()));
  connect(m_uiForm.pbPlotPreview, SIGNAL(clicked()), this,
          SLOT(plotCurrentPreview()));

  m_uiForm.ckTieCentres->setChecked(true);
}

/** Setup FABADA minimizer options
 *
 */
void ConvFit::initFABADAOptions() {

  m_properties["FABADA"] = m_grpManager->addProperty("Bayesian");
  m_properties["UseFABADA"] = m_blnManager->addProperty("Use FABADA");
  m_properties["FABADA"]->addSubProperty(m_properties["UseFABADA"]);

  // Output chain
  m_properties["OutputFABADAChain"] = m_blnManager->addProperty("Output Chain");
  // Chain length
  m_properties["FABADAChainLength"] = m_dblManager->addProperty("Chain Length");
  m_dblManager->setDecimals(m_properties["FABADAChainLength"], 0);
  m_dblManager->setValue(m_properties["FABADAChainLength"], 1000000);
  // Convergence criteria
  m_properties["FABADAConvergenceCriteria"] =
      m_dblManager->addProperty("Convergence Criteria");
  m_dblManager->setValue(m_properties["FABADAConvergenceCriteria"], 0.1);
  // Jump acceptance rate
  m_properties["FABADAJumpAcceptanceRate"] =
      m_dblManager->addProperty("Acceptance Rate");
  m_dblManager->setValue(m_properties["FABADAJumpAcceptanceRate"], 0.25);

  // Advanced options
  m_properties["FABADAAdvanced"] = m_blnManager->addProperty("Advanced");
  m_blnManager->setValue(m_properties["FABADAAdvanced"], false);
  // Steps between values
  m_properties["FABADAStepsBetweenValues"] =
      m_dblManager->addProperty("Steps Between Values");
  m_dblManager->setDecimals(m_properties["FABADAStepsBetweenValues"], 0);
  m_dblManager->setValue(m_properties["FABADAStepsBetweenValues"], 10);
  // Inactive convergence criterion
  m_properties["FABADAInactiveConvergenceCriterion"] =
      m_dblManager->addProperty("Inactive Convergence Criterion");
  m_dblManager->setDecimals(m_properties["FABADAInactiveConvergenceCriterion"],
                            0);
  m_dblManager->setValue(m_properties["FABADAInactiveConvergenceCriterion"], 5);
  // Simulated annealing applied
  m_properties["FABADASimAnnealingApplied"] =
      m_blnManager->addProperty("Sim Annealing Applied");
  // Maximum temperature
  m_properties["FABADAMaximumTemperature"] =
      m_dblManager->addProperty("Maximum Temperature");
  m_dblManager->setValue(m_properties["FABADAMaximumTemperature"], 10.0);
  // Number of regrigeration steps
  m_properties["FABADANumRefrigerationSteps"] =
      m_dblManager->addProperty("Num Refrigeration Steps");
  m_dblManager->setDecimals(m_properties["FABADANumRefrigerationSteps"], 0);
  m_dblManager->setValue(m_properties["FABADANumRefrigerationSteps"], 5);
  // Simulated annealing iterations
  m_properties["FABADASimAnnealingIterations"] =
      m_dblManager->addProperty("Sim Annealing Iterations");
  m_dblManager->setDecimals(m_properties["FABADASimAnnealingIterations"], 0);
  m_dblManager->setValue(m_properties["FABADASimAnnealingIterations"], 10000);
  // Overexploration
  m_properties["FABADAOverexploration"] =
      m_blnManager->addProperty("Overexploration");
  m_cfTree->addProperty(m_properties["FABADA"]);
  // Number of bins in PDF
  m_properties["FABADANumberBinsPDF"] =
      m_dblManager->addProperty("Number Bins PDF");
  m_dblManager->setDecimals(m_properties["FABADANumberBinsPDF"], 0);
  m_dblManager->setValue(m_properties["FABADANumberBinsPDF"], 20);
}

/**
 * Handles the initial set up and running of the ConvolutionFitSequential
 * algorithm
 */
void ConvFit::run() {
  // Get input from interface
  setMinimumSpectrum(m_uiForm.spSpectraMin->value());
  setMaximumSpectrum(m_uiForm.spSpectraMax->value());
  const auto specMin = m_uiForm.spSpectraMin->text().toStdString();
  const auto specMax = m_uiForm.spSpectraMax->text().toStdString();
  setFitFunctions(indexToFitFunctions(m_uiForm.cbFitType->currentIndex()));
  runFitAlgorithm(sequentialFit(specMin, specMax, m_baseName));
}

IAlgorithm_sptr ConvFit::sequentialFit(const std::string &specMin,
                                       const std::string &specMax,
                                       QString &outputWSName) {
  const auto func = createFunction(m_uiForm.ckTieCentres->isChecked() &&
                                   m_uiForm.ckTieCentres->isVisible());
  const auto function = std::string(func->asString());

  // Construct expected name
  outputWSName = QString::fromStdString(inputWorkspace()->getName());

  // Remove _red
  const auto cutIndex = outputWSName.lastIndexOf("_");
  if (cutIndex != -1) {
    outputWSName = outputWSName.left(cutIndex + 1);
  }

  // Add fit specific suffix
  const auto bgType = backgroundString();
  const auto fitType = fitTypeString();
  outputWSName += "conv_";
  outputWSName += fitType;
  outputWSName += bgType;
  outputWSName += QString::fromStdString(specMin);

  if (specMin != specMax) {
    outputWSName += "_to_";
    outputWSName += QString::fromStdString(specMax);
  }

  // Run ConvolutionFitSequential Algorithm
  auto cfs = AlgorithmManager::Instance().create("ConvolutionFitSequential");
  cfs->initialize();
  cfs->setProperty("InputWorkspace", inputWorkspace());
  cfs->setProperty("Function", function);
  cfs->setProperty("PassWSIndexToFunction", true);
  cfs->setProperty("BackgroundType",
                   m_uiForm.cbBackground->currentText().toStdString());
  cfs->setProperty("StartX", m_properties["StartX"]->valueText().toStdString());
  cfs->setProperty("EndX", m_properties["EndX"]->valueText().toStdString());
  cfs->setProperty("SpecMin", specMin);
  cfs->setProperty("SpecMax", specMax);
  cfs->setProperty("Convolve", true);
  cfs->setProperty("ExtractMembers", static_cast<bool>(m_blnManager->value(
                                         m_properties["ExtractMembers"])));
  cfs->setProperty("Minimizer",
                   minimizerString("$outputname_$wsindex").toStdString());
  cfs->setProperty("MaxIterations", static_cast<int>(m_dblManager->value(
                                        m_properties["MaxIterations"])));
  cfs->setProperty("OutputWorkspace", (outputWSName.toStdString() + "_Result"));
  return cfs;
}

/**
 * Handles saving the workspace when save is clicked
 */
void ConvFit::saveClicked() {
  IndirectFitAnalysisTab::saveResult(m_baseName.toStdString() + "_Result");
}

/**
 * Handles plotting the workspace when plot is clicked
 */
void ConvFit::plotClicked() {
  IndirectFitAnalysisTab::plotResult(m_baseName.toStdString() + "_Result",
                                     m_uiForm.cbPlotType->currentText());
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

  std::string outputPrefix = m_baseName.toStdString();

  const auto resultName = outputPrefix + "_Result";
  MatrixWorkspace_sptr resultWs =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(resultName);

  // Name for GroupWorkspace
  const auto groupName = outputPrefix + "_Workspaces";
  // Add Sample logs for ResolutionFiles
  const auto resFile = m_uiForm.dsResInput->getCurrentDataName().toStdString();
  addSampleLogsToWorkspace(resultName, "resolution_filename", resFile,
                           "String");
  addSampleLogsToWorkspace(groupName, "resolution_filename", resFile, "String");
  m_usedTemperature = false;

  // Check if temperature is used and is valid
  if (m_uiForm.ckTempCorrection->isChecked()) {
    const QString temperature = m_uiForm.leTempCorrection->text();
    double temp = 0.0;

    if (!temperature.isEmpty()) {
      temp = temperature.toDouble();
    }

    if (temp != 0.0) {
      m_usedTemperature = true;
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
  m_batchAlgoRunner->executeBatchAsync();

  const std::string paramWsName = outputPrefix + "_Parameters";
  QHash<QString, QString> backgroundMap;
  backgroundMap["Background.A0"] = "f0.A0";
  backgroundMap["Background.A1"] = "f0.A1";
  IndirectFitAnalysisTab::fitAlgorithmComplete(paramWsName, backgroundMap);

  m_uiForm.pbSave->setEnabled(true);
  m_uiForm.pbPlot->setEnabled(true);
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

  const QString fitType = fitTypeString();
  if (fitType == "") {
    uiv.addErrorMessage("No fit type defined");
  }

  uiv.checkDataSelectorIsValid("Sample", m_uiForm.dsSampleInput);
  uiv.checkDataSelectorIsValid("Resolution", m_uiForm.dsResInput);

  const auto range = std::make_pair(m_dblManager->value(m_properties["StartX"]),
                                    m_dblManager->value(m_properties["EndX"]));
  uiv.checkValidRange("Fitting Range", range);

  // Enforce the rule that at least one fit is needed; either a delta function,
  // one or two Lorentzian functions,
  // or both.  (The resolution function must be convolved with a model.)
  if (m_uiForm.cbFitType->currentIndex() == 0 &&
      !m_blnManager->value(m_properties["UseDeltaFunc"]))
    uiv.addErrorMessage("No fit function has been selected.");

  if (m_uiForm.ckTempCorrection->isChecked()) {
    if (m_uiForm.leTempCorrection->text().compare("") == 0) {
      uiv.addErrorMessage("Temperature correction has been checked in the "
                          "interface, but no value has been given.");
    }
  }

  const auto error = uiv.generateErrorMessage();
  showMessageBox(error);

  return error.isEmpty();
}

/**
 * Reads in settings files
 * @param settings The name of the QSettings object to retrieve data from
 */
void ConvFit::loadSettings(const QSettings &settings) {
  m_uiForm.dsSampleInput->readSettings(settings.group());
  m_uiForm.dsResInput->readSettings(settings.group());
}

/**
 * Called when new data has been loaded by the data selector.
 *
 * Configures ranges for spin boxes before raw plot is done.
 *
 * @param wsName Name of new workspace loaded
 */
void ConvFit::newDataLoaded(const QString &wsName) {
  m_baseName.clear();
  IndirectFitAnalysisTab::newInputDataLoaded(wsName);

  const int maxWsIndex =
      static_cast<int>(inputWorkspace()->getNumberHistograms()) - 1;

  m_uiForm.spPlotSpectrum->setMaximum(maxWsIndex);
  m_uiForm.spPlotSpectrum->setMinimum(0);
  m_uiForm.spPlotSpectrum->setValue(0);

  m_uiForm.spSpectraMin->setMaximum(maxWsIndex);
  m_uiForm.spSpectraMin->setMinimum(0);

  m_uiForm.spSpectraMax->setMaximum(maxWsIndex);
  m_uiForm.spSpectraMax->setMinimum(0);
  m_uiForm.spSpectraMax->setValue(maxWsIndex);
}

/**
 * Create a resolution workspace with the same number of histograms as in the
 * sample, if the resolution and sample differ in their number of histograms.
 *
 * Needed to allow DiffSphere and DiffRotDiscreteCircle fit functions to work as
 * they need to have the WorkspaceIndex attribute set.
 */
void ConvFit::extendResolutionWorkspace() {
  auto inputWs = inputWorkspace();

  if (inputWs && m_uiForm.dsResInput->isValid()) {
    const std::string resWsName =
        m_uiForm.dsResInput->getCurrentDataName().toStdString();
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
    IAlgorithm_sptr cloneAlg =
        AlgorithmManager::Instance().create("CloneWorkspace");
    cloneAlg->setLogging(false);
    cloneAlg->initialize();
    cloneAlg->setProperty("InputWorkspace", resWsName);
    cloneAlg->setProperty("OutputWorkspace", "__ConvFit_Resolution");
    cloneAlg->execute();
    // Append to cloned workspace if necessary
    if (resolutionNumHist == 1 && numHist > 1) {
      IAlgorithm_sptr appendAlg =
          AlgorithmManager::Instance().create("AppendSpectra");
      appendAlg->setLogging(false);
      appendAlg->initialize();
      appendAlg->setPropertyValue("InputWorkspace1", "__ConvFit_Resolution");
      appendAlg->setPropertyValue("InputWorkspace2", resWsName);
      appendAlg->setProperty("Number", static_cast<int>(numHist - 1));
      appendAlg->setPropertyValue("OutputWorkspace", "__ConvFit_Resolution");
      appendAlg->execute();
    }
  }
}

namespace {
////////////////////////////
// Anon Helper functions. //
////////////////////////////

/**
 * Takes an index and a name, and constructs a single level parameter name
 * for use with function ties, etc.
 *
 * @param index :: the index of the function in the first level.
 * @param name  :: the name of the parameter inside the function.
 *
 * @returns the constructed function parameter name.
 */
std::string createParName(size_t index, const std::string &name = "") {
  std::stringstream prefix;
  prefix << "f" << index << "." << name;
  return prefix.str();
}

/**
 * Takes an index, a sub index and a name, and constructs a double level
 * (nested) parameter name for use with function ties, etc.
 *
 * @param index    :: the index of the function in the first level.
 * @param subIndex :: the index of the function in the second level.
 * @param name     :: the name of the parameter inside the function.
 *
 * @returns the constructed function parameter name.
 */
std::string createParName(size_t index, size_t subIndex,
                          const std::string &name = "") {
  std::stringstream prefix;
  prefix << "f" << index << ".f" << subIndex << "." << name;
  return prefix.str();
}
} // namespace

/**
 * Creates a function to carry out the fitting in the "ConvFit" tab.  The
 * function consists of various sub functions, with the following structure:
 *
 * Composite
 *  |
 *  +- LinearBackground
 *  +- Convolution
 *      |
 *      +- Resolution
 *      +- Model (AT LEAST one delta function or one/two lorentzians.)
 *          |
 *          +- Delta Function(yes/no)
 *				+- ProductFunction
 *					|
 *					+- Lorentzian 1(yes/no)
 *					+- Temperature Correction(yes/no)
 *				+- ProductFunction
 *					|
 *					+- Lorentzian 2(yes/no)
 *					+- Temperature Correction(yes/no)
 *				+- ProductFunction
 *					|
 *					+- InelasticDiffSphere(yes/no)
 *					+- Temperature Correction(yes/no)
 *				+- ProductFunction
 *					|
 *					+- InelasticDiffRotDisCircle(yes/no)
 *					+- Temperature Correction(yes/no)
 *
 * @param tieCentres :: whether to tie centres of the two lorentzians.
 *
 * @returns the composite fitting function.
 */
CompositeFunction_sptr ConvFit::createFunction(bool tieCentres,
                                               bool addQValues) {
  auto conv = boost::dynamic_pointer_cast<CompositeFunction>(
      FunctionFactory::Instance().createFunction("Convolution"));
  CompositeFunction_sptr comp(new CompositeFunction);

  IFunction_sptr func;
  size_t index = 0;

  // -------------------------------------
  // --- Composite / Linear Background ---
  // -------------------------------------
  func = FunctionFactory::Instance().createFunction("LinearBackground");
  comp->addFunction(func);

  // 0 = Fixed Flat, 1 = Fit Flat, 2 = Fit all
  const int bgType = m_uiForm.cbBackground->currentIndex();

  if (bgType == 0 ||
      !m_properties["Background.A0"]->subProperties().isEmpty()) {
    comp->tie("f0.A0",
              m_properties["Background.A1"]->valueText().toStdString());
  } else {
    func->setParameter("A0",
                       m_properties["Background.A0"]->valueText().toDouble());
  }

  if (bgType != 2) {
    comp->tie("f0.A1", "0.0");
  } else {
    if (!m_properties["Background.A1"]->subProperties().isEmpty()) {
      comp->tie("f0.A1",
                m_properties["Background.A1"]->valueText().toStdString());
    } else {
      func->setParameter("A1",
                         m_properties["Background.A1"]->valueText().toDouble());
    }
  }

  // --------------------------------------------
  // --- Composite / Convolution / Resolution ---
  // --------------------------------------------
  func = FunctionFactory::Instance().createFunction("Resolution");
  conv->addFunction(func);

  // add resolution file
  IFunction::Attribute attr("__ConvFit_Resolution");
  func->setAttribute("Workspace", attr);

  // --------------------------------------------------------
  // --- Composite / Convolution / Model / Delta Function ---
  // --------------------------------------------------------
  CompositeFunction_sptr model(new CompositeFunction);

  bool useDeltaFunc = m_blnManager->value(m_properties["UseDeltaFunc"]);

  if (useDeltaFunc) {
    func = FunctionFactory::Instance().createFunction("DeltaFunction");
    index = model->addFunction(func);
    std::string parName = createParName(index);
    populateFunction(func, model, m_properties["Delta Function"], false,
                     parName);
  }

  // ------------------------------------------------------------
  // --- Composite / Convolution / Model / Temperature Factor ---
  // ------------------------------------------------------------

  // create temperature correction function to multiply with the lorentzians
  IFunction_sptr tempFunc;
  QString temperature = m_uiForm.leTempCorrection->text();
  bool useTempCorrection =
      (!temperature.isEmpty() && m_uiForm.ckTempCorrection->isChecked());

  // -----------------------------------------------------
  // --- Composite / Convolution / Model / Lorentzians ---
  // -----------------------------------------------------
  std::string prefix1;
  std::string prefix2;

  int fitTypeIndex = m_uiForm.cbFitType->currentIndex();
  if (fitTypeIndex > 0) {
    size_t subIndex = 0;
    auto product = boost::dynamic_pointer_cast<CompositeFunction>(
        FunctionFactory::Instance().createFunction("ProductFunction"));

    if (useTempCorrection) {
      createTemperatureCorrection(product);
    }

    // Add 1st Lorentzian

    // if temperature not included then product is Lorentzian * 1
    // create product function for temp * Lorentzian

    QVector<QString> functionNames = indexToFitFunctions(fitTypeIndex, false);

    func = getFunction(functionNames[0]);
    // If addQValues is true and the selected fit function is an inelastic
    // diffusion function, add the Q-Values from the input workspace to the
    // function.
    if ((fitTypeIndex == 3 || fitTypeIndex == 4) && addQValues) {
      func->setWorkspace(inputWorkspace());
    }

    subIndex = product->addFunction(func);
    index = model->addFunction(product);
    prefix1 = createParName(index, subIndex);

    populateFunction(func, model, m_properties[functionNames[0]], false,
                     prefix1);

    // Add 2nd Lorentzian
    if (fitTypeIndex == 2) {
      // if temperature not included then product is Lorentzian * 1
      // create product function for temp * Lorentzian
      auto product = boost::dynamic_pointer_cast<CompositeFunction>(
          FunctionFactory::Instance().createFunction("ProductFunction"));

      if (useTempCorrection) {
        createTemperatureCorrection(product);
      }

      func = getFunction(functionNames[1]);
      subIndex = product->addFunction(func);
      index = model->addFunction(product);
      prefix2 = createParName(index, subIndex);

      populateFunction(func, model, m_properties[functionNames[1]], false,
                       prefix2);
    }
  }

  conv->addFunction(model);
  comp->addFunction(conv);

  // Tie PeakCentres together
  if (tieCentres) {
    std::string tieL = prefix1 + "PeakCentre";
    std::string tieR = prefix2 + "PeakCentre";
    model->tie(tieL, tieR);
  }

  comp->applyTies();
  return comp;
}

/**
 * Creates the correction for the temperature
 */
void ConvFit::createTemperatureCorrection(CompositeFunction_sptr product) {
  // create temperature correction function to multiply with the lorentzians
  IFunction_sptr tempFunc;
  QString temperature = m_uiForm.leTempCorrection->text();

  // create user function for the exponential correction
  // (x*temp) / 1-exp(-(x*temp))
  tempFunc = FunctionFactory::Instance().createFunction("UserFunction");
  // 11.606 is the conversion factor from meV to K
  std::string formula = "((x*11.606)/Temp) / (1 - exp(-((x*11.606)/Temp)))";
  IFunction::Attribute att(formula);
  tempFunc->setAttribute("Formula", att);
  tempFunc->setParameter("Temp", temperature.toDouble());

  product->addFunction(tempFunc);
  product->tie("f0.Temp", temperature.toStdString());
  product->applyTies();
}

/**
 * Obtains the instrument resolution from the provided workspace
 * @param workspaceName The workspaces which holds the instrument
 *                      resolution
 * @return The resolution of the instrument. returns 0 if no resolution data
 * could be found
 */
double ConvFit::getInstrumentResolution(MatrixWorkspace_sptr workspace) {
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
    if (inst->getComponentByName(analyser) == NULL ||
        inst->getComponentByName(analyser)
                ->getNumberParameter("resolution")
                .size() == 0) {
      std::string reflection = inst->getStringParameter("reflection")[0];

      IAlgorithm_sptr loadParamFile =
          AlgorithmManager::Instance().create("LoadParameterFile");
      loadParamFile->setChild(true);
      loadParamFile->initialize();
      loadParamFile->setProperty("Workspace", workspace);
      loadParamFile->setProperty(
          "Filename", idfDirectory + inst->getName() + "_" + analyser + "_" +
                          reflection + "_Parameters.xml");
      loadParamFile->execute();

      if (!loadParamFile->isExecuted()) {
        g_log.warning("Could not load parameter file, ensure instrument "
                      "directory is in data search paths.");
        return 0.0;
      }

      inst = workspace->getInstrument();
    }
    if (inst->getComponentByName(analyser) != NULL) {
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

/**
 * Generate a string to describe the fit type selected by the user.
 * Used when naming the resultant workspaces.
 *
 * Assertions used to guard against any future changes that don't take
 * workspace naming into account.
 *
 * @returns the generated QString.
 */
QString ConvFit::fitTypeString() const {
  QString fitType("");

  if (m_blnManager->value(m_properties["UseDeltaFunc"]))
    fitType += "Delta";

  fitType += m_fitStrings.at(m_uiForm.cbFitType->currentIndex());

  return fitType;
}

/**
 * Generate a string to describe the background selected by the user.
 * Used when naming the resultant workspaces.
 *
 * Assertions used to guard against any future changes that don't take
 * workspace naming into account.
 *
 * @returns the generated QString.
 */
QString ConvFit::backgroundString() const {
  switch (m_uiForm.cbBackground->currentIndex()) {
  case 0:
    return "FixF_s";
  case 1:
    return "FitF_s";
  case 2:
    return "FitL_s";
  default:
    return "";
  }
}

/**
 * Generates a string that defines the fitting minimizer based on the user
 * options.
 *
 * @return Minimizer as a string
 */
QString ConvFit::minimizerString(QString outputName) const {
  QString minimizer = "Levenberg-Marquardt";

  if (m_blnManager->value(m_properties["UseFABADA"])) {
    minimizer = "FABADA";

    int chainLength = static_cast<int>(
        m_dblManager->value(m_properties["FABADAChainLength"]));
    minimizer += ",ChainLength=" + QString::number(chainLength);

    double convergenceCriteria =
        m_dblManager->value(m_properties["FABADAConvergenceCriteria"]);
    minimizer += ",ConvergenceCriteria=" + QString::number(convergenceCriteria);

    double jumpAcceptanceRate =
        m_dblManager->value(m_properties["FABADAJumpAcceptanceRate"]);
    minimizer += ",JumpAcceptanceRate=" + QString::number(jumpAcceptanceRate);

    minimizer += ",PDF=" + outputName + "_PDF";

    if (m_blnManager->value(m_properties["OutputFABADAChain"]))
      minimizer += ",Chains=" + outputName + "_Chain";

    if (m_blnManager->value(m_properties["FABADASimAnnealingApplied"])) {
      minimizer += ",SimAnnealingApplied=1";
    } else {
      minimizer += ",SimAnnealingApplied=0";
    }
    double maximumTemperature =
        m_dblManager->value(m_properties["FABADAMaximumTemperature"]);
    minimizer += ",MaximumTemperature=" + QString::number(maximumTemperature);
    double refSteps =
        m_dblManager->value(m_properties["FABADANumRefrigerationSteps"]);
    minimizer += ",NumRefrigerationSteps=" + QString::number(refSteps);
    double simAnnealingIter =
        m_dblManager->value(m_properties["FABADASimAnnealingIterations"]);
    minimizer += ",SimAnnealingIterations=" + QString::number(simAnnealingIter);
    bool overexploration =
        m_blnManager->value(m_properties["FABADAOverexploration"]);
    minimizer += ",Overexploration=";
    minimizer += overexploration ? "1" : "0";

    double stepsBetweenValues =
        m_dblManager->value(m_properties["FABADAStepsBetweenValues"]);
    minimizer += ",StepsBetweenValues=" + QString::number(stepsBetweenValues);

    double inactiveConvCriterion =
        m_dblManager->value(m_properties["FABADAInactiveConvergenceCriterion"]);
    minimizer += ",InnactiveConvergenceCriterion=" +
                 QString::number(inactiveConvCriterion);

    double binsPDF = m_dblManager->value(m_properties["FABADANumberBinsPDF"]);
    minimizer += ",NumberBinsPDF=" + QString::number(binsPDF);
  }

  return minimizer;
}

/**
 * Changes property tree and plot appearance based on Fit Type
 * @param index A reference to the Fit Type (0-9)
 */
void ConvFit::typeSelection(int index) {

  auto hwhmRangeSelector = m_uiForm.ppPlotTop->getRangeSelector("ConvFitHWHM");

  if (index == 0) {
    hwhmRangeSelector->setVisible(false);
  } else if (index < 3) {
    hwhmRangeSelector->setVisible(true);
  } else {
    hwhmRangeSelector->setVisible(false);
    m_blnManager->setValue(m_properties["UseDeltaFunc"], false);
  }

  // Disable Use Delta Function for DiffSphere and DiffRotDiscreteCircle
  m_properties["UseDeltaFunc"]->setEnabled(index < 3 || index == 7);
}

/**
 * Add/Remove sub property 'BGA1' from background based on Background type
 * @param index A reference to the Background type
 */
void ConvFit::bgTypeSelection(int index) {
  if (index == 2) {
    m_properties["Background"]->addSubProperty(m_properties["Background.A0"]);
  } else {
    m_properties["Background"]->removeSubProperty(
        m_properties["Background.A1"]);
  }
}

/**
 * Updates the plot in the GUI window
 */
void ConvFit::updatePreviewPlots() {
  const auto inputWS = inputWorkspace();

  if (inputWS) {
    // Default FWHM to resolution of instrument
    double resolution = getInstrumentResolution(inputWS);
    if (resolution > 0) {
      m_dblManager->setValue(m_properties["InstrumentResolution"], resolution);
    }
  }

  // If there is a result workspace plot then plot it
  const auto baseGroupName = m_baseName.toStdString() + "_Workspaces";
  IndirectFitAnalysisTab::updatePlot(baseGroupName, m_uiForm.ppPlotTop,
                                     m_uiForm.ppPlotBottom);
  IndirectDataAnalysisTab::updatePlotRange("ConvFitRange", m_uiForm.ppPlotTop);
}

QVector<QString> ConvFit::indexToFitFunctions(const int &fitTypeIndex,
                                              const bool &includeDelta) const {
  QVector<QString> fitFunctions = {};

  if (includeDelta && m_blnManager->value(m_properties["UseDeltaFunc"]))
    fitFunctions.push_back("Delta Function");

  if (fitTypeIndex == 1)
    fitFunctions.push_back("Lorentzian 1");
  else if (fitTypeIndex == 2) {
    fitFunctions.push_back("Lorentzian 1");
    fitFunctions.push_back("Lorentzian 2");
  } else if (fitTypeIndex == 3)
    fitFunctions.push_back("InelasticDiffSphere");
  else if (fitTypeIndex == 4)
    fitFunctions.push_back("InelasticDiffRotDiscreteCircle");
  else if (fitTypeIndex == 5)
    fitFunctions.push_back("ElasticDiffSphere");
  else if (fitTypeIndex == 6)
    fitFunctions.push_back("ElasticDiffRotDiscreteCircle");
  else if (fitTypeIndex == 7) {
    fitFunctions.push_back("StretchedExpFT");
  }

  return fitFunctions;
}

void ConvFit::disablePlotGuess() {
  m_uiForm.ckPlotGuess->setEnabled(false);
  disconnect(m_dblManager, SIGNAL(propertyChanged(QtProperty *)), this,
             SLOT(plotGuess()));
  disconnect(m_uiForm.cbFitType, SIGNAL(currentIndexChanged(int)), this,
             SLOT(plotGuess()));
  disconnect(m_uiForm.ckPlotGuess, SIGNAL(stateChanged(int)), this,
             SLOT(plotGuess()));
}

void ConvFit::enablePlotGuess() {
  m_uiForm.ckPlotGuess->setEnabled(true);
  connect(m_dblManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(plotGuess()));
  connect(m_uiForm.cbFitType, SIGNAL(currentIndexChanged(int)), this,
          SLOT(plotGuess()));
  connect(m_uiForm.ckPlotGuess, SIGNAL(stateChanged(int)), this,
          SLOT(plotGuess()));
  plotGuess();
}

/**
 * Updates the guess for the plot
 */
void ConvFit::plotGuess() {

  // Do nothing if there is not a sample and resolution
  if (m_uiForm.dsResInput->isValid() && m_uiForm.ckPlotGuess->isChecked()) {
    extendResolutionWorkspace();
    const bool tieCentres = (m_uiForm.cbFitType->currentIndex() == 2);
    IndirectDataAnalysisTab::plotGuess(m_uiForm.ppPlotTop,
                                       createFunction(tieCentres, true));
  } else {
    m_uiForm.ppPlotTop->removeSpectrum("Guess");
    m_uiForm.ckPlotGuess->setChecked(false);
  }
}

/**
 * Runs the single fit algorithm
 */
void ConvFit::singleFit() {
  // Validate tab before running a single fit
  if (!validate()) {
    return;
  }

  // ensure algorithm was successful
  m_uiForm.ckPlotGuess->setChecked(false);
  int specNo = m_uiForm.spPlotSpectrum->value();
  setMinimumSpectrum(specNo);
  setMaximumSpectrum(specNo);
  std::string specNoStr = m_uiForm.spPlotSpectrum->text().toStdString();

  setFitFunctions(indexToFitFunctions(m_uiForm.cbFitType->currentIndex()));

  auto cfs = sequentialFit(specNoStr, specNoStr, m_baseName);
  runFitAlgorithm(cfs);
}

/**
 * Handles the user entering a new minimum spectrum index.
 *
 * Prevents the user entering an overlapping spectra range.
 *
 * @param value Minimum spectrum index
 */
void ConvFit::specMinChanged(int value) {
  m_uiForm.spSpectraMax->setMinimum(value);
}

/**
 * Handles the user entering a new maximum spectrum index.
 *
 * Prevents the user entering an overlapping spectra range.
 *
 * @param value Maximum spectrum index
 */
void ConvFit::specMaxChanged(int value) {
  m_uiForm.spSpectraMin->setMaximum(value);
}

void ConvFit::minChanged(double val) {
  m_dblManager->setValue(m_properties["StartX"], val);
}

void ConvFit::maxChanged(double val) {
  m_dblManager->setValue(m_properties["EndX"], val);
}

void ConvFit::hwhmChanged(double val) {
  const double peakCentre =
      m_dblManager->value(m_properties["Lorentzian 1.PeakCentre"]);
  // Always want FWHM to display as positive.
  const double hwhm = std::fabs(val - peakCentre);
  // Update the property
  auto hwhmRangeSelector = m_uiForm.ppPlotTop->getRangeSelector("ConvFitHWHM");
  hwhmRangeSelector->blockSignals(true);
  QString propName = "Lorentzian 1.FWHM";
  m_dblManager->setValue(m_properties[propName], hwhm * 2);
  hwhmRangeSelector->blockSignals(false);
}

void ConvFit::backgLevel(double val) {
  m_dblManager->setValue(m_properties["Background.A0"], val);
}

void ConvFit::updateRS(QtProperty *prop, double val) {
  auto fitRangeSelector = m_uiForm.ppPlotTop->getRangeSelector("ConvFitRange");
  auto backRangeSelector =
      m_uiForm.ppPlotTop->getRangeSelector("ConvFitBackRange");

  if (prop == m_properties["StartX"]) {
    fitRangeSelector->setMinimum(val);
  } else if (prop == m_properties["EndX"]) {
    fitRangeSelector->setMaximum(val);
  } else if (prop == m_properties["Background.A0"]) {
    backRangeSelector->setMinimum(val);
  } else if (prop == m_properties["InstrumentResolution"]) {
    hwhmUpdateRS(val);
  }
}

void ConvFit::hwhmUpdateRS(double val) {
  auto hwhmRangeSelector = m_uiForm.ppPlotTop->getRangeSelector("ConvFitHWHM");
  hwhmRangeSelector->setMinimum(-val / 2);
  hwhmRangeSelector->setMaximum(+val / 2);
}

void ConvFit::checkBoxUpdate(QtProperty *prop, bool checked) {
  UNUSED_ARG(checked);

  if (prop == m_properties["UseDeltaFunc"]) {
    const int fitTypeIndex = m_uiForm.cbFitType->currentIndex();
    fillPlotTypeComboBox(m_uiForm.cbPlotType);

    if (checked) {
      m_properties["Delta Function"]->addSubProperty(
          m_properties["Delta Function.Height"]);
      m_dblManager->setValue(m_properties["Delta Function.Height"], 1.0000);
      m_properties["Delta Function"]->addSubProperty(
          m_properties["Delta Function.Centre"]);
      m_dblManager->setValue(m_properties["Delta Function.Centre"], 0.0000);
    } else {
      m_properties["Delta Function"]->removeSubProperty(
          m_properties["Delta Function.Height"]);
      m_properties["Delta Function"]->removeSubProperty(
          m_properties["Delta Function.Centre"]);
    }
    setPropertyFunctions(indexToFitFunctions(fitTypeIndex));
  } else if (prop == m_properties["UseFABADA"]) {
    if (checked) {
      // FABADA needs a much higher iteration limit
      m_dblManager->setValue(m_properties["MaxIterations"], 20000);
      showFABADA(m_blnManager->value(m_properties["FABADAAdvanced"]));
    } else {
      m_dblManager->setValue(m_properties["MaxIterations"], 500);
      hideFABADA();
    }
  } else if (prop == m_properties["FABADAAdvanced"]) {
    showFABADA(checked);
  }
}

/** Shows FABADA minimizer options in the property browser
 *
 * @param advanced :: true if advanced options should be shown, false otherwise
 */
void ConvFit::showFABADA(bool advanced) {

  m_properties["FABADA"]->addSubProperty(m_properties["OutputFABADAChain"]);
  m_properties["FABADA"]->addSubProperty(m_properties["FABADAChainLength"]);
  m_properties["FABADA"]->addSubProperty(
      m_properties["FABADAConvergenceCriteria"]);
  m_properties["FABADA"]->addSubProperty(
      m_properties["FABADAJumpAcceptanceRate"]);
  m_properties["FABADA"]->addSubProperty(m_properties["FABADAAdvanced"]);
  if (advanced) {
    m_properties["FABADA"]->addSubProperty(
        m_properties["FABADAStepsBetweenValues"]);
    m_properties["FABADA"]->addSubProperty(
        m_properties["FABADAInactiveConvergenceCriterion"]);
    m_properties["FABADA"]->addSubProperty(
        m_properties["FABADASimAnnealingApplied"]);
    m_properties["FABADA"]->addSubProperty(
        m_properties["FABADAMaximumTemperature"]);
    m_properties["FABADA"]->addSubProperty(
        m_properties["FABADANumRefrigerationSteps"]);
    m_properties["FABADA"]->addSubProperty(
        m_properties["FABADASimAnnealingIterations"]);
    m_properties["FABADA"]->addSubProperty(
        m_properties["FABADAOverexploration"]);
    m_properties["FABADA"]->addSubProperty(m_properties["FABADANumberBinsPDF"]);
  } else {
    m_properties["FABADA"]->removeSubProperty(
        m_properties["FABADAStepsBetweenValues"]);
    m_properties["FABADA"]->removeSubProperty(
        m_properties["FABADAInactiveConvergenceCriterion"]);
    m_properties["FABADA"]->removeSubProperty(
        m_properties["FABADASimAnnealingApplied"]);
    m_properties["FABADA"]->removeSubProperty(
        m_properties["FABADAMaximumTemperature"]);
    m_properties["FABADA"]->removeSubProperty(
        m_properties["FABADANumRefrigerationSteps"]);
    m_properties["FABADA"]->removeSubProperty(
        m_properties["FABADASimAnnealingIterations"]);
    m_properties["FABADA"]->removeSubProperty(
        m_properties["FABADAOverexploration"]);
    m_properties["FABADA"]->removeSubProperty(
        m_properties["FABADANumberBinsPDF"]);
  }
}

/** Hide FABADA minimizer options from the browser
 *
 */
void ConvFit::hideFABADA() {

  m_properties["FABADA"]->removeSubProperty(m_properties["OutputFABADAChain"]);
  m_properties["FABADA"]->removeSubProperty(m_properties["FABADAChainLength"]);
  m_properties["FABADA"]->removeSubProperty(
      m_properties["FABADAConvergenceCriteria"]);
  m_properties["FABADA"]->removeSubProperty(
      m_properties["FABADAJumpAcceptanceRate"]);
  m_properties["FABADA"]->removeSubProperty(m_properties["FABADAAdvanced"]);

  // Advanced options
  m_properties["FABADA"]->removeSubProperty(
      m_properties["FABADAStepsBetweenValues"]);
  m_properties["FABADA"]->removeSubProperty(
      m_properties["FABADAInactiveConvergenceCriterion"]);
  m_properties["FABADA"]->removeSubProperty(
      m_properties["FABADASimAnnealingApplied"]);
  m_properties["FABADA"]->removeSubProperty(
      m_properties["FABADAMaximumTemperature"]);
  m_properties["FABADA"]->removeSubProperty(
      m_properties["FABADANumRefrigerationSteps"]);
  m_properties["FABADA"]->removeSubProperty(
      m_properties["FABADASimAnnealingIterations"]);
  m_properties["FABADA"]->removeSubProperty(
      m_properties["FABADAOverexploration"]);
  m_properties["FABADA"]->removeSubProperty(
      m_properties["FABADANumberBinsPDF"]);
}

void ConvFit::fitContextMenu(const QPoint &) {
  IndirectFitAnalysisTab::fitContextMenu("ConvFit");
}

void ConvFit::showTieCheckbox(QString fitType) {
  m_uiForm.ckTieCentres->setVisible(fitType == "Two Lorentzians");
}

/**
 * Handles a new fit function being selected.
 * @param functionName Name of new fit function
 */
void ConvFit::fitFunctionSelected(int fitTypeIndex) {
  // If resolution file has been entered update default FWHM to resolution
  if (m_uiForm.dsResInput->getCurrentDataName().compare("") != 0) {
    const auto res = getInstrumentResolution(inputWorkspace());
    setDefaultPropertyValue("FWHM", res);
  }

  setPropertyFunctions(indexToFitFunctions(fitTypeIndex));
  fillPlotTypeComboBox(m_uiForm.cbPlotType);
}

QString ConvFit::addPrefixToParameter(const QString &parameter,
                                      const QString &functionName,
                                      const int &functionNumber) const {
  QString tempPrefix = m_usedTemperature ? "f1." : "f0.";

  if (functionName == "Delta Function")
    return "f1.f1.f" + QString::number(functionNumber) + "." + parameter;
  else
    return "f1.f1.f" + QString::number(functionNumber) + "." + tempPrefix +
           parameter;
}

QString ConvFit::addPrefixToParameter(const QString &parameter,
                                      const QString &) const {
  QString tempPrefix = m_usedTemperature ? "f1." : "f0.";
  return "f1.f1." + tempPrefix + parameter;
}

/**
 * Populates the default parameter map with the initial default values
 */
void ConvFit::createDefaultParamsMap() {
  setDefaultPropertyValue("PeakCentre", 0.0);
  setDefaultPropertyValue("Centre", 0.0);
  // Reset all parameters to default of 1
  setDefaultPropertyValue("Amplitude", 1.0);
  setDefaultPropertyValue("beta", 1.0);
  setDefaultPropertyValue("Decay", 1.0);
  setDefaultPropertyValue("Diffusion", 1.0);
  setDefaultPropertyValue("height",
                          1.0); // Lower case in StretchedExp - this can be
                                // improved with a case insensitive check
  setDefaultPropertyValue("Height", 1.0);
  setDefaultPropertyValue("Intensity", 1.0);
  setDefaultPropertyValue("Radius", 1.0);
  setDefaultPropertyValue("tau", 1.0);
}

IFunction_sptr ConvFit::getFunction(const QString &functionName) const {
  if (functionName.startsWith("Lorentzian"))
    return IndirectFitAnalysisTab::getFunction("Lorentzian");
  else if (functionName == "Delta Function")
    return IndirectFitAnalysisTab::getFunction("DeltaFunction");
  else if (functionName == "Background")
    return IndirectFitAnalysisTab::getFunction("LinearBackground");
  else
    return IndirectFitAnalysisTab::getFunction(functionName);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
