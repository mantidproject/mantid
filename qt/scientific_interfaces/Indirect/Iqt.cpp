// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "Iqt.h"

#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidQtWidgets/Common/SignalBlocker.h"
#include "MantidQtWidgets/LegacyQwt/RangeSelector.h"

#include "../General/UserInputValidator.h"

#include <qwt_plot.h>

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

std::size_t getWsNumberOfSpectra(std::string const &workspaceName) {
  return getADSMatrixWorkspace(workspaceName)->getNumberHistograms();
}

bool isWorkspacePlottable(MatrixWorkspace_sptr workspace) {
  return workspace->y(0).size() > 1;
}

std::string
checkInstrumentParametersMatch(Instrument_const_sptr sampleInstrument,
                               Instrument_const_sptr resolutionInstrument,
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

std::string checkParametersMatch(MatrixWorkspace_const_sptr sampleWorkspace,
                                 MatrixWorkspace_const_sptr resolutionWorkspace,
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
checkInstrumentsMatch(MatrixWorkspace_const_sptr sampleWorkspace,
                      MatrixWorkspace_const_sptr resolutionWorkspace) {
  auto const sampleInstrument = sampleWorkspace->getInstrument();
  auto const resolutionInstrument = resolutionWorkspace->getInstrument();
  if (sampleInstrument->getName() != resolutionInstrument->getName())
    return "The sample and resolution must have matching instruments.";
  return "";
}

std::string
validateNumberOfHistograms(MatrixWorkspace_const_sptr sampleWorkspace,
                           MatrixWorkspace_const_sptr resolutionWorkspace) {
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

void cloneWorkspace(std::string const &workspaceName,
                    std::string const &cloneName) {
  auto cloner = AlgorithmManager::Instance().create("CloneWorkspace");
  cloner->initialize();
  cloner->setProperty("InputWorkspace", workspaceName);
  cloner->setProperty("OutputWorkspace", cloneName);
  cloner->execute();
}

void cropWorkspace(std::string const &name, std::string const &newName,
                   double const &cropValue) {
  auto croper = AlgorithmManager::Instance().create("CropWorkspace");
  croper->initialize();
  croper->setProperty("InputWorkspace", name);
  croper->setProperty("OutputWorkspace", newName);
  croper->setProperty("XMin", cropValue);
  croper->execute();
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
  const auto paramTableName = "__IqtProperties_temp";
  try {
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
Iqt::Iqt(QWidget *parent)
    : IndirectDataAnalysisTab(parent), m_iqtTree(nullptr), m_iqtResFileType() {
  m_uiForm.setupUi(parent);
}

void Iqt::setup() {
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

  auto xRangeSelector = m_uiForm.ppPlot->addRangeSelector("IqtRange");

  // signals / slots & validators
  connect(xRangeSelector, SIGNAL(selectionChangedLazy(double, double)), this,
          SLOT(rsRangeChangedLazy(double, double)));
  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(updateRS(QtProperty *, double)));
  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(updatePropertyValues(QtProperty *, double)));
  connect(m_uiForm.dsInput, SIGNAL(dataReady(const QString &)), this,
          SLOT(plotInput(const QString &)));
  connect(m_uiForm.dsResolution, SIGNAL(dataReady(const QString &)), this,
          SLOT(updateDisplayedBinParameters()));
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(algorithmComplete(bool)));
  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(runClicked()));
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));
  connect(m_uiForm.pbPlot, SIGNAL(clicked()), this, SLOT(plotClicked()));
  connect(m_uiForm.pbTile, SIGNAL(clicked()), this, SLOT(plotTiled()));
  connect(m_uiForm.pbPlotPreview, SIGNAL(clicked()), this,
          SLOT(plotCurrentPreview()));
  connect(m_uiForm.cbCalculateErrors, SIGNAL(clicked()), this,
          SLOT(errorsClicked()));
  connect(m_uiForm.spTiledPlotFirst, SIGNAL(valueChanged(int)), this,
          SLOT(setTiledPlotFirstPlot(int)));
  connect(m_uiForm.spTiledPlotLast, SIGNAL(valueChanged(int)), this,
          SLOT(setTiledPlotLastPlot(int)));
}

void Iqt::run() {
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
void Iqt::algorithmComplete(bool error) {
  setRunIsRunning(false);
  if (error) {
    setPlotSpectrumEnabled(false);
    setTiledPlotEnabled(false);
    setSaveResultEnabled(false);
  } else {
    auto const lastSpectrumIndex =
        static_cast<int>(getWsNumberOfSpectra(m_pythonExportWsName)) - 1;
    auto const selectedSpec = selectedSpectrum();

    setPlotSpectrumIndexMax(lastSpectrumIndex);
    setPlotSpectrumIndex(selectedSpec);
    setMinMaxOfTiledPlotFirstIndex(0, lastSpectrumIndex);
    setMinMaxOfTiledPlotLastIndex(0, lastSpectrumIndex);
    setTiledPlotFirstIndex(selectedSpec);
    setTiledPlotLastIndex(lastSpectrumIndex);
  }
} // namespace IDA
/**
 * Handle saving of workspace
 */
void Iqt::saveClicked() {
  checkADSForPlotSaveWorkspace(m_pythonExportWsName, false);
  addSaveWorkspaceToQueue(QString::fromStdString(m_pythonExportWsName));
  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Handle mantid plotting
 */
void Iqt::plotClicked() {
  setPlotSpectrumIsPlotting(true);

  plotResult(QString::fromStdString(m_pythonExportWsName));

  setPlotSpectrumIsPlotting(false);
}

void Iqt::plotResult(QString const &workspaceName) {
  auto const name = workspaceName.toStdString();
  if (checkADSForPlotSaveWorkspace(name, true)) {
    if (isWorkspacePlottable(getADSMatrixWorkspace(name)))
      plotSpectrum(workspaceName, getPlotSpectrumIndex());
    else
      showMessageBox("Plotting a spectrum of the workspace " + workspaceName +
                     " failed : Workspace only has one data point");
  }
}

void Iqt::runClicked() { runTab(); }

void Iqt::errorsClicked() {
  m_uiForm.spIterations->setEnabled(isErrorsEnabled());
}

bool Iqt::isErrorsEnabled() { return m_uiForm.cbCalculateErrors->isChecked(); }

std::size_t Iqt::getXMinIndex(Mantid::MantidVec const &yData,
                              std::vector<double>::const_iterator iter) {
  auto cropIndex = 0;
  if (iter != yData.end()) {
    auto const index = static_cast<int>(iter - yData.begin());
    cropIndex = index > 0 ? index - 1 : index;
  } else
    showMessageBox(
        "Incorrect data provided for Tiled Plot: y values are out of range");
  return cropIndex;
}

double Iqt::getXMinValue(MatrixWorkspace_const_sptr workspace,
                         std::size_t const &index) {
  auto const firstSpectraYData = workspace->dataY(index);
  auto const positionIter =
      std::find_if(firstSpectraYData.begin(), firstSpectraYData.end(),
                   [&](double const &value) { return value < 1.0; });
  auto const cropIndex = getXMinIndex(firstSpectraYData, positionIter);
  return workspace->dataX(index)[cropIndex];
}

void Iqt::plotTiled() {
  setTiledPlotIsPlotting(true);

  auto const outWs = getADSMatrixWorkspace(m_pythonExportWsName);

  auto const tiledPlotWsName = outWs->getName() + "_tiled";
  auto const firstTiledPlot = m_uiForm.spTiledPlotFirst->text().toInt();
  auto const lastTiledPlot = m_uiForm.spTiledPlotLast->text().toInt();

  // Clone workspace before cropping to keep in ADS
  if (!AnalysisDataService::Instance().doesExist(tiledPlotWsName))
    cloneWorkspace(outWs->getName(), tiledPlotWsName);

  // Get first x value which corresponds to a y value below 1
  auto const cropValue =
      getXMinValue(outWs, static_cast<std::size_t>(firstTiledPlot));
  cropWorkspace(tiledPlotWsName, tiledPlotWsName, cropValue);

  auto const tiledPlotWs = getADSMatrixWorkspace(tiledPlotWsName);

  // Plot tiledwindow
  std::size_t const numberOfPlots = lastTiledPlot - firstTiledPlot + 1;
  if (numberOfPlots != 0) {
    QString pyInput = "from mantidplot import newTiledWindow\n";
    pyInput += "newTiledWindow(sources=[";
    for (auto index = firstTiledPlot; index <= lastTiledPlot; ++index) {
      if (index > firstTiledPlot) {
        pyInput += ",";
      }
      std::string const pyInStr =
          "(['" + tiledPlotWsName + "'], " + std::to_string(index) + ")";
      pyInput += QString::fromStdString(pyInStr);
    }
    pyInput += "])\n";
    runPythonCode(pyInput);
  }
  setTiledPlotIsPlotting(false);
}

/**
 * Ensure we have present and valid file/ws inputs.
 *
 * The underlying Fourier transform of Iqt
 * also means we must enforce several rules on the parameters.
 */
bool Iqt::validate() {
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
    addErrorMessage(
        uiv, checkParametersMatch(sampleWorkspace, resWorkspace, "analyser"));
    addErrorMessage(
        uiv, checkParametersMatch(sampleWorkspace, resWorkspace, "reflection"));
    addErrorMessage(uiv,
                    validateNumberOfHistograms(sampleWorkspace, resWorkspace));
  }

  auto const message = uiv.generateErrorMessage();
  showMessageBox(message);

  return message.isEmpty();
}

/**
 * Ensures that absolute min and max energy are equal.
 *
 * @param prop Qt property that was changed
 * @param val New value of that property
 */
void Iqt::updatePropertyValues(QtProperty *prop, double val) {
  disconnect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
             SLOT(updatePropertyValues(QtProperty *, double)));

  if (prop == m_properties["EHigh"]) {
    // If the user enters a negative value for EHigh assume they did not mean to
    // add a -
    if (val < 0) {
      val = -val;
      m_dblManager->setValue(m_properties["EHigh"], val);
    }

    m_dblManager->setValue(m_properties["ELow"], -val);
  } else if (prop == m_properties["ELow"]) {
    // If the user enters a positive value for ELow, assume they meant to add a
    if (val > 0) {
      val = -val;
      m_dblManager->setValue(m_properties["ELow"], val);
    }

    m_dblManager->setValue(m_properties["EHigh"], -val);
  }

  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(updatePropertyValues(QtProperty *, double)));

  updateDisplayedBinParameters();
}

/**
 * Calculates binning parameters.
 */
void Iqt::updateDisplayedBinParameters() {
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
               SLOT(updatePropertyValues(QtProperty *, double)));
    // Update data in property editor
    m_dblManager->setValue(m_properties["EWidth"], energyWidth);
    m_dblManager->setValue(m_properties["ResolutionBins"], resolutionBins);
    m_dblManager->setValue(m_properties["SampleBins"], sampleBins);
    connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
            SLOT(updatePropertyValues(QtProperty *, double)));

    // Warn for low number of resolution bins
    if (resolutionBins < 5)
      showMessageBox("Results may be inaccurate as ResolutionBins is "
                     "less than 5.\nLower the SampleBinning.");
  }
}

void Iqt::loadSettings(const QSettings &settings) {
  m_uiForm.dsInput->readSettings(settings.group());
  m_uiForm.dsResolution->readSettings(settings.group());
}

void Iqt::plotInput(const QString &wsname) {
  disconnect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
             SLOT(updatePropertyValues(QtProperty *, double)));

  MatrixWorkspace_sptr workspace;
  try {
    workspace = Mantid::API::AnalysisDataService::Instance()
                    .retrieveWS<MatrixWorkspace>(wsname.toStdString());
    setInputWorkspace(workspace);
  } catch (Mantid::Kernel::Exception::NotFoundError &) {
    showMessageBox(QString("Unable to retrieve workspace: " + wsname));
    return;
  }

  IndirectDataAnalysisTab::plotInput(m_uiForm.ppPlot);
  auto xRangeSelector = m_uiForm.ppPlot->getRangeSelector("IqtRange");

  try {
    QPair<double, double> range = m_uiForm.ppPlot->getCurveRange("Sample");
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

  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(updatePropertyValues(QtProperty *, double)));

  updateDisplayedBinParameters();
}

/**
 * Updates the range selectors and properties when range selector is moved.
 *
 * @param min Range selector min value
 * @param max Range selector amx value
 */
void Iqt::rsRangeChangedLazy(double min, double max) {
  double oldMin = m_dblManager->value(m_properties["ELow"]);
  double oldMax = m_dblManager->value(m_properties["EHigh"]);

  if (fabs(oldMin - min) > 0.0000001)
    m_dblManager->setValue(m_properties["ELow"], min);

  if (fabs(oldMax - max) > 0.0000001)
    m_dblManager->setValue(m_properties["EHigh"], max);
}

void Iqt::updateRS(QtProperty *prop, double val) {
  auto xRangeSelector = m_uiForm.ppPlot->getRangeSelector("IqtRange");

  if (prop == m_properties["ELow"])
    xRangeSelector->setMinimum(val);
  else if (prop == m_properties["EHigh"])
    xRangeSelector->setMaximum(val);
}

void Iqt::setTiledPlotFirstPlot(int value) {
  MantidQt::API::SignalBlocker<QObject> blocker(m_uiForm.spTiledPlotFirst);
  auto const lastPlotIndex = m_uiForm.spTiledPlotLast->text().toInt();
  auto const rangeSize = lastPlotIndex - value;
  if (value > lastPlotIndex)
    setTiledPlotFirstIndex(lastPlotIndex);
  else if (rangeSize > m_maxTiledPlots) {
    auto const lastSpectrumIndex =
        static_cast<int>(getWsNumberOfSpectra(m_pythonExportWsName)) - 1;
    auto const lastIndex = value + m_maxTiledPlots <= lastSpectrumIndex
                               ? value + m_maxTiledPlots
                               : lastSpectrumIndex;
    setTiledPlotLastIndex(lastIndex);
  }
}

void Iqt::setTiledPlotLastPlot(int value) {
  MantidQt::API::SignalBlocker<QObject> blocker(m_uiForm.spTiledPlotLast);
  auto const firstPlotIndex = m_uiForm.spTiledPlotFirst->text().toInt();
  auto const rangeSize = value - firstPlotIndex;
  if (value < firstPlotIndex)
    setTiledPlotLastIndex(firstPlotIndex);
  else if (rangeSize > m_maxTiledPlots) {
    auto const firstIndex =
        value - m_maxTiledPlots >= 0 ? value - m_maxTiledPlots : 0;
    setTiledPlotFirstIndex(firstIndex);
  }
}

void Iqt::setMinMaxOfTiledPlotFirstIndex(int minimum, int maximum) {
  m_uiForm.spTiledPlotFirst->setMinimum(minimum);
  m_uiForm.spTiledPlotFirst->setMaximum(maximum);
}

void Iqt::setMinMaxOfTiledPlotLastIndex(int minimum, int maximum) {
  m_uiForm.spTiledPlotLast->setMinimum(minimum);
  m_uiForm.spTiledPlotLast->setMaximum(maximum);
}

void Iqt::setTiledPlotFirstIndex(int value) {
  MantidQt::API::SignalBlocker<QObject> blocker(m_uiForm.spTiledPlotFirst);
  m_uiForm.spTiledPlotFirst->setValue(value);
}

void Iqt::setTiledPlotLastIndex(int value) {
  MantidQt::API::SignalBlocker<QObject> blocker(m_uiForm.spTiledPlotLast);
  auto const firstPlotIndex = m_uiForm.spTiledPlotFirst->text().toInt();
  auto const lastPlotIndex = value - m_maxTiledPlots > firstPlotIndex
                                 ? firstPlotIndex + m_maxTiledPlots
                                 : value;
  m_uiForm.spTiledPlotLast->setValue(lastPlotIndex);
}

void Iqt::setPlotSpectrumIndexMax(int maximum) {
  MantidQt::API::SignalBlocker<QObject> blocker(m_uiForm.spSpectrum);
  m_uiForm.spSpectrum->setMaximum(maximum);
}

void Iqt::setPlotSpectrumIndex(int spectrum) {
  MantidQt::API::SignalBlocker<QObject> blocker(m_uiForm.spSpectrum);
  m_uiForm.spSpectrum->setValue(spectrum);
}

int Iqt::getPlotSpectrumIndex() { return m_uiForm.spSpectrum->text().toInt(); }

void Iqt::setRunEnabled(bool enabled) { m_uiForm.pbRun->setEnabled(enabled); }

void Iqt::setPlotSpectrumEnabled(bool enabled) {
  m_uiForm.pbPlot->setEnabled(enabled);
  m_uiForm.spSpectrum->setEnabled(enabled);
}

void Iqt::setTiledPlotEnabled(bool enabled) {
  m_uiForm.pbTile->setEnabled(enabled);
  m_uiForm.spTiledPlotFirst->setEnabled(enabled);
  m_uiForm.spTiledPlotLast->setEnabled(enabled);
}

void Iqt::setSaveResultEnabled(bool enabled) {
  m_uiForm.pbSave->setEnabled(enabled);
}

void Iqt::setButtonsEnabled(bool enabled) {
  setRunEnabled(enabled);
  setPlotSpectrumEnabled(enabled);
  setSaveResultEnabled(enabled);
  setTiledPlotEnabled(enabled);
}

void Iqt::setRunIsRunning(bool running) {
  m_uiForm.pbRun->setText(running ? "Running..." : "Run");
  setButtonsEnabled(!running);
}

void Iqt::setPlotSpectrumIsPlotting(bool plotting) {
  m_uiForm.pbPlot->setText(plotting ? "Plotting..." : "Plot Spectrum");
  setButtonsEnabled(!plotting);
}

void Iqt::setTiledPlotIsPlotting(bool plotting) {
  m_uiForm.pbTile->setText(plotting ? "Plotting..." : "Tiled Plot");
  setButtonsEnabled(!plotting);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
