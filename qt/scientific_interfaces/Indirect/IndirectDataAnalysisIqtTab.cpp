// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectDataAnalysisIqtTab.h"

#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidQtWidgets/Common/SignalBlocker.h"
#include "MantidQtWidgets/Plotting/RangeSelector.h"

#include "MantidQtWidgets/Common/UserInputValidator.h"

#include <tuple>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace MantidQt::CustomInterfaces;

namespace {
Mantid::Kernel::Logger g_log("Iqt");

MatrixWorkspace_sptr getADSMatrixWorkspace(std::string const &workspaceName) {
  return AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
      workspaceName);
}

std::string checkInstrumentParametersMatch(
    const Instrument_const_sptr &sampleInstrument,
    const Instrument_const_sptr &resolutionInstrument,
    std::string const &parameter) {
  if (!sampleInstrument->hasParameter(parameter))
    return "Could not find the " + parameter + " for the sample workspace.";
  if (!resolutionInstrument->hasParameter(parameter))
    return "Could not find the " + parameter +
           " for the resolution workspaces.";
  if (sampleInstrument->getStringParameter(parameter)[0] !=
      resolutionInstrument->getStringParameter(parameter)[0])
    return "The sample and resolution must have matching " + parameter + "s.";
  return "";
}

std::string
checkParametersMatch(const MatrixWorkspace_const_sptr &sampleWorkspace,
                     const MatrixWorkspace_const_sptr &resolutionWorkspace,
                     std::string const &parameter) {
  auto const sampleInstrument = sampleWorkspace->getInstrument();
  auto const resolutionInstrument = resolutionWorkspace->getInstrument();
  return checkInstrumentParametersMatch(sampleInstrument, resolutionInstrument,
                                        parameter);
}

std::string checkParametersMatch(std::string const &sampleName,
                                 std::string const &resolutionName,
                                 std::string const &parameter) {
  auto const sampleWorkspace = getADSMatrixWorkspace(sampleName);
  auto const resolutionWorkspace = getADSMatrixWorkspace(resolutionName);
  return checkParametersMatch(sampleWorkspace, resolutionWorkspace, parameter);
}

std::string
checkInstrumentsMatch(const MatrixWorkspace_const_sptr &sampleWorkspace,
                      const MatrixWorkspace_const_sptr &resolutionWorkspace) {
  auto const sampleInstrument = sampleWorkspace->getInstrument();
  auto const resolutionInstrument = resolutionWorkspace->getInstrument();
  if (sampleInstrument->getName() != resolutionInstrument->getName())
    return "The sample and resolution must have matching instruments.";
  return "";
}

std::string validateNumberOfHistograms(
    const MatrixWorkspace_const_sptr &sampleWorkspace,
    const MatrixWorkspace_const_sptr &resolutionWorkspace) {
  auto const sampleSize = sampleWorkspace->getNumberHistograms();
  auto const resolutionSize = resolutionWorkspace->getNumberHistograms();
  if (resolutionSize > 1 && sampleSize != resolutionSize)
    return "Resolution must have either one or as many spectra as the sample.";
  return "";
}

void addErrorMessage(UserInputValidator &uiv, std::string const &message) {
  if (!message.empty())
    uiv.addErrorMessage(QString::fromStdString(message) + "\n");
}

bool isTechniqueDirect(const MatrixWorkspace_const_sptr &sampleWorkspace,
                       const MatrixWorkspace_const_sptr &resWorkspace) {
  try {
    auto const logValue1 = sampleWorkspace->getLog("deltaE-mode")->value();
    auto const logValue2 = resWorkspace->getLog("deltaE-mode")->value();
    return (logValue1 == "Direct") && (logValue2 == "Direct");
  } catch (std::exception const &) {
    return false;
  }
}

/**
 * Calculate the number of bins in the sample & resolution workspaces
 * @param wsName The sample workspace name
 * @param resName the resolution woskapce name
 * @param energyMin Minimum energy for chosen bin range
 * @param energyMax Maximum energy for chosen bin range
 * @param binReductionFactor The factor by which to reduce the number of bins
 * @return A 4-tuple where the first entry denotes whether the
 * calculation was successful or not. The final 3 values
 * are EWidth, SampleBins, ResolutionBins if the calculation succeeded,
 * otherwise they are undefined.
 */
std::tuple<bool, float, int, int>
calculateBinParameters(std::string const &wsName, std::string const &resName,
                       double energyMin, double energyMax,
                       double binReductionFactor) {
  ITableWorkspace_sptr propsTable;
  try {
    const auto paramTableName = "__IqtProperties_temp";
    auto toIqt = AlgorithmManager::Instance().createUnmanaged("TransformToIqt");
    toIqt->initialize();
    toIqt->setChild(true); // record this as internal
    toIqt->setProperty("SampleWorkspace", wsName);
    toIqt->setProperty("ResolutionWorkspace", resName);
    toIqt->setProperty("ParameterWorkspace", paramTableName);
    toIqt->setProperty("EnergyMin", energyMin);
    toIqt->setProperty("EnergyMax", energyMax);
    toIqt->setProperty("BinReductionFactor", binReductionFactor);
    toIqt->setProperty("DryRun", true);
    toIqt->execute();
    propsTable = toIqt->getProperty("ParameterWorkspace");
    // the algorithm can create output even if it failed...
    auto deleter = AlgorithmManager::Instance().create("DeleteWorkspace");
    deleter->initialize();
    deleter->setChild(true);
    deleter->setProperty("Workspace", paramTableName);
    deleter->execute();
  } catch (std::exception &) {
    return std::make_tuple(false, 0.0f, 0, 0);
  }
  assert(propsTable);
  return std::make_tuple(
      true, propsTable->getColumn("EnergyWidth")->cell<float>(0),
      propsTable->getColumn("SampleOutputBins")->cell<int>(0),
      propsTable->getColumn("ResolutionBins")->cell<int>(0));
}
} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
IndirectDataAnalysisIqtTab::IndirectDataAnalysisIqtTab(QWidget *parent)
    : IndirectDataAnalysisTab(parent), m_iqtTree(nullptr), m_iqtResFileType() {
  m_uiForm.setupUi(parent);
  setOutputPlotOptionsPresenter(std::make_unique<IndirectPlotOptionsPresenter>(
      m_uiForm.ipoPlotOptions, this, PlotWidget::SpectraTiled));
}

IndirectDataAnalysisIqtTab::~IndirectDataAnalysisIqtTab() {
  m_iqtTree->unsetFactoryForManager(m_dblManager);
}

void IndirectDataAnalysisIqtTab::setup() {
  m_iqtTree = new QtTreePropertyBrowser();
  m_uiForm.properties->addWidget(m_iqtTree);

  // Create and configure properties
  m_properties["ELow"] = m_dblManager->addProperty("ELow");
  m_dblManager->setDecimals(m_properties["ELow"], NUM_DECIMALS);

  m_properties["EWidth"] = m_dblManager->addProperty("EWidth");
  m_dblManager->setDecimals(m_properties["EWidth"], NUM_DECIMALS);
  m_properties["EWidth"]->setEnabled(false);

  m_properties["EHigh"] = m_dblManager->addProperty("EHigh");
  m_dblManager->setDecimals(m_properties["EHigh"], NUM_DECIMALS);

  m_properties["SampleBinning"] = m_dblManager->addProperty("SampleBinning");
  m_dblManager->setDecimals(m_properties["SampleBinning"], 0);

  m_properties["SampleBins"] = m_dblManager->addProperty("SampleBins");
  m_dblManager->setDecimals(m_properties["SampleBins"], 0);
  m_properties["SampleBins"]->setEnabled(false);

  m_properties["ResolutionBins"] = m_dblManager->addProperty("ResolutionBins");
  m_dblManager->setDecimals(m_properties["ResolutionBins"], 0);
  m_properties["ResolutionBins"]->setEnabled(false);

  m_iqtTree->addProperty(m_properties["ELow"]);
  m_iqtTree->addProperty(m_properties["EWidth"]);
  m_iqtTree->addProperty(m_properties["EHigh"]);
  m_iqtTree->addProperty(m_properties["SampleBinning"]);
  m_iqtTree->addProperty(m_properties["SampleBins"]);
  m_iqtTree->addProperty(m_properties["ResolutionBins"]);

  m_dblManager->setValue(m_properties["SampleBinning"], 10);

  m_iqtTree->setFactoryForManager(m_dblManager, m_dblEdFac);

  // Format the tree widget so its easier to read the contents
  m_iqtTree->setIndentation(0);
  for (auto const &item : m_properties)
    m_iqtTree->setBackgroundColor(m_iqtTree->topLevelItem(item),
                                  QColor(246, 246, 246));

  setPreviewSpectrumMaximum(0);

  auto xRangeSelector = m_uiForm.ppPlot->addRangeSelector("IqtRange");

  // signals / slots & validators
  connect(xRangeSelector, SIGNAL(selectionChanged(double, double)), this,
          SLOT(rangeChanged(double, double)));
  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(updateRangeSelector(QtProperty *, double)));
  connect(m_uiForm.dsInput, SIGNAL(dataReady(const QString &)), this,
          SLOT(plotInput(const QString &)));
  connect(m_uiForm.dsResolution, SIGNAL(dataReady(const QString &)), this,
          SLOT(updateDisplayedBinParameters()));
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(algorithmComplete(bool)));
  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));
  connect(m_uiForm.pbPlotPreview, SIGNAL(clicked()), this,
          SLOT(plotCurrentPreview()));
  connect(m_uiForm.cbCalculateErrors, SIGNAL(clicked()), this,
          SLOT(errorsClicked()));

  connect(m_uiForm.spPreviewSpec, SIGNAL(valueChanged(int)), this,
          SLOT(setSelectedSpectrum(int)));
  connect(m_uiForm.spPreviewSpec, SIGNAL(valueChanged(int)), this,
          SLOT(plotInput()));

  connect(m_uiForm.ckSymmetricEnergy, SIGNAL(stateChanged(int)), this,
          SLOT(updateEnergyRange(int)));

  m_uiForm.dsInput->isOptional(true);
  m_uiForm.dsResolution->isOptional(true);
}

void IndirectDataAnalysisIqtTab::run() {
  m_uiForm.ppPlot->watchADS(false);
  setRunIsRunning(true);

  updateDisplayedBinParameters();

  // Construct the result workspace for Python script export
  QString const sampleName = m_uiForm.dsInput->getCurrentDataName();
  m_pythonExportWsName =
      sampleName.left(sampleName.lastIndexOf("_")).toStdString() + "_iqt";

  QString const wsName = m_uiForm.dsInput->getCurrentDataName();
  QString const resName = m_uiForm.dsResolution->getCurrentDataName();
  QString const nIterations = m_uiForm.spIterations->cleanText();
  bool const calculateErrors = m_uiForm.cbCalculateErrors->isChecked();

  double const energyMin = m_dblManager->value(m_properties["ELow"]);
  double const energyMax = m_dblManager->value(m_properties["EHigh"]);
  double const numBins = m_dblManager->value(m_properties["SampleBinning"]);

  auto IqtAlg = AlgorithmManager::Instance().create("TransformToIqt");
  IqtAlg->initialize();

  IqtAlg->setProperty("SampleWorkspace", wsName.toStdString());
  IqtAlg->setProperty("ResolutionWorkspace", resName.toStdString());
  IqtAlg->setProperty("NumberOfIterations", nIterations.toStdString());
  IqtAlg->setProperty("CalculateErrors", calculateErrors);

  IqtAlg->setProperty("EnergyMin", energyMin);
  IqtAlg->setProperty("EnergyMax", energyMax);
  IqtAlg->setProperty("BinReductionFactor", numBins);
  IqtAlg->setProperty("OutputWorkspace", m_pythonExportWsName);

  IqtAlg->setProperty("DryRun", false);

  m_batchAlgoRunner->addAlgorithm(IqtAlg);
  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Handle algorithm completion.
 *
 * @param error If the algorithm failed
 */
void IndirectDataAnalysisIqtTab::algorithmComplete(bool error) {
  m_uiForm.ppPlot->watchADS(true);
  setRunIsRunning(false);
  if (error)
    setSaveResultEnabled(false);
  else
    setOutputPlotOptionsWorkspaces({m_pythonExportWsName});
}

/**
 * Handle saving of workspace
 */
void IndirectDataAnalysisIqtTab::saveClicked() {
  checkADSForPlotSaveWorkspace(m_pythonExportWsName, false);
  addSaveWorkspaceToQueue(QString::fromStdString(m_pythonExportWsName));
  m_batchAlgoRunner->executeBatchAsync();
}

void IndirectDataAnalysisIqtTab::runClicked() {
  clearOutputPlotOptionsWorkspaces();
  runTab();
}

void IndirectDataAnalysisIqtTab::errorsClicked() {
  m_uiForm.spIterations->setEnabled(isErrorsEnabled());
}

bool IndirectDataAnalysisIqtTab::isErrorsEnabled() {
  return m_uiForm.cbCalculateErrors->isChecked();
}

/**
 * Ensure we have present and valid file/ws inputs.
 *
 * The underlying Fourier transform of Iqt
 * also means we must enforce several rules on the parameters.
 */
bool IndirectDataAnalysisIqtTab::validate() {
  UserInputValidator uiv;

  uiv.checkDataSelectorIsValid("Sample", m_uiForm.dsInput);
  uiv.checkDataSelectorIsValid("Resolution", m_uiForm.dsResolution);

  auto const eLow = m_dblManager->value(m_properties["ELow"]);
  auto const eHigh = m_dblManager->value(m_properties["EHigh"]);

  if (eLow >= eHigh)
    uiv.addErrorMessage("ELow must be less than EHigh.\n");

  auto const sampleName = m_uiForm.dsInput->getCurrentDataName().toStdString();
  auto const resolutionName =
      m_uiForm.dsResolution->getCurrentDataName().toStdString();

  auto &ads = AnalysisDataService::Instance();
  if (ads.doesExist(sampleName) && ads.doesExist(resolutionName)) {
    auto const sampleWorkspace = getADSMatrixWorkspace(sampleName);
    auto const resWorkspace = getADSMatrixWorkspace(resolutionName);

    addErrorMessage(uiv, checkInstrumentsMatch(sampleWorkspace, resWorkspace));
    addErrorMessage(uiv,
                    validateNumberOfHistograms(sampleWorkspace, resWorkspace));

    if (!isTechniqueDirect(sampleWorkspace, resWorkspace)) {
      addErrorMessage(
          uiv, checkParametersMatch(sampleWorkspace, resWorkspace, "analyser"));
      addErrorMessage(uiv, checkParametersMatch(sampleWorkspace, resWorkspace,
                                                "reflection"));
    }
  }

  auto const message = uiv.generateErrorMessage();
  showMessageBox(message);

  return message.isEmpty();
}

/**
 * Calculates binning parameters.
 */
void IndirectDataAnalysisIqtTab::updateDisplayedBinParameters() {
  auto const sampleName = m_uiForm.dsInput->getCurrentDataName().toStdString();
  auto const resolutionName =
      m_uiForm.dsResolution->getCurrentDataName().toStdString();

  auto &ads = AnalysisDataService::Instance();
  if (!ads.doesExist(sampleName) || !ads.doesExist(resolutionName))
    return;

  if (!checkParametersMatch(sampleName, resolutionName, "analyser").empty() ||
      !checkParametersMatch(sampleName, resolutionName, "reflection").empty())
    return;

  double energyMin = m_dblManager->value(m_properties["ELow"]);
  double energyMax = m_dblManager->value(m_properties["EHigh"]);
  double numBins = m_dblManager->value(m_properties["SampleBinning"]);

  if (numBins == 0)
    return;
  if (energyMin == 0 && energyMax == 0)
    return;

  bool success(false);
  float energyWidth(0.0f);
  int resolutionBins(0), sampleBins(0);
  std::tie(success, energyWidth, sampleBins, resolutionBins) =
      calculateBinParameters(sampleName, resolutionName, energyMin, energyMax,
                             numBins);
  if (success) {
    disconnect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
               SLOT(updateRangeSelector(QtProperty *, double)));

    // Update data in property editor
    m_dblManager->setValue(m_properties["EWidth"], energyWidth);
    m_dblManager->setValue(m_properties["ResolutionBins"], resolutionBins);
    m_dblManager->setValue(m_properties["SampleBins"], sampleBins);

    connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
            SLOT(updateRangeSelector(QtProperty *, double)));

    // Warn for low number of resolution bins
    if (resolutionBins < 5)
      showMessageBox("Results may be inaccurate as ResolutionBins is "
                     "less than 5.\nLower the SampleBinning.");
  }
}

void IndirectDataAnalysisIqtTab::loadSettings(const QSettings &settings) {
  m_uiForm.dsInput->readSettings(settings.group());
  m_uiForm.dsResolution->readSettings(settings.group());
}

void IndirectDataAnalysisIqtTab::plotInput() {
  IndirectDataAnalysisTab::plotInput(m_uiForm.ppPlot);
}

void IndirectDataAnalysisIqtTab::setFileExtensionsByName(bool filter) {
  QStringList const noSuffixes{""};
  auto const tabName("Iqt");
  m_uiForm.dsInput->setFBSuffixes(filter ? getSampleFBSuffixes(tabName)
                                         : getExtensions(tabName));
  m_uiForm.dsInput->setWSSuffixes(filter ? getSampleWSSuffixes(tabName)
                                         : noSuffixes);
  m_uiForm.dsResolution->setFBSuffixes(filter ? getResolutionFBSuffixes(tabName)
                                              : getExtensions(tabName));
  m_uiForm.dsResolution->setWSSuffixes(filter ? getResolutionWSSuffixes(tabName)
                                              : noSuffixes);
}

void IndirectDataAnalysisIqtTab::plotInput(const QString &wsname) {
  MatrixWorkspace_sptr workspace;
  try {
    workspace = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        wsname.toStdString());
    setInputWorkspace(workspace);
  } catch (Mantid::Kernel::Exception::NotFoundError &) {
    showMessageBox(QString("Unable to retrieve workspace: " + wsname));
    setPreviewSpectrumMaximum(0);
    return;
  }

  setPreviewSpectrumMaximum(
      static_cast<int>(getInputWorkspace()->getNumberHistograms()) - 1);

  IndirectDataAnalysisTab::plotInput(m_uiForm.ppPlot);
  auto xRangeSelector = m_uiForm.ppPlot->getRangeSelector("IqtRange");

  try {
    auto const range = getXRangeFromWorkspace(workspace);
    double rounded_min(range.first);
    double rounded_max(range.second);
    const std::string instrName(workspace->getInstrument()->getName());
    if (instrName == "BASIS") {
      xRangeSelector->setRange(range.first, range.second);
      m_dblManager->setValue(m_properties["ELow"], rounded_min);
      m_dblManager->setValue(m_properties["EHigh"], rounded_max);
      m_dblManager->setValue(m_properties["EWidth"], 0.0004);
      m_dblManager->setValue(m_properties["SampleBinning"], 1);
    } else {
      rounded_min = floor(rounded_min * 10 + 0.5) / 10.0;
      rounded_max = floor(rounded_max * 10 + 0.5) / 10.0;

      // corrections for if nearest value is outside of range
      if (rounded_max > range.second) {
        rounded_max -= 0.1;
      }

      if (rounded_min < range.first) {
        rounded_min += 0.1;
      }

      // check incase we have a really small range
      if (fabs(rounded_min) > 0 && fabs(rounded_max) > 0) {
        xRangeSelector->setRange(rounded_min, rounded_max);
        m_dblManager->setValue(m_properties["ELow"], rounded_min);
        m_dblManager->setValue(m_properties["EHigh"], rounded_max);
      } else {
        xRangeSelector->setRange(range.first, range.second);
        m_dblManager->setValue(m_properties["ELow"], range.first);
        m_dblManager->setValue(m_properties["EHigh"], range.second);
      }
      // set default value for width
      m_dblManager->setValue(m_properties["EWidth"], 0.005);
    }
  } catch (std::invalid_argument &exc) {
    showMessageBox(exc.what());
  }

  updateDisplayedBinParameters();
}

void IndirectDataAnalysisIqtTab::setPreviewSpectrumMaximum(int value) {
  m_uiForm.spPreviewSpec->setMaximum(value);
}

/**
 * Updates the range selectors and properties when range selector is moved.
 *
 * @param min Range selector min value
 * @param max Range selector max value
 */
void IndirectDataAnalysisIqtTab::rangeChanged(double min, double max) {
  double oldMin = m_dblManager->value(m_properties["ELow"]);
  double oldMax = m_dblManager->value(m_properties["EHigh"]);

  auto xRangeSelector = m_uiForm.ppPlot->getRangeSelector("IqtRange");

  disconnect(xRangeSelector, SIGNAL(selectionChanged(double, double)), this,
             SLOT(rangeChanged(double, double)));
  disconnect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
             SLOT(updateRangeSelector(QtProperty *, double)));

  if (fabs(oldMin - min) > 0.0000001) {
    m_dblManager->setValue(m_properties["ELow"], min);
    xRangeSelector->setMinimum(min);
    if (m_uiForm.ckSymmetricEnergy->isChecked()) {
      m_dblManager->setValue(m_properties["EHigh"], -min);
      xRangeSelector->setMaximum(-min);
    }
  }

  if (fabs(oldMax - max) > 0.0000001) {
    m_dblManager->setValue(m_properties["EHigh"], max);
    xRangeSelector->setMaximum(max);
    if (m_uiForm.ckSymmetricEnergy->isChecked()) {
      m_dblManager->setValue(m_properties["ELow"], -max);
      xRangeSelector->setMinimum(-max);
    }
  }

  connect(xRangeSelector, SIGNAL(selectionChanged(double, double)), this,
          SLOT(rangeChanged(double, double)));
  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(updateRangeSelector(QtProperty *, double)));
}

/**
 * Updates the range selectors when the ELow or EHigh property is changed in the
 * table.
 *
 * @param prop The property which has been changed
 * @param val The new position for the range selector
 */
void IndirectDataAnalysisIqtTab::updateRangeSelector(QtProperty *prop,
                                                     double val) {
  auto xRangeSelector = m_uiForm.ppPlot->getRangeSelector("IqtRange");

  disconnect(xRangeSelector, SIGNAL(selectionChanged(double, double)), this,
             SLOT(rangeChanged(double, double)));
  disconnect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
             SLOT(updateRangeSelector(QtProperty *, double)));

  if (prop == m_properties["ELow"]) {
    setRangeSelectorMin(m_properties["ELow"], m_properties["EHigh"],
                        xRangeSelector, val);
    if (m_uiForm.ckSymmetricEnergy->isChecked()) {
      m_dblManager->setValue(m_properties["EHigh"], -val);
      setRangeSelectorMax(m_properties["ELow"], m_properties["EHigh"],
                          xRangeSelector, -val);
    }

  } else if (prop == m_properties["EHigh"]) {
    setRangeSelectorMax(m_properties["ELow"], m_properties["EHigh"],
                        xRangeSelector, val);
    if (m_uiForm.ckSymmetricEnergy->isChecked()) {
      m_dblManager->setValue(m_properties["ELow"], -val);
      setRangeSelectorMin(m_properties["ELow"], m_properties["EHigh"],
                          xRangeSelector, -val);
    }
  }

  connect(xRangeSelector, SIGNAL(selectionChanged(double, double)), this,
          SLOT(rangeChanged(double, double)));
  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(updateRangeSelector(QtProperty *, double)));

  updateDisplayedBinParameters();
}

void IndirectDataAnalysisIqtTab::updateEnergyRange(int state) {
  if (state != 0) {
    auto const value = m_dblManager->value(m_properties["ELow"]);
    m_dblManager->setValue(m_properties["EHigh"], -value);
  }
}

void IndirectDataAnalysisIqtTab::setRunEnabled(bool enabled) {
  m_uiForm.pbRun->setEnabled(enabled);
}

void IndirectDataAnalysisIqtTab::setSaveResultEnabled(bool enabled) {
  m_uiForm.pbSave->setEnabled(enabled);
}

void IndirectDataAnalysisIqtTab::setButtonsEnabled(bool enabled) {
  setRunEnabled(enabled);
  setSaveResultEnabled(enabled);
}

void IndirectDataAnalysisIqtTab::setRunIsRunning(bool running) {
  m_uiForm.pbRun->setText(running ? "Running..." : "Run");
  setButtonsEnabled(!running);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
