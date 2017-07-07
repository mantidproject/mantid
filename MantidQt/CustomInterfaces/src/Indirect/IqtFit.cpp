#include "MantidQtCustomInterfaces/Indirect/IqtFit.h"

#include "MantidQtCustomInterfaces/UserInputValidator.h"
#include "MantidQtMantidWidgets/RangeSelector.h"

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
    : IndirectDataAnalysisTab(parent), m_stringManager(NULL), m_iqtFTree(NULL),
      m_iqtFRangeManager(NULL), m_fixedProps(), m_iqtFInputWS(),
      m_iqtFOutputWS(), m_iqtFInputWSName(), m_ties() {
  m_uiForm.setupUi(parent);
}

void IqtFit::setup() {
  m_stringManager = new QtStringPropertyManager(m_parentWidget);

  m_iqtFTree = new QtTreePropertyBrowser(m_parentWidget);
  m_uiForm.properties->addWidget(m_iqtFTree);

  auto fitRangeSelector = m_uiForm.ppPlot->addRangeSelector("IqtFitRange");
  connect(fitRangeSelector, SIGNAL(minValueChanged(double)), this,
          SLOT(xMinSelected(double)));
  connect(fitRangeSelector, SIGNAL(maxValueChanged(double)), this,
          SLOT(xMaxSelected(double)));

  auto backgroundRangeSelector = m_uiForm.ppPlot->addRangeSelector(
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
  m_properties["BackgroundA0"] = m_iqtFRangeManager->addProperty("A0");
  m_iqtFRangeManager->setDecimals(m_properties["BackgroundA0"], NUM_DECIMALS);
  m_properties["LinearBackground"]->addSubProperty(
      m_properties["BackgroundA0"]);

  m_properties["Exponential1"] = createExponential("Exponential1");
  m_properties["Exponential2"] = createExponential("Exponential2");

  m_properties["StretchedExp"] = createStretchedExp("StretchedExp");

  m_iqtFRangeManager->setMinimum(m_properties["BackgroundA0"], 0);
  m_iqtFRangeManager->setMaximum(m_properties["BackgroundA0"], 1);

  m_dblManager->setMinimum(m_properties["Exponential1.Intensity"], 0);
  m_dblManager->setMaximum(m_properties["Exponential1.Intensity"], 1);

  m_dblManager->setMinimum(m_properties["Exponential2.Intensity"], 0);
  m_dblManager->setMaximum(m_properties["Exponential2.Intensity"], 1);

  m_dblManager->setMinimum(m_properties["StretchedExp.Intensity"], 0);
  m_dblManager->setMaximum(m_properties["StretchedExp.Intensity"], 1);

  typeSelection(m_uiForm.cbFitType->currentIndex());

  // Update guess curve on property change
  connect(m_dblManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(plotGuess(QtProperty *)));

  // Signal/slot ui connections
  connect(m_uiForm.dsSampleInput, SIGNAL(dataReady(const QString &)), this,
          SLOT(newDataLoaded(const QString &)));
  connect(m_uiForm.cbFitType, SIGNAL(currentIndexChanged(int)), this,
          SLOT(typeSelection(int)));
  connect(m_uiForm.pbSingle, SIGNAL(clicked()), this, SLOT(singleFit()));

  connect(m_uiForm.dsSampleInput, SIGNAL(filesFound()), this,
          SLOT(updatePlot()));

  connect(m_uiForm.spPlotSpectrum, SIGNAL(valueChanged(int)), this,
          SLOT(updatePlot()));

  connect(m_uiForm.spSpectraMin, SIGNAL(valueChanged(int)), this,
          SLOT(specMinChanged(int)));
  connect(m_uiForm.spSpectraMax, SIGNAL(valueChanged(int)), this,
          SLOT(specMaxChanged(int)));
  connect(m_uiForm.ckPlotGuess, SIGNAL(toggled(bool)), this,
          SLOT(plotGuessChanged(bool)));
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
}

void IqtFit::run() {
  if (m_iqtFInputWS == NULL) {
    return;
  }

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
  const long specMin = m_uiForm.spSpectraMin->value();
  const long specMax = m_uiForm.spSpectraMax->value();
  const auto minimizer = minimizerString("$outputname_$wsindex");
  m_plotOption = m_uiForm.cbPlotType->currentText().toStdString();
  const auto startX = boost::lexical_cast<double>(
      m_properties["StartX"]->valueText().toStdString());
  const auto endX = boost::lexical_cast<double>(
      m_properties["EndX"]->valueText().toStdString());
  const auto maxIt = boost::lexical_cast<long>(
      m_properties["MaxIterations"]->valueText().toStdString());

  if (!constrainBeta) {
    m_baseName = constructBaseName(m_iqtFInputWSName.toStdString(), fitType,
                                   false, specMin, specMax);
    auto iqtFitSequential =
        AlgorithmManager::Instance().create("IqtFitSequential");
    iqtFitSequential->initialize();
    iqtFitSequential->setProperty("InputWorkspace", m_iqtFInputWS);
    iqtFitSequential->setProperty("Function", function);
    iqtFitSequential->setProperty("FitType", fitType);
    iqtFitSequential->setProperty("StartX", startX);
    iqtFitSequential->setProperty("EndX", endX);
    iqtFitSequential->setProperty("SpecMin", specMin);
    iqtFitSequential->setProperty("SpecMax", specMax);
    iqtFitSequential->setProperty("Minimizer", minimizer.toStdString());
    iqtFitSequential->setProperty("MaxIterations", maxIt);
    iqtFitSequential->setProperty("ConstrainIntensities", constrainIntens);
    iqtFitSequential->setProperty("OutputResultWorkspace",
                                  m_baseName + "_Result");
    iqtFitSequential->setProperty("OutputParameterWorkspace",
                                  m_baseName + "_Parameters");
    iqtFitSequential->setProperty("OutputWorkspaceGroup",
                                  m_baseName + "_Workspaces");
    m_pythonExportWsName = (m_baseName + "_Workspaces");
    m_batchAlgoRunner->addAlgorithm(iqtFitSequential);
    connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
            SLOT(algorithmComplete(bool)));
    m_batchAlgoRunner->executeBatchAsync();

  } else {
    m_baseName = constructBaseName(m_iqtFInputWSName.toStdString(), fitType,
                                   true, specMin, specMax);
    auto iqtFitMultiple = AlgorithmManager::Instance().create("IqtFitMultiple");
    iqtFitMultiple->initialize();
    iqtFitMultiple->setProperty("InputWorkspace",
                                m_iqtFInputWSName.toStdString());
    iqtFitMultiple->setProperty("Function", function);
    iqtFitMultiple->setProperty("FitType", fitType);
    iqtFitMultiple->setProperty("StartX", startX);
    iqtFitMultiple->setProperty("EndX", endX);
    iqtFitMultiple->setProperty("SpecMin", specMin);
    iqtFitMultiple->setProperty("SpecMax", specMax);
    iqtFitMultiple->setProperty("Minimizer", minimizer.toStdString());
    iqtFitMultiple->setProperty("MaxIterations", maxIt);
    iqtFitMultiple->setProperty("ConstrainIntensities", constrainIntens);
    iqtFitMultiple->setProperty("OutputResultWorkspace",
                                m_baseName + "_Result");
    iqtFitMultiple->setProperty("OutputParameterWorkspace",
                                m_baseName + "_Parameters");
    iqtFitMultiple->setProperty("OutputWorkspaceGroup",
                                m_baseName + "_Workspaces");
    m_pythonExportWsName = (m_baseName + "_Workspaces");
    m_batchAlgoRunner->addAlgorithm(iqtFitMultiple);
    connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
            SLOT(algorithmComplete(bool)));
    m_batchAlgoRunner->executeBatchAsync();
  }
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
* Handles completion of the IqtFitMultiple algorithm.
* @param error True if the algorithm was stopped due to error, false otherwise
*/
void IqtFit::algorithmComplete(bool error) {
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
             SLOT(algorithmComplete(bool)));
  if (error)
    return;
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
                                      const bool &multi, const long &specMin,
                                      const long &specMax) {
  QString functionType = QString::fromStdString(fitType);
  if (multi) {
    functionType = "1Smult_s";
  }

  QString baseName = QString::fromStdString(inputName);
  baseName = baseName.left(baseName.lastIndexOf("_"));
  baseName += "_IqtFit_";
  baseName += functionType;
  baseName += QString::number(specMin);
  baseName += "_to_";
  baseName += QString::number(specMax);
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
  m_iqtFInputWSName = wsName;
  m_iqtFInputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
      m_iqtFInputWSName.toStdString());

  int maxWsIndex = static_cast<int>(m_iqtFInputWS->getNumberHistograms()) - 1;

  m_uiForm.spPlotSpectrum->setMaximum(maxWsIndex);
  m_uiForm.spPlotSpectrum->setMinimum(0);
  m_uiForm.spPlotSpectrum->setValue(0);

  m_uiForm.spSpectraMin->setMaximum(maxWsIndex);
  m_uiForm.spSpectraMin->setMinimum(0);

  m_uiForm.spSpectraMax->setMaximum(maxWsIndex);
  m_uiForm.spSpectraMax->setMinimum(0);
  m_uiForm.spSpectraMax->setValue(maxWsIndex);

  updatePlot();
}

CompositeFunction_sptr IqtFit::createFunction(bool tie) {
  CompositeFunction_sptr result(new CompositeFunction);
  QString fname;
  const int fitType = m_uiForm.cbFitType->currentIndex();

  IFunction_sptr func =
      FunctionFactory::Instance().createFunction("LinearBackground");
  func->setParameter("A0",
                     m_iqtFRangeManager->value(m_properties["BackgroundA0"]));
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

  plotGuess(NULL);
  m_uiForm.ppPlot->removeSpectrum("Fit");
  m_uiForm.ppPlot->removeSpectrum("Diff");
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
  if (!m_iqtFInputWS) {
    g_log.error("No workspace loaded, cannot create preview plot.");
    return;
  }

  int specNo = m_uiForm.spPlotSpectrum->value();

  m_uiForm.ppPlot->clear();
  m_uiForm.ppPlot->addSpectrum("Sample", m_iqtFInputWS, specNo);

  try {
    const QPair<double, double> curveRange =
        m_uiForm.ppPlot->getCurveRange("Sample");
    const std::pair<double, double> range(curveRange.first, curveRange.second);
    m_uiForm.ppPlot->getRangeSelector("IqtFitRange")
        ->setRange(range.first, range.second);
    m_iqtFRangeManager->setRange(m_properties["StartX"], range.first,
                                 range.second);
    m_iqtFRangeManager->setRange(m_properties["EndX"], range.first,
                                 range.second);

    setDefaultParameters("Exponential1");
    setDefaultParameters("Exponential2");
    setDefaultParameters("StretchedExp");

    m_uiForm.ppPlot->resizeX();
    m_uiForm.ppPlot->setAxisRange(qMakePair(0.0, 1.0), QwtPlot::yLeft);
  } catch (std::invalid_argument &exc) {
    showMessageBox(exc.what());
  }

  // If there is a result plot then plot it
  if (AnalysisDataService::Instance().doesExist(m_pythonExportWsName)) {
    WorkspaceGroup_sptr outputGroup =
        AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
            m_pythonExportWsName);
    if (specNo >= static_cast<int>(outputGroup->size()))
      return;
    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        outputGroup->getItem(specNo));
    if (ws) {
      if (m_uiForm.ckPlotGuess->isChecked()) {
        m_uiForm.ckPlotGuess->setChecked(false);
      }
      m_uiForm.ppPlot->addSpectrum("Fit", ws, 1, Qt::red);
      m_uiForm.ppPlot->addSpectrum("Diff", ws, 2, Qt::blue);
    }
  }
}

void IqtFit::setDefaultParameters(const QString &name) {
  double background = m_dblManager->value(m_properties["BackgroundA0"]);
  // intensity is always 1-background
  m_dblManager->setValue(m_properties[name + ".Intensity"], 1.0 - background);
  auto x = m_iqtFInputWS->x(0);
  auto y = m_iqtFInputWS->y(0);
  double tau = 0;

  if (x.size() > 4) {
    tau = -x[4] / log(y[4]);
  }

  m_dblManager->setValue(m_properties[name + ".Tau"], tau);
  m_dblManager->setValue(m_properties[name + ".Beta"], 1.0);
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
  m_iqtFRangeManager->setValue(m_properties["BackgroundA0"], val);
  m_dblManager->setValue(m_properties["Exponential1.Intensity"], 1.0 - val);
  m_dblManager->setValue(m_properties["Exponential2.Intensity"], 1.0 - val);
  m_dblManager->setValue(m_properties["StretchedExp.Intensity"], 1.0 - val);
}

void IqtFit::propertyChanged(QtProperty *prop, double val) {
  auto fitRangeSelector = m_uiForm.ppPlot->getRangeSelector("IqtFitRange");
  auto backgroundRangeSelector =
      m_uiForm.ppPlot->getRangeSelector("IqtFitBackground");

  if (prop == m_properties["StartX"]) {
    fitRangeSelector->setMinimum(val);
  } else if (prop == m_properties["EndX"]) {
    fitRangeSelector->setMaximum(val);
  } else if (prop == m_properties["BackgroundA0"]) {
    backgroundRangeSelector->setMinimum(val);
    m_dblManager->setValue(m_properties["Exponential1.Intensity"], 1.0 - val);
    m_dblManager->setValue(m_properties["Exponential2.Intensity"], 1.0 - val);
    m_dblManager->setValue(m_properties["StretchedExp.Intensity"], 1.0 - val);
  } else if (prop == m_properties["Exponential1.Intensity"] ||
             prop == m_properties["Exponential2.Intensity"] ||
             prop == m_properties["StretchedExp.Intensity"]) {
    backgroundRangeSelector->setMinimum(1.0 - val);
    m_dblManager->setValue(m_properties["Exponential1.Intensity"], val);
    m_dblManager->setValue(m_properties["Exponential2.Intensity"], val);
    m_dblManager->setValue(m_properties["StretchedExp.Intensity"], val);
  }
}

void IqtFit::plotGuessChanged(bool checked) {
  if (!checked) {
    m_uiForm.ppPlot->removeSpectrum("Guess");
  } else {
    m_uiForm.ppPlot->removeSpectrum("Fit");
    m_uiForm.ppPlot->removeSpectrum("Diff");
    plotGuess(NULL);
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

  // First create the function
  auto function = createFunction();

  const int fitType = m_uiForm.cbFitType->currentIndex();
  if (m_uiForm.ckConstrainIntensities->isChecked()) {
    switch (fitType) {
    case 0: // 1 Exp
    case 2: // 1 Str
      m_ties = "f1.Height = 1-f0.A0";
      break;
    case 1: // 2 Exp
    case 3: // 1 Exp & 1 Str
      m_ties = "f1.Height=1-f2.Height-f0.A0";
      break;
    default:
      break;
    }
  }
  QString ftype = fitTypeString();

  updatePlot();
  if (m_iqtFInputWS == NULL) {
    return;
  }

  QString pyInput =
      "from IndirectCommon import getWSprefix\nprint getWSprefix('%1')\n";
  pyInput = pyInput.arg(m_iqtFInputWSName);
  m_singleFitOutputName = runPythonCode(pyInput).trimmed() + QString("iqt_") +
                          ftype + m_uiForm.spPlotSpectrum->text();

  // Create the Fit Algorithm
  m_singleFitAlg = AlgorithmManager::Instance().create("Fit");
  m_singleFitAlg->initialize();
  m_singleFitAlg->setPropertyValue("Function", function->asString());
  m_singleFitAlg->setPropertyValue("InputWorkspace",
                                   m_iqtFInputWSName.toStdString());
  m_singleFitAlg->setProperty("WorkspaceIndex",
                              m_uiForm.spPlotSpectrum->text().toInt());
  m_singleFitAlg->setProperty(
      "StartX", m_iqtFRangeManager->value(m_properties["StartX"]));
  m_singleFitAlg->setProperty("EndX",
                              m_iqtFRangeManager->value(m_properties["EndX"]));
  m_singleFitAlg->setProperty(
      "MaxIterations",
      static_cast<int>(m_dblManager->value(m_properties["MaxIterations"])));
  m_singleFitAlg->setProperty(
      "Minimizer", minimizerString(m_singleFitOutputName).toStdString());
  m_singleFitAlg->setProperty("Ties", m_ties.toStdString());
  m_singleFitAlg->setPropertyValue("Output",
                                   m_singleFitOutputName.toStdString());

  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(singleFitComplete(bool)));

  m_batchAlgoRunner->addAlgorithm(m_singleFitAlg);
  m_batchAlgoRunner->executeBatchAsync();
}

void IqtFit::singleFitComplete(bool error) {
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
             SLOT(singleFitComplete(bool)));

  if (error) {
    QString msg =
        "There was an error executing the fitting algorithm. Please see the "
        "Results Log pane for more details.";
    showMessageBox(msg);
    return;
  }

  IFunction_sptr outputFunc = m_singleFitAlg->getProperty("Function");

  // Get params.
  QMap<QString, double> parameters;
  std::vector<std::string> parNames = outputFunc->getParameterNames();
  std::vector<double> parVals;

  for (size_t i = 0; i < parNames.size(); ++i)
    parVals.push_back(outputFunc->getParameter(parNames[i]));

  for (size_t i = 0; i < parNames.size(); ++i)
    parameters[QString(parNames[i].c_str())] = parVals[i];

  m_iqtFRangeManager->setValue(m_properties["BackgroundA0"],
                               parameters["f0.A0"]);

  const int fitType = m_uiForm.cbFitType->currentIndex();
  if (fitType != 2) {
    // Exp 1
    m_dblManager->setValue(m_properties["Exponential1.Intensity"],
                           parameters["f1.Height"]);
    m_dblManager->setValue(m_properties["Exponential1.Tau"],
                           parameters["f1.Lifetime"]);

    if (fitType == 1) {
      // Exp 2
      m_dblManager->setValue(m_properties["Exponential2.Intensity"],
                             parameters["f2.Height"]);
      m_dblManager->setValue(m_properties["Exponential2.Tau"],
                             parameters["f2.Lifetime"]);
    }
  }

  if (fitType > 1) {
    // Str
    QString fval;
    if (fitType == 2) {
      fval = "f1.";
    } else {
      fval = "f2.";
    }

    m_dblManager->setValue(m_properties["StretchedExp.Intensity"],
                           parameters[fval + "Height"]);
    m_dblManager->setValue(m_properties["StretchedExp.Tau"],
                           parameters[fval + "Lifetime"]);
    m_dblManager->setValue(m_properties["StretchedExp.Beta"],
                           parameters[fval + "Stretching"]);
  }

  // Can start updating the guess curve again
  connect(m_dblManager, SIGNAL(propertyChanged(QtProperty *)), this,
          SLOT(plotGuess(QtProperty *)));

  // Plot the guess first so that it is under the fit
  plotGuess(NULL);
  // Now show the fitted curve of the mini plot
  m_uiForm.ppPlot->addSpectrum("Fit", m_singleFitOutputName + "_Workspace", 1,
                               Qt::red);

  m_pythonExportWsName = "";
}

void IqtFit::plotGuess(QtProperty *) {
  // Do nothing if there is no sample data curve
  if (!m_uiForm.ppPlot->hasCurve("Sample"))
    return;

  CompositeFunction_sptr function = createFunction(true);

  // Create the double* array from the input workspace
  const size_t binIndxLow = m_iqtFInputWS->binIndexOf(
      m_iqtFRangeManager->value(m_properties["StartX"]));
  const size_t binIndxHigh = m_iqtFInputWS->binIndexOf(
      m_iqtFRangeManager->value(m_properties["EndX"]));
  const size_t nData = binIndxHigh - binIndxLow;

  const auto &xPoints = m_iqtFInputWS->points(0);

  std::vector<double> dataX(nData);
  std::copy(&xPoints[binIndxLow], &xPoints[binIndxLow + nData], dataX.begin());

  FunctionDomain1DVector domain(dataX);
  FunctionValues outputData(domain);
  function->function(domain, outputData);

  std::vector<double> dataY(nData);
  for (size_t i = 0; i < nData; i++) {
    dataY[i] = outputData.getCalculated(i);
  }

  IAlgorithm_sptr createWsAlg =
      AlgorithmManager::Instance().create("CreateWorkspace");
  createWsAlg->initialize();
  createWsAlg->setChild(true);
  createWsAlg->setLogging(false);
  createWsAlg->setProperty("OutputWorkspace", "__GuessAnon");
  createWsAlg->setProperty("NSpec", 1);
  createWsAlg->setProperty("DataX", dataX);
  createWsAlg->setProperty("DataY", dataY);
  createWsAlg->execute();
  MatrixWorkspace_sptr guessWs = createWsAlg->getProperty("OutputWorkspace");

  m_uiForm.ppPlot->addSpectrum("Guess", guessWs, 0, Qt::green);
}

void IqtFit::fitContextMenu(const QPoint &) {
  QtBrowserItem *item(NULL);

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

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
