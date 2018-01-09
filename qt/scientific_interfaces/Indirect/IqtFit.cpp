#include "IqtFit.h"

#include "../General/UserInputValidator.h"
#include "MantidQtWidgets/LegacyQwt/RangeSelector.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/WorkspaceGroup.h"

#include <QMenu>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>

using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("IqtFit");
}

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

IqtFit::IqtFit(QWidget *parent) : IndirectFitAnalysisTab(parent), m_ties() {
  m_uiForm.setupUi(parent);
  m_iqtFTree = m_propertyTree;
}

void IqtFit::setup() {
  setMinimumSpectrum(0);
  setMaximumSpectrum(0);
  m_uiForm.properties->addWidget(m_iqtFTree);

  auto fitRangeSelector = m_uiForm.ppPlotTop->addRangeSelector("IqtFitRange");
  connect(fitRangeSelector, SIGNAL(minValueChanged(double)), this,
          SLOT(xMinSelected(double)));
  connect(fitRangeSelector, SIGNAL(maxValueChanged(double)), this,
          SLOT(xMaxSelected(double)));

  auto backgroundRangeSelector = m_uiForm.ppPlotTop->addRangeSelector(
      "IqtFitBackground", MantidWidgets::RangeSelector::YSINGLE);
  backgroundRangeSelector->setRange(0.0, 1.0);
  backgroundRangeSelector->setColour(Qt::darkGreen);
  connect(backgroundRangeSelector, SIGNAL(minValueChanged(double)), this,
          SLOT(backgroundSelected(double)));

  // setupTreePropertyBrowser
  m_dblManager = new QtDoublePropertyManager(m_parentWidget);

  m_iqtFTree->setFactoryForManager(m_blnManager, m_blnEdFac);
  m_iqtFTree->setFactoryForManager(m_dblManager, m_dblEdFac);

  m_properties["FitRange"] = m_grpManager->addProperty("Fitting Range");
  m_properties["StartX"] = m_dblManager->addProperty("StartX");
  m_dblManager->setDecimals(m_properties["StartX"], NUM_DECIMALS);
  m_properties["EndX"] = m_dblManager->addProperty("EndX");
  m_dblManager->setDecimals(m_properties["EndX"], NUM_DECIMALS);
  m_properties["FitRange"]->addSubProperty(m_properties["StartX"]);
  m_properties["FitRange"]->addSubProperty(m_properties["EndX"]);
  m_iqtFTree->addProperty(m_properties["FitRange"]);

  m_properties["MaxIterations"] = m_dblManager->addProperty("Max Iterations");
  m_dblManager->setDecimals(m_properties["MaxIterations"], 0);
  m_dblManager->setValue(m_properties["MaxIterations"], 500);
  m_iqtFTree->addProperty(m_properties["MaxIterations"]);

  // FABADA
  m_properties["FABADA"] = m_grpManager->addProperty("Bayesian");
  m_properties["UseFABADA"] = m_blnManager->addProperty("Use FABADA");
  m_properties["FABADA"]->addSubProperty(m_properties["UseFABADA"]);
  m_properties["OutputFABADAChain"] = m_blnManager->addProperty("Output Chain");
  m_properties["FABADAChainLength"] = m_dblManager->addProperty("Chain Length");
  m_dblManager->setDecimals(m_properties["FABADAChainLength"], 0);
  m_dblManager->setValue(m_properties["FABADAChainLength"], 10000);
  m_properties["FABADAConvergenceCriteria"] =
      m_dblManager->addProperty("Convergence Criteria");
  m_dblManager->setValue(m_properties["FABADAConvergenceCriteria"], 0.1);
  m_properties["FABADAJumpAcceptanceRate"] =
      m_dblManager->addProperty("Acceptance Rate");
  m_dblManager->setValue(m_properties["FABADAJumpAcceptanceRate"], 0.25);
  m_iqtFTree->addProperty(m_properties["FABADA"]);

  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(propertyChanged(QtProperty *, double)));
  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(propertyChanged(QtProperty *, double)));

  m_properties["Background"] = createFunctionProperty("Background");
  m_properties["Background"]->removeSubProperty(m_properties["Background.A1"]);
  m_iqtFTree->addProperty(m_properties["Background"]);

  m_properties["Exponential1"] = createFunctionProperty("Exponential1");
  m_properties["Exponential2"] = createFunctionProperty("Exponential2");

  m_properties["StretchedExp"] = createFunctionProperty("StretchedExp");

  m_dblManager->setMinimum(m_properties["Background.A0"], 0);
  m_dblManager->setMaximum(m_properties["Background.A0"], 1);

  m_dblManager->setMinimum(m_properties["Exponential1.Intensity"], 0);
  m_dblManager->setMaximum(m_properties["Exponential1.Intensity"], 1);

  m_dblManager->setMinimum(m_properties["Exponential2.Intensity"], 0);
  m_dblManager->setMaximum(m_properties["Exponential2.Intensity"], 1);

  m_dblManager->setMinimum(m_properties["StretchedExp.Intensity"], 0);
  m_dblManager->setMaximum(m_properties["StretchedExp.Intensity"], 1);

  typeSelection(m_uiForm.cbFitType->currentIndex());

  m_uiForm.ckPlotGuess->setChecked(false);
  enablePlotGuess();

  // Signal/slot ui connections
  connect(m_uiForm.dsSampleInput, SIGNAL(dataReady(const QString &)), this,
          SLOT(newDataLoaded(const QString &)));
  connect(m_uiForm.cbFitType, SIGNAL(currentIndexChanged(int)), this,
          SLOT(typeSelection(int)));
  connect(m_uiForm.pbSingle, SIGNAL(clicked()), this, SLOT(singleFit()));

  // Update plot when fit type changes
  connect(m_uiForm.cbFitType, SIGNAL(currentIndexChanged(int)), this,
          SLOT(updatePreviewPlots()));

  connect(m_uiForm.spPlotSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(setSelectedSpectrum(int)));
  connect(m_uiForm.spPlotSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(updatePreviewPlots()));
  connect(m_uiForm.spPlotSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(updateProperties(int)));

  connect(m_uiForm.spSpectraMin, SIGNAL(valueChanged(int)), this,
          SLOT(specMinChanged(int)));
  connect(m_uiForm.spSpectraMax, SIGNAL(valueChanged(int)), this,
          SLOT(specMaxChanged(int)));
  connect(m_uiForm.cbPlotType, SIGNAL(currentIndexChanged(QString)), this,
          SLOT(updateCurrentPlotOption(QString)));

  // Set a custom handler for the QTreePropertyBrowser's ContextMenu event
  m_iqtFTree->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(m_iqtFTree, SIGNAL(customContextMenuRequested(const QPoint &)), this,
          SLOT(fitContextMenu(const QPoint &)));

  connect(m_blnManager, SIGNAL(valueChanged(QtProperty *, bool)), this,
          SLOT(checkBoxUpdate(QtProperty *, bool)));
  connect(m_uiForm.pbPlot, SIGNAL(clicked()), this, SLOT(plotWorkspace()));
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveResult()));
  connect(m_uiForm.pbPlotPreview, SIGNAL(clicked()), this,
          SLOT(plotCurrentPreview()));
}

void IqtFit::run() {
  auto inputWs = inputWorkspace();

  if (!inputWs) {
    QString msg = "Input workspace was deleted from the Analysis Data Service "
                  "before Algorithm could run.";
    showMessageBox(msg);
  }

  setMinimumSpectrum(m_uiForm.spSpectraMin->value());
  setMaximumSpectrum(m_uiForm.spSpectraMax->value());

  setFitFunctions(indexToFitFunctions(selectedSpectrum()));
  auto iqtFitAlg =
      iqtFitAlgorithm(inputWs, minimumSpectrum(), maximumSpectrum());
  runFitAlgorithm(iqtFitAlg);
}

Mantid::API::IAlgorithm_sptr
IqtFit::iqtFitAlgorithm(MatrixWorkspace_sptr inputWs, const size_t &specMin,
                        const size_t &specMax) {
  const bool constrainBeta = m_uiForm.ckConstrainBeta->isChecked();
  const bool constrainIntens = m_uiForm.ckConstrainIntensities->isChecked();
  CompositeFunction_sptr func = createFunction();
  func->tie("f0.A1", "0");

  if (constrainIntens) {
    constrainIntensities(func);
  }

  func->applyTies();

  const auto function = std::string(func->asString());
  const auto fitType = fitTypeString().toStdString();
  const auto minimizer = minimizerString("$outputname_$wsindex");
  m_plotOption = m_uiForm.cbPlotType->currentText().toStdString();
  const auto startX = boost::lexical_cast<double>(
      m_properties["StartX"]->valueText().toStdString());
  const auto endX = boost::lexical_cast<double>(
      m_properties["EndX"]->valueText().toStdString());
  const auto maxIt = boost::lexical_cast<long>(
      m_properties["MaxIterations"]->valueText().toStdString());

  m_baseName =
      constructBaseName(inputWs->getName(), fitType, true, specMin, specMax);

  IAlgorithm_sptr iqtFitAlg;

  if (!constrainBeta) {
    iqtFitAlg = AlgorithmManager::Instance().create("IqtFitSequential");
  } else {
    iqtFitAlg = AlgorithmManager::Instance().create("IqtFitMultiple");
  }

  auto replaceAlg = replaceInfinityAndNaN(inputWs->getName());
  replaceAlg->execute();
  MatrixWorkspace_sptr iqtInputWs = replaceAlg->getProperty("OutputWorkspace");

  iqtFitAlg->initialize();
  iqtFitAlg->setProperty("InputWorkspace", iqtInputWs);
  iqtFitAlg->setProperty("Function", function);
  iqtFitAlg->setProperty("FitType", fitType);
  iqtFitAlg->setProperty("StartX", startX);
  iqtFitAlg->setProperty("EndX", endX);
  iqtFitAlg->setProperty("SpecMin", boost::numeric_cast<long>(specMin));
  iqtFitAlg->setProperty("SpecMax", boost::numeric_cast<long>(specMax));
  iqtFitAlg->setProperty("Minimizer", minimizer.toStdString());
  iqtFitAlg->setProperty("MaxIterations", maxIt);
  iqtFitAlg->setProperty("ConstrainIntensities", constrainIntens);
  iqtFitAlg->setProperty("OutputResultWorkspace", m_baseName + "_Result");
  iqtFitAlg->setProperty("OutputParameterWorkspace",
                         m_baseName + "_Parameters");
  iqtFitAlg->setProperty("OutputWorkspaceGroup", m_baseName + "_Workspaces");
  return iqtFitAlg;
}

/**
 * Plot workspace based on user input
 */
void IqtFit::plotWorkspace() {
  IndirectFitAnalysisTab::plotResult(m_baseName + "_Result",
                                     QString::fromStdString(m_plotOption));
}

/**
 * Save the result of the algorithm
 */
void IqtFit::saveResult() {
  IndirectFitAnalysisTab::saveResult(m_baseName + "_Result");
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

  IndirectFitAnalysisTab::fitAlgorithmComplete(m_baseName + "_Parameters");
  m_uiForm.pbPlot->setEnabled(true);
  m_uiForm.pbSave->setEnabled(true);
  m_uiForm.cbPlotType->setEnabled(true);
}

/**
 * Constructs the desired output base name for the  IqtFitMultiple
 * @param inputName     :: Name of the inputworkspace
 * @param fitType       :: The type of fit that is being performed
 * @param multi         :: If the fit is running the IqtFitMultiple
 * @param specMin       :: Minimum number of spectra being fitted
 * @param specMax       :: Maximum number of spectra being fitted
 * @return the base name
 */
std::string IqtFit::constructBaseName(const std::string &inputName,
                                      const std::string &fitType,
                                      const bool &multi, const size_t &specMin,
                                      const size_t &specMax) {
  QString functionType = QString::fromStdString(fitType);
  if (multi) {
    functionType = "1Smult_s";
  }

  QString baseName = QString::fromStdString(inputName);
  baseName = baseName.left(baseName.lastIndexOf("_"));
  baseName += "_IqtFit_";
  baseName += functionType;
  baseName += QString::number(specMin);

  // Check whether a single spectrum is being fit
  if (specMin != specMax) {
    baseName += "_to_";
    baseName += QString::number(specMax);
  }
  const auto baseName_str = baseName.toStdString();
  return baseName_str;
}

bool IqtFit::validate() {
  UserInputValidator uiv;

  uiv.checkDataSelectorIsValid("Sample", m_uiForm.dsSampleInput);

  auto range = std::make_pair(m_dblManager->value(m_properties["StartX"]),
                              m_dblManager->value(m_properties["EndX"]));
  uiv.checkValidRange("Ranges", range);

  QString error = uiv.generateErrorMessage();
  showMessageBox(error);

  return error.isEmpty();
}

void IqtFit::loadSettings(const QSettings &settings) {
  m_uiForm.dsSampleInput->readSettings(settings.group());
}

/**
 * Called when new data has been loaded by the data selector.
 *
 * Configures ranges for spin boxes before raw plot is done.
 *
 * @param wsName Name of new workspace loaded
 */
void IqtFit::newDataLoaded(const QString wsName) {
  m_baseName.clear();
  IndirectFitAnalysisTab::newInputDataLoaded(wsName);

  int maxWsIndex =
      static_cast<int>(inputWorkspace()->getNumberHistograms()) - 1;

  m_uiForm.spPlotSpectrum->setMaximum(maxWsIndex);
  m_uiForm.spPlotSpectrum->setMinimum(0);
  m_uiForm.spPlotSpectrum->setValue(0);

  m_uiForm.spSpectraMin->setMaximum(maxWsIndex);
  m_uiForm.spSpectraMin->setMinimum(0);

  m_uiForm.spSpectraMax->setMaximum(maxWsIndex);
  m_uiForm.spSpectraMax->setMinimum(0);
  m_uiForm.spSpectraMax->setValue(maxWsIndex);

  setDefaultParameters();
}

CompositeFunction_sptr IqtFit::createFunction(bool tie) {
  CompositeFunction_sptr result(new CompositeFunction);
  const int fitType = m_uiForm.cbFitType->currentIndex();
  const auto functionNames = indexToFitFunctions(fitType);

  for (int i = 0; i < functionNames.size(); ++i) {
    const auto &functionName = functionNames[i];
    IFunction_sptr func1 = getFunction(functionName);
    result->addFunction(func1);
    const std::string prefix = "f" + std::to_string(i) + ".";
    populateFunction(func1, result, m_properties[functionName], tie, prefix);
  }

  // Return CompositeFunction object to caller.
  result->applyTies();
  return result;
}

QString IqtFit::fitTypeString() const {
  switch (m_uiForm.cbFitType->currentIndex()) {
  case 0:
    return "1E_s";
  case 1:
    return "2E_s";
  case 2:
    return "1S_s";
  case 3:
    return "1E1S_s";
  default:
    return "s";
  };
}

void IqtFit::typeSelection(int index) {
  disconnect(m_uiForm.cbPlotType, SIGNAL(currentIndexChanged(QString)), this,
             SLOT(updateCurrentPlotOption(QString)));
  fillPlotTypeComboBox(m_uiForm.cbPlotType);

  // option should only be available with a single stretched exponential
  m_uiForm.ckConstrainBeta->setEnabled((index == 2));
  if (!m_uiForm.ckConstrainBeta->isEnabled()) {
    m_uiForm.ckConstrainBeta->setChecked(false);
  }

  const auto optionIndex =
      m_uiForm.cbPlotType->findText(QString::fromStdString(m_plotOption));
  if (optionIndex != -1) {
    m_uiForm.cbPlotType->setCurrentIndex(optionIndex);
  } else {
    m_uiForm.cbPlotType->setCurrentIndex(0);
  }

  setPropertyFunctions(indexToFitFunctions(index));
  fillPlotTypeComboBox(m_uiForm.cbPlotType);
  connect(m_uiForm.cbPlotType, SIGNAL(currentIndexChanged(QString)), this,
          SLOT(updateCurrentPlotOption(QString)));
}

/**
 * Update the current plot option selected
 */
void IqtFit::updateCurrentPlotOption(QString newOption) {
  m_plotOption = newOption.toStdString();
}

void IqtFit::updatePreviewPlots() {
  // If there is a result workspace plot then plot it
  const auto groupName = m_baseName + "_Workspaces";
  IndirectFitAnalysisTab::updatePlot(groupName, m_uiForm.ppPlotTop,
                                     m_uiForm.ppPlotBottom);

  IndirectDataAnalysisTab::updatePlotRange("IqtFitRange", m_uiForm.ppPlotTop);
  plotGuess();
}

void IqtFit::setDefaultParameters() {
  double background = m_dblManager->value(m_properties["Background.A0"]);
  // intensity is always 1-background
  setDefaultPropertyValue("Height", 1.0 - background);

  auto inputWs = inputWorkspace();
  double tau = 0;

  if (inputWs) {
    auto x = inputWs->x(0);
    auto y = inputWs->y(0);

    if (x.size() > 4) {
      tau = -x[4] / log(y[4]);
    }
  }

  setDefaultPropertyValue("Lifetime", tau);
  setDefaultPropertyValue("Stretching", 1.0);
  setDefaultPropertyValue("A0", 0.0);
}

/**
 * Handles the user entering a new minimum spectrum index.
 *
 * Prevents the user entering an overlapping spectra range.
 *
 * @param value Minimum spectrum index
 */
void IqtFit::specMinChanged(int value) {
  m_uiForm.spSpectraMax->setMinimum(value);
}

/**
 * Handles the user entering a new maximum spectrum index.
 *
 * Prevents the user entering an overlapping spectra range.
 *
 * @param value Maximum spectrum index
 */
void IqtFit::specMaxChanged(int value) {
  m_uiForm.spSpectraMin->setMaximum(value);
}

void IqtFit::xMinSelected(double val) {
  m_dblManager->setValue(m_properties["StartX"], val);
}

void IqtFit::xMaxSelected(double val) {
  m_dblManager->setValue(m_properties["EndX"], val);
}

void IqtFit::backgroundSelected(double val) {
  m_dblManager->setValue(m_properties["Background.A0"], val);
  m_dblManager->setValue(m_properties["Exponential1.Height"], 1.0 - val);
  m_dblManager->setValue(m_properties["Exponential2.Height"], 1.0 - val);
  m_dblManager->setValue(m_properties["StretchedExp.Height"], 1.0 - val);
}

void IqtFit::propertyChanged(QtProperty *prop, double val) {
  auto fitRangeSelector = m_uiForm.ppPlotTop->getRangeSelector("IqtFitRange");
  auto backgroundRangeSelector =
      m_uiForm.ppPlotTop->getRangeSelector("IqtFitBackground");
  auto specNo = m_uiForm.spPlotSpectrum->value();
  bool autoUpdate = this->hasParameterValue(prop->propertyName(), specNo);

  if (prop == m_properties["StartX"]) {
    fitRangeSelector->setMinimum(val);
  } else if (prop == m_properties["EndX"]) {
    fitRangeSelector->setMaximum(val);
  } else if (autoUpdate && prop == m_properties["Background.A0"]) {
    backgroundRangeSelector->setMinimum(val);
    m_dblManager->setValue(m_properties["Exponential1.Height"], 1.0 - val);
    m_dblManager->setValue(m_properties["Exponential2.Height"], 1.0 - val);
    m_dblManager->setValue(m_properties["StretchedExp.Height"], 1.0 - val);
  } else if (autoUpdate && (prop == m_properties["Exponential1.Height"] ||
                            prop == m_properties["Exponential2.Height"] ||
                            prop == m_properties["StretchedExp.Height"])) {
    backgroundRangeSelector->setMinimum(1.0 - val);
    m_dblManager->setValue(m_properties["Exponential1.Height"], val);
    m_dblManager->setValue(m_properties["Exponential2.Height"], val);
    m_dblManager->setValue(m_properties["StretchedExp.Height"], val);
  }
}

void IqtFit::checkBoxUpdate(QtProperty *prop, bool checked) {
  if (prop == m_properties["UseFABADA"]) {
    if (checked) {
      m_dblManager->setValue(m_properties["MaxIterations"], 20000);

      m_properties["FABADA"]->addSubProperty(m_properties["OutputFABADAChain"]);
      m_properties["FABADA"]->addSubProperty(m_properties["FABADAChainLength"]);
      m_properties["FABADA"]->addSubProperty(
          m_properties["FABADAConvergenceCriteria"]);
      m_properties["FABADA"]->addSubProperty(
          m_properties["FABADAJumpAcceptanceRate"]);
    } else {
      m_dblManager->setValue(m_properties["MaxIterations"], 500);

      m_properties["FABADA"]->removeSubProperty(
          m_properties["OutputFABADAChain"]);
      m_properties["FABADA"]->removeSubProperty(
          m_properties["FABADAChainLength"]);
      m_properties["FABADA"]->removeSubProperty(
          m_properties["FABADAConvergenceCriteria"]);
      m_properties["FABADA"]->removeSubProperty(
          m_properties["FABADAJumpAcceptanceRate"]);
    }
  }
}

void IqtFit::constrainIntensities(CompositeFunction_sptr func) {
  std::string paramName = "f1.Height";
  size_t index = func->parameterIndex(paramName);

  switch (m_uiForm.cbFitType->currentIndex()) {
  case 0: // 1 Exp
  case 2: // 1 Str
    if (func->isActive(index)) {
      func->tie(paramName, "1-f0.A0");
    } else {
      std::string paramValue =
          boost::lexical_cast<std::string>(func->getParameter(paramName));
      func->tie(paramName, paramValue);
      func->tie("f0.A0", "1-" + paramName);
    }
    break;
  case 1: // 2 Exp
  case 3: // 1 Exp & 1 Str
    if (func->isActive(index)) {
      func->tie(paramName, "1-f2.Height-f0.A0");
    } else {
      std::string paramValue =
          boost::lexical_cast<std::string>(func->getParameter(paramName));
      func->tie(paramName, "1-f2.Height-f0.A0");
      func->tie(paramName, paramValue);
    }
    break;
  }
}

/**
 * Generates a string that defines the fitting minimizer based on the user
 * options.
 *
 * @return Minimizer as a string
 */
QString IqtFit::minimizerString(QString outputName) const {
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
  }

  return minimizer;
}

void IqtFit::singleFit() {
  if (!validate())
    return;

  // Don't plot a new guess curve until there is a fit
  disconnect(m_dblManager, SIGNAL(propertyChanged(QtProperty *)), this,
             SLOT(plotGuess()));

  setFitFunctions(indexToFitFunctions(selectedSpectrum()));
  size_t specNo = m_uiForm.spPlotSpectrum->text().toULongLong();
  m_singleFitAlg = iqtFitAlgorithm(inputWorkspace(), specNo, specNo);
  runFitAlgorithm(m_singleFitAlg);
}

void IqtFit::disablePlotGuess() {
  m_uiForm.ckPlotGuess->setEnabled(false);
  disconnect(m_dblManager, SIGNAL(propertyChanged(QtProperty *)), this,
             SLOT(plotGuess()));
  disconnect(m_uiForm.ckPlotGuess, SIGNAL(stateChanged(int)), this,
             SLOT(plotGuess()));
  disconnect(m_uiForm.cbFitType, SIGNAL(currentIndexChanged(int)), this,
             SLOT(plotGuess()));
}

void IqtFit::enablePlotGuess() {
  m_uiForm.ckPlotGuess->setEnabled(true);
  connect(m_dblManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(plotGuess()));
  connect(m_uiForm.ckPlotGuess, SIGNAL(stateChanged(int)), this,
          SLOT(plotGuess()));
  connect(m_uiForm.cbFitType, SIGNAL(currentIndexChanged(int)), this,
          SLOT(plotGuess()));
  plotGuess();
}

void IqtFit::plotGuess() {

  if (m_uiForm.ckPlotGuess->isChecked())
    IndirectDataAnalysisTab::plotGuess(m_uiForm.ppPlotTop,
                                       createFunction(true));
  else {
    m_uiForm.ppPlotTop->removeSpectrum("Guess");
    m_uiForm.ckPlotGuess->setChecked(false);
  }
}

/*
 * Updates a vector storing which functions were last used in a fitting.
 */
QVector<QString> IqtFit::indexToFitFunctions(const int &fitTypeIndex) const {
  if (fitTypeIndex == 0)
    return {"Background", "Exponential1"};
  else if (fitTypeIndex == 1)
    return {"Background", "Exponential1", "Exponential2"};
  else if (fitTypeIndex == 2)
    return {"Background", "StretchedExp"};
  else if (fitTypeIndex == 3)
    return {"Background", "Exponential1", "StretchedExp"};
  else
    return {};
}

void IqtFit::fitContextMenu(const QPoint &) {
  IndirectFitAnalysisTab::fitContextMenu("IqtFit");
}

IAlgorithm_sptr IqtFit::replaceInfinityAndNaN(const std::string &wsName) {
  auto replaceAlg = AlgorithmManager::Instance().create("ReplaceSpecialValues");
  replaceAlg->setChild(true);
  replaceAlg->initialize();
  replaceAlg->setProperty("InputWorkspace", wsName);
  replaceAlg->setProperty("NaNValue", 0.0);
  replaceAlg->setProperty("InfinityError", 0.0);
  replaceAlg->setProperty("OutputWorkspace", wsName);
  return replaceAlg;
}

IFunction_sptr IqtFit::getFunction(const QString &functionName) const {
  if (functionName.startsWith("Exponential"))
    return IndirectFitAnalysisTab::getFunction("ExpDecay");
  else if (functionName == "StretchedExp")
    return IndirectFitAnalysisTab::getFunction("StretchExp");
  else if (functionName == "Background")
    return IndirectFitAnalysisTab::getFunction("LinearBackground");
  else
    return IndirectFitAnalysisTab::getFunction(functionName);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
