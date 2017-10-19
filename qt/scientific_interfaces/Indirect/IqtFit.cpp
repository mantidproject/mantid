#include "IqtFit.h"

#include "../General/UserInputValidator.h"
#include "MantidQtWidgets/Common/RangeSelector.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionDomain1D.h"
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

IqtFit::IqtFit(QWidget *parent)
    : IndirectDataAnalysisTab(parent), m_stringManager(nullptr),
      m_iqtFTree(nullptr), m_iqtFRangeManager(nullptr), m_fixedProps(),
      m_ties(), m_fitFunctions(), m_parameterValues(), m_parameterToProperty() {
  m_uiForm.setupUi(parent);
}

void IqtFit::setup() {
  setMinimumSpectra(0);
  setMaximumSpectra(0);

  m_stringManager = new QtStringPropertyManager(m_parentWidget);

  m_iqtFTree = new QtTreePropertyBrowser(m_parentWidget);
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
  m_iqtFRangeManager = new QtDoublePropertyManager(m_parentWidget);

  m_iqtFTree->setFactoryForManager(m_blnManager, m_blnEdFac);
  m_iqtFTree->setFactoryForManager(m_dblManager, m_dblEdFac);
  m_iqtFTree->setFactoryForManager(m_iqtFRangeManager, m_dblEdFac);

  m_properties["StartX"] = m_iqtFRangeManager->addProperty("StartX");
  m_iqtFRangeManager->setDecimals(m_properties["StartX"], NUM_DECIMALS);
  m_properties["EndX"] = m_iqtFRangeManager->addProperty("EndX");
  m_dblManager->setDecimals(m_properties["EndX"], NUM_DECIMALS);
  m_properties["MaxIterations"] = m_dblManager->addProperty("Max Iterations");
  m_dblManager->setDecimals(m_properties["MaxIterations"], 0);
  m_dblManager->setValue(m_properties["MaxIterations"], 500);

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

  connect(m_iqtFRangeManager, SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(propertyChanged(QtProperty *, double)));
  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(propertyChanged(QtProperty *, double)));

  m_properties["LinearBackground"] =
      m_grpManager->addProperty("LinearBackground");
  m_properties["BackgroundA0"] = m_dblManager->addProperty("A0");
  m_dblManager->setDecimals(m_properties["BackgroundA0"], NUM_DECIMALS);
  m_properties["LinearBackground"]->addSubProperty(
      m_properties["BackgroundA0"]);

  m_properties["Exponential1"] = createExponential("Exponential1");
  m_properties["Exponential2"] = createExponential("Exponential2");

  m_properties["StretchedExp"] = createStretchedExp("StretchedExp");

  m_dblManager->setMinimum(m_properties["BackgroundA0"], 0);
  m_dblManager->setMaximum(m_properties["BackgroundA0"], 1);

  m_dblManager->setMinimum(m_properties["Exponential1.Intensity"], 0);
  m_dblManager->setMaximum(m_properties["Exponential1.Intensity"], 1);

  m_dblManager->setMinimum(m_properties["Exponential2.Intensity"], 0);
  m_dblManager->setMaximum(m_properties["Exponential2.Intensity"], 1);

  m_dblManager->setMinimum(m_properties["StretchedExp.Intensity"], 0);
  m_dblManager->setMaximum(m_properties["StretchedExp.Intensity"], 1);

  typeSelection(m_uiForm.cbFitType->currentIndex());

  // Update guess curve on property change
  connect(m_dblManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(updateGuessPlot(QtProperty *)));

  // Signal/slot ui connections
  connect(m_uiForm.dsSampleInput, SIGNAL(dataReady(const QString &)), this,
          SLOT(newDataLoaded(const QString &)));
  connect(m_uiForm.cbFitType, SIGNAL(currentIndexChanged(int)), this,
          SLOT(typeSelection(int)));
  connect(m_uiForm.pbSingle, SIGNAL(clicked()), this, SLOT(singleFit()));

  connect(m_uiForm.dsSampleInput, SIGNAL(filesFound()), this,
          SLOT(updatePlot()));

  connect(m_uiForm.spPlotSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(setSelectedSpectra(int)));
  connect(m_uiForm.spPlotSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(updatePlot()));
  connect(m_uiForm.spPlotSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(updateProperties(int)));

  connect(m_uiForm.spSpectraMin, SIGNAL(valueChanged(int)), this,
          SLOT(specMinChanged(int)));
  connect(m_uiForm.spSpectraMax, SIGNAL(valueChanged(int)), this,
          SLOT(specMaxChanged(int)));
  connect(m_uiForm.ckPlotGuess, SIGNAL(toggled(bool)), this,
          SLOT(updateGuessPlot()));
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

  setMinimumSpectra(m_uiForm.spSpectraMin->value());
  setMaximumSpectra(m_uiForm.spSpectraMax->value());

  updateFitFunctions();
  IAlgorithm_sptr iqtFitAlg =
      iqtFitAlgorithm(inputWs, minimumSpectra(), maximumSpectra());

  m_batchAlgoRunner->addAlgorithm(iqtFitAlg);
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(algorithmComplete(bool)));
  m_batchAlgoRunner->executeBatchAsync();
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
  auto resultWs = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
      m_baseName + "_Result");
  if (!(m_plotOption.compare("None") == 0)) {
    if (m_plotOption.compare("All") == 0) {
      int specEnd = (int)resultWs->getNumberHistograms();
      for (int i = 0; i < specEnd; i++) {
        IndirectTab::plotSpectrum(QString::fromStdString(resultWs->getName()),
                                  i, i);
      }
    } else {
      int specNumber = m_uiForm.cbPlotType->currentIndex();
      IndirectTab::plotSpectrum(QString::fromStdString(resultWs->getName()),
                                specNumber, specNumber);
    }
  }
}

/**
 * Save the result of the algorithm
 */
void IqtFit::saveResult() {
  const auto workingdirectory =
      Mantid::Kernel::ConfigService::Instance().getString(
          "defaultsave.directory");
  const auto filepath = workingdirectory + m_baseName + "_Result.nxs";
  addSaveWorkspaceToQueue(QString::fromStdString(m_baseName + "_Result"),
                          QString::fromStdString(filepath));
  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Plots the current spectrum displayed in the preview plot
 */
void IqtFit::plotCurrentPreview() {
  auto inputWs = inputWorkspace();
  auto previewWs = previewPlotWorkspace();

  if (!inputWs || !previewWs) {
    return;
  }

  if (inputWs->getName().compare(previewWs->getName()) == 0) {
    // Plot only the sample curve
    const auto workspaceIndex = m_uiForm.spPlotSpectrum->value();
    IndirectTab::plotSpectrum(QString::fromStdString(previewWs->getName()),
                              workspaceIndex, workspaceIndex);
  } else {
    // Plot Sample, Fit and Diff curve
    IndirectTab::plotSpectrum(QString::fromStdString(previewWs->getName()), 0,
                              2);
  }
}

/**
 * Handles completion of the IqtFitMultiple algorithm.
 * @param error True if the algorithm was stopped due to error, false otherwise
 * @param outputWsName The name of the output workspace
 */
void IqtFit::algorithmComplete(bool error) {
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
             SLOT(algorithmComplete(bool)));

  if (error) {
    QString msg =
        "There was an error executing the fitting algorithm. Please see the "
        "Results Log pane for more details.";
    showMessageBox(msg);
    return;
  }

  m_parameterToProperty = createParameterToPropertyMap(m_fitFunctions);
  m_parameterValues = IndirectTab::extractParametersFromTable(
      m_baseName + "_Parameters", m_parameterToProperty.keys().toSet(),
      minimumSpectra(), maximumSpectra());
  updateProperties(m_uiForm.spPlotSpectrum->value());

  updatePlot();
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

  auto range = std::make_pair(m_iqtFRangeManager->value(m_properties["StartX"]),
                              m_iqtFRangeManager->value(m_properties["EndX"]));
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
  auto inputWs = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
      wsName.toStdString());
  setInputWorkspace(inputWs);
  setPreviewPlotWorkspace(inputWs);
  m_baseName.clear();
  m_parameterValues.clear();
  m_parameterToProperty.clear();
  m_fitFunctions.clear();

  int maxWsIndex = static_cast<int>(inputWs->getNumberHistograms()) - 1;

  m_uiForm.spPlotSpectrum->setMaximum(maxWsIndex);
  m_uiForm.spPlotSpectrum->setMinimum(0);
  m_uiForm.spPlotSpectrum->setValue(0);

  m_uiForm.spSpectraMin->setMaximum(maxWsIndex);
  m_uiForm.spSpectraMin->setMinimum(0);

  m_uiForm.spSpectraMax->setMaximum(maxWsIndex);
  m_uiForm.spSpectraMax->setMinimum(0);
  m_uiForm.spSpectraMax->setValue(maxWsIndex);

  m_dblManager->setValue(m_properties["BackgroundA0"], 1.0);

  setDefaultParameters("Exponential1");
  setDefaultParameters("Exponential2");
  setDefaultParameters("StretchedExp");

  updatePlot();
}

CompositeFunction_sptr IqtFit::createFunction(bool tie) {
  CompositeFunction_sptr result(new CompositeFunction);
  QString fname;
  const int fitType = m_uiForm.cbFitType->currentIndex();

  IFunction_sptr func =
      FunctionFactory::Instance().createFunction("LinearBackground");
  func->setParameter("A0", m_dblManager->value(m_properties["BackgroundA0"]));
  result->addFunction(func);
  result->tie("f0.A1", "0");
  if (tie) {
    result->tie("f0.A0",
                m_properties["BackgroundA0"]->valueText().toStdString());
  }

  if (fitType == 2) {
    fname = "StretchedExp";
  } else {
    fname = "Exponential1";
  }

  result->addFunction(createExponentialFunction(fname, tie));

  if (fitType == 1 || fitType == 3) {
    if (fitType == 1) {
      fname = "Exponential2";
    } else {
      fname = "StretchedExp";
    }
    result->addFunction(createExponentialFunction(fname, tie));
  }

  // Return CompositeFunction object to caller.
  result->applyTies();
  return result;
}

IFunction_sptr IqtFit::createExponentialFunction(const QString &name,
                                                 bool tie) {
  IFunction_sptr result;
  if (name.startsWith("Exp")) {
    IFunction_sptr result =
        FunctionFactory::Instance().createFunction("ExpDecay");
    result->setParameter(
        "Height", m_dblManager->value(m_properties[name + ".Intensity"]));
    result->setParameter("Lifetime",
                         m_dblManager->value(m_properties[name + ".Tau"]));
    if (tie) {
      result->tie("Height",
                  m_properties[name + ".Intensity"]->valueText().toStdString());
      result->tie("Lifetime",
                  m_properties[name + ".Tau"]->valueText().toStdString());
    }
    result->applyTies();
    return result;
  } else {
    IFunction_sptr result =
        FunctionFactory::Instance().createFunction("StretchExp");
    result->setParameter(
        "Height", m_dblManager->value(m_properties[name + ".Intensity"]));
    result->setParameter("Lifetime",
                         m_dblManager->value(m_properties[name + ".Tau"]));
    result->setParameter("Stretching",
                         m_dblManager->value(m_properties[name + ".Beta"]));
    if (tie) {
      result->tie("Height",
                  m_properties[name + ".Intensity"]->valueText().toStdString());
      result->tie("Lifetime",
                  m_properties[name + ".Tau"]->valueText().toStdString());
      result->tie("Stretching",
                  m_properties[name + ".Beta"]->valueText().toStdString());
    }
    result->applyTies();
    return result;
  }
}

QtProperty *IqtFit::createExponential(const QString &name) {
  QtProperty *expGroup = m_grpManager->addProperty(name);
  m_properties[name + ".Intensity"] = m_dblManager->addProperty("Intensity");
  m_dblManager->setDecimals(m_properties[name + ".Intensity"], NUM_DECIMALS);
  m_properties[name + ".Tau"] = m_dblManager->addProperty("Tau");
  m_dblManager->setDecimals(m_properties[name + ".Tau"], NUM_DECIMALS);
  expGroup->addSubProperty(m_properties[name + ".Intensity"]);
  expGroup->addSubProperty(m_properties[name + ".Tau"]);
  return expGroup;
}

QtProperty *IqtFit::createStretchedExp(const QString &name) {
  QtProperty *prop = m_grpManager->addProperty(name);
  m_properties[name + ".Intensity"] = m_dblManager->addProperty("Intensity");
  m_properties[name + ".Tau"] = m_dblManager->addProperty("Tau");
  m_properties[name + ".Beta"] = m_dblManager->addProperty("Beta");
  m_dblManager->setRange(m_properties[name + ".Beta"], 0, 1);
  m_dblManager->setDecimals(m_properties[name + ".Intensity"], NUM_DECIMALS);
  m_dblManager->setDecimals(m_properties[name + ".Tau"], NUM_DECIMALS);
  m_dblManager->setDecimals(m_properties[name + ".Beta"], NUM_DECIMALS);
  prop->addSubProperty(m_properties[name + ".Intensity"]);
  prop->addSubProperty(m_properties[name + ".Tau"]);
  prop->addSubProperty(m_properties[name + ".Beta"]);
  return prop;
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
  m_iqtFTree->clear();

  m_iqtFTree->addProperty(m_properties["StartX"]);
  m_iqtFTree->addProperty(m_properties["EndX"]);
  m_iqtFTree->addProperty(m_properties["MaxIterations"]);
  m_iqtFTree->addProperty(m_properties["LinearBackground"]);
  m_iqtFTree->addProperty(m_properties["FABADA"]);

  // option should only be available with a single stretched exponential
  m_uiForm.ckConstrainBeta->setEnabled((index == 2));
  if (!m_uiForm.ckConstrainBeta->isEnabled()) {
    m_uiForm.ckConstrainBeta->setChecked(false);
  }

  switch (index) {
  case 0:
    m_iqtFTree->addProperty(m_properties["Exponential1"]);

    // remove option to plot beta and add all
    m_uiForm.cbPlotType->removeItem(4);
    m_uiForm.cbPlotType->removeItem(3);
    m_uiForm.cbPlotType->addItem("All");
    break;
  case 1:
    m_iqtFTree->addProperty(m_properties["Exponential1"]);
    m_iqtFTree->addProperty(m_properties["Exponential2"]);

    // remove option to plot beta and add all
    m_uiForm.cbPlotType->removeItem(4);
    m_uiForm.cbPlotType->removeItem(3);

    m_uiForm.cbPlotType->addItem("All");
    break;
  case 2:
    m_iqtFTree->addProperty(m_properties["StretchedExp"]);

    // add option to plot beta and all
    m_uiForm.cbPlotType->removeItem(4);
    m_uiForm.cbPlotType->removeItem(3);
    m_uiForm.cbPlotType->addItem("Beta");
    m_uiForm.cbPlotType->addItem("All");

    break;
  case 3:
    m_iqtFTree->addProperty(m_properties["Exponential1"]);
    m_iqtFTree->addProperty(m_properties["StretchedExp"]);

    // add option to plot beta and all
    m_uiForm.cbPlotType->removeItem(4);
    m_uiForm.cbPlotType->removeItem(3);
    m_uiForm.cbPlotType->addItem("Beta");
    m_uiForm.cbPlotType->addItem("All");

    break;
  }
  const auto optionIndex =
      m_uiForm.cbPlotType->findText(QString::fromStdString(m_plotOption));
  if (optionIndex != -1) {
    m_uiForm.cbPlotType->setCurrentIndex(optionIndex);
  } else {
    m_uiForm.cbPlotType->setCurrentIndex(0);
  }

  updateGuessPlot();
  m_uiForm.ppPlotTop->removeSpectrum("Fit");
  m_uiForm.ppPlotTop->removeSpectrum("Diff");
  connect(m_uiForm.cbPlotType, SIGNAL(currentIndexChanged(QString)), this,
          SLOT(updateCurrentPlotOption(QString)));
}

/**
 * Update the current plot option selected
 */
void IqtFit::updateCurrentPlotOption(QString newOption) {
  m_plotOption = newOption.toStdString();
}

void IqtFit::updatePlot() {
  // If there is a result workspace plot then plot it
  const auto groupName = m_baseName + "_Workspaces";
  IndirectDataAnalysisTab::updatePlot(groupName, m_uiForm.ppPlotTop,
                                      m_uiForm.ppPlotBottom);

  IndirectDataAnalysisTab::updatePlotRange("IqtFitRange", m_uiForm.ppPlotTop);
  resizePlotRange(m_uiForm.ppPlotTop);
  updateGuessPlot();
}

void IqtFit::resizePlotRange(MantidQt::MantidWidgets::PreviewPlot *preview) {
  preview->resizeX();
  preview->setAxisRange(qMakePair(0.0, 1.0), QwtPlot::yLeft);
}

void IqtFit::setDefaultParameters(const QString &name) {
  double background = m_dblManager->value(m_properties["BackgroundA0"]);
  // intensity is always 1-background
  m_dblManager->setValue(m_properties[name + ".Intensity"], 1.0 - background);

  auto inputWs = inputWorkspace();
  double tau = 0;

  if (inputWs) {
    auto x = inputWs->x(0);
    auto y = inputWs->y(0);

    if (x.size() > 4) {
      tau = -x[4] / log(y[4]);
    }
  }

  m_dblManager->setValue(m_properties[name + ".Tau"], tau);
  m_dblManager->setValue(m_properties[name + ".Beta"], 1.0);
}

QHash<QString, QString>
IqtFit::createParameterToPropertyMap(const QVector<QString> &functionNames) {
  QHash<QString, QString> parameterToProperty;

  for (int i = 0; i < functionNames.size(); ++i) {
    QString prefix = "f" + QString::number(i + 1) + ".";
    extendParameterToPropertyMap(functionNames[i], prefix, parameterToProperty);
  }

  parameterToProperty["f0.A0"] = "BackgroundA0";
  return parameterToProperty;
}

void IqtFit::extendParameterToPropertyMap(
    const QString &functionName, const QString &prefix,
    QHash<QString, QString> &parameterToProperty) {
  bool isExponential = boost::starts_with(functionName, "Exponential") ||
                       functionName == "StretchedExp";

  if (isExponential) {
    QString intensityName = prefix + "Height";
    parameterToProperty[intensityName] = functionName + ".Intensity";
    QString tauName = prefix + "Lifetime";
    parameterToProperty[tauName] = functionName + ".Tau";

    if (functionName == "StretchedExp") {
      QString betaName = prefix + "Stretching";
      parameterToProperty[betaName] = functionName + ".Beta";
    }
  }
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
  m_iqtFRangeManager->setValue(m_properties["StartX"], val);
}

void IqtFit::xMaxSelected(double val) {
  m_iqtFRangeManager->setValue(m_properties["EndX"], val);
}

void IqtFit::backgroundSelected(double val) {
  m_dblManager->setValue(m_properties["BackgroundA0"], val);
  m_dblManager->setValue(m_properties["Exponential1.Intensity"], 1.0 - val);
  m_dblManager->setValue(m_properties["Exponential2.Intensity"], 1.0 - val);
  m_dblManager->setValue(m_properties["StretchedExp.Intensity"], 1.0 - val);
}

void IqtFit::propertyChanged(QtProperty *prop, double val) {
  auto fitRangeSelector = m_uiForm.ppPlotTop->getRangeSelector("IqtFitRange");
  auto backgroundRangeSelector =
      m_uiForm.ppPlotTop->getRangeSelector("IqtFitBackground");
  auto specNo = m_uiForm.spPlotSpectrum->value();
  bool autoUpdate = specNo > maximumSpectra() || specNo < minimumSpectra() ||
                    m_parameterValues[prop->propertyName()].isEmpty();

  if (prop == m_properties["StartX"]) {
    fitRangeSelector->setMinimum(val);
  } else if (prop == m_properties["EndX"]) {
    fitRangeSelector->setMaximum(val);
  } else if (autoUpdate && prop == m_properties["BackgroundA0"]) {
    backgroundRangeSelector->setMinimum(val);
    m_dblManager->setValue(m_properties["Exponential1.Intensity"], 1.0 - val);
    m_dblManager->setValue(m_properties["Exponential2.Intensity"], 1.0 - val);
    m_dblManager->setValue(m_properties["StretchedExp.Intensity"], 1.0 - val);
  } else if (autoUpdate && (prop == m_properties["Exponential1.Intensity"] ||
                            prop == m_properties["Exponential2.Intensity"] ||
                            prop == m_properties["StretchedExp.Intensity"])) {
    backgroundRangeSelector->setMinimum(1.0 - val);
    m_dblManager->setValue(m_properties["Exponential1.Intensity"], val);
    m_dblManager->setValue(m_properties["Exponential2.Intensity"], val);
    m_dblManager->setValue(m_properties["StretchedExp.Intensity"], val);
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
             SLOT(plotGuess(QtProperty *)));

  updateFitFunctions();
  size_t specNo = m_uiForm.spPlotSpectrum->text().toULongLong();
  m_singleFitAlg = iqtFitAlgorithm(inputWorkspace(), specNo, specNo);

  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(singleFitComplete(bool)));

  m_batchAlgoRunner->addAlgorithm(m_singleFitAlg);
  m_batchAlgoRunner->executeBatchAsync();
}

void IqtFit::singleFitComplete(bool error) {
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
             SLOT(singleFitComplete(bool)));

  algorithmComplete(error);

  // Can start updating the guess curve again
  connect(m_dblManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(plotGuess(QtProperty *)));
}

void IqtFit::updateGuessPlot() {
  // Do nothing if there is no sample data curve
  if (!m_uiForm.ppPlotTop->hasCurve("Sample"))
    return;

  // Don't plot guess if plot guess is unchecked
  if (!m_uiForm.ckPlotGuess->isChecked()) {
    m_uiForm.ppPlotTop->removeSpectrum("Guess");
  } else {
    plotGuess(NULL);
  }
}

void IqtFit::plotGuess(QtProperty *) {

  if (m_uiForm.ckPlotGuess->isChecked())
    IndirectDataAnalysisTab::plotGuess(m_uiForm.ppPlotTop, createFunction(true));
  else
    m_uiForm.ppPlotTop->removeSpectrum("Guess");
}

/*
 * Updates the properties in the property table using the stored parameter
 * values for the specified spectrum.
 *
 * @param specNo  The index of the parameter values to update the properties in
 *                the property table with.
 */
void IqtFit::updateProperties(int specNo) {
  size_t index = boost::numeric_cast<size_t>(specNo);
  auto parameterNames = m_parameterValues.keys();

  // Check whether parameter values exist for the specified spectrum number
  if (m_parameterValues[parameterNames[0]].contains(index)) {

    for (auto &paramName : parameterNames) {
      auto propertyName = m_parameterToProperty[paramName];
      m_dblManager->setValue(m_properties[propertyName],
                             m_parameterValues[paramName][index]);
    }

  } else {
    setDefaultParameters("Exponential1");
    setDefaultParameters("Exponential2");
    setDefaultParameters("StretchedExp");
  }
}

/*
 * Updates a vector storing which functions were last used in a fitting.
 */
void IqtFit::updateFitFunctions() {
  const int fitType = m_uiForm.cbFitType->currentIndex();

  if (fitType == 0)
    m_fitFunctions = {"Exponential1"};
  else if (fitType == 1)
    m_fitFunctions = {"Exponential1", "Exponential2"};
  else if (fitType == 2)
    m_fitFunctions = {"StretchedExp"};
  else if (fitType == 3)
    m_fitFunctions = {"Exponential1", "StretchedExp"};
}

void IqtFit::fitContextMenu(const QPoint &) {
  QtBrowserItem *item(nullptr);

  item = m_iqtFTree->currentItem();

  if (!item)
    return;

  // is it a fit property ?
  QtProperty *prop = item->property();

  // is it already fixed?
  bool fixed = prop->propertyManager() != m_dblManager;

  if (fixed && prop->propertyManager() != m_stringManager)
    return;

  // Create the menu
  QMenu *menu = new QMenu("IqtFit", m_iqtFTree);
  QAction *action;

  if (!fixed) {
    action = new QAction("Fix", m_parentWidget);
    connect(action, SIGNAL(triggered()), this, SLOT(fixItem()));
  } else {
    action = new QAction("Remove Fix", m_parentWidget);
    connect(action, SIGNAL(triggered()), this, SLOT(unFixItem()));
  }

  menu->addAction(action);

  // Show the menu
  menu->popup(QCursor::pos());
}

void IqtFit::fixItem() {
  QtBrowserItem *item = m_iqtFTree->currentItem();

  // Determine what the property is.
  QtProperty *prop = item->property();

  QtProperty *fixedProp = m_stringManager->addProperty(prop->propertyName());
  QtProperty *fprlbl = m_stringManager->addProperty("Fixed");
  fixedProp->addSubProperty(fprlbl);
  m_stringManager->setValue(fixedProp, prop->valueText());

  item->parent()->property()->addSubProperty(fixedProp);
  m_fixedProps[fixedProp] = prop;
  item->parent()->property()->removeSubProperty(prop);
}

void IqtFit::unFixItem() {
  QtBrowserItem *item = m_iqtFTree->currentItem();

  QtProperty *prop = item->property();
  if (prop->subProperties().empty()) {
    item = item->parent();
    prop = item->property();
  }

  item->parent()->property()->addSubProperty(m_fixedProps[prop]);
  item->parent()->property()->removeSubProperty(prop);
  m_fixedProps.remove(prop);
  QtProperty *proplbl = prop->subProperties()[0];
  delete proplbl;
  delete prop;
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

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
