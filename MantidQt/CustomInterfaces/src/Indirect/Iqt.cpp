#include "MantidQtCustomInterfaces/Indirect/Iqt.h"
#include "MantidQtCustomInterfaces/UserInputValidator.h"

#include "MantidAPI/ITableWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidQtMantidWidgets/RangeSelector.h"

#include <qwt_plot.h>

namespace {
Mantid::Kernel::Logger g_log("Iqt");
}

using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
Iqt::Iqt(QWidget *parent)
    : IndirectDataAnalysisTab(parent), m_iqtTree(NULL), m_iqtResFileType() {
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
          SLOT(calculateBinning()));
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(algorithmComplete(bool)));
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveClicked()));
  connect(m_uiForm.pbPlot, SIGNAL(clicked()), this, SLOT(plotClicked()));
  connect(m_uiForm.pbTile, SIGNAL(clicked()), this, SLOT(PlotTiled()));
}

void Iqt::run() {
  using namespace Mantid::API;

  calculateBinning();

  // Construct the result workspace for Python script export
  QString sampleName = m_uiForm.dsInput->getCurrentDataName();
  m_pythonExportWsName =
      sampleName.left(sampleName.lastIndexOf("_")).toStdString() + "_iqt";

  QString wsName = m_uiForm.dsInput->getCurrentDataName();
  QString resName = m_uiForm.dsResolution->getCurrentDataName();

  double energyMin = m_dblManager->value(m_properties["ELow"]);
  double energyMax = m_dblManager->value(m_properties["EHigh"]);
  double numBins = m_dblManager->value(m_properties["SampleBinning"]);

  IAlgorithm_sptr IqtAlg =
      AlgorithmManager::Instance().create("TransformToIqt");
  IqtAlg->initialize();

  IqtAlg->setProperty("SampleWorkspace", wsName.toStdString());
  IqtAlg->setProperty("ResolutionWorkspace", resName.toStdString());

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
  if (error)
    return;
  m_uiForm.pbPlot->setEnabled(true);
  m_uiForm.pbSave->setEnabled(true);
  m_uiForm.pbTile->setEnabled(true);
}
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
  checkADSForPlotSaveWorkspace(m_pythonExportWsName, false);
  plotSpectrum(QString::fromStdString(m_pythonExportWsName));
}

void Iqt::PlotTiled() {
  MatrixWorkspace_const_sptr outWs =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          m_pythonExportWsName);

  // Find x value where y > 1 in 0th spectra
  const auto tiledPlotWsName = outWs->getName() + "_tiled";
  const auto y_data = outWs->y(0);
  const auto y_data_length = y_data.size();
  auto crop_index = y_data.size();
  for (size_t i = 0; i < y_data_length; i++) {
    if (y_data[i] > 1) {
      crop_index = i - 1;
      break;
    }
  }
  const auto crop_value = outWs->x(0)[crop_index];

  // Clone workspace before cropping to keep in ADS
  IAlgorithm_sptr clone = AlgorithmManager::Instance().create("CloneWorkspace");
  clone->initialize();
  clone->setProperty("InputWorkspace", outWs->getName());
  clone->setProperty("OutputWorkspace", tiledPlotWsName);
  clone->execute();

  // Crop based on crop_value
  IAlgorithm_sptr crop = AlgorithmManager::Instance().create("CropWorkspace");
  crop->initialize();
  crop->setProperty("InputWorkspace", tiledPlotWsName);
  crop->setProperty("OutputWorkspace", tiledPlotWsName);
  crop->setProperty("XMax", crop_value);
  crop->execute();
  MatrixWorkspace_const_sptr tiledPlotWs =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
          tiledPlotWsName);

  // Plot tiledwindow
  const size_t nPlots = tiledPlotWs->getNumberHistograms();
  if (nPlots == 0)
    return;
  QString pyInput = "from mantidplot import newTiledWindow\n";
  pyInput += "newTiledWindow(sources=[";
  for (size_t index = 0; index < nPlots; ++index) {
    if (index > 0) {
      pyInput += ",";
    }
    const std::string pyInStr =
        "(['" + tiledPlotWsName + "'], " + std::to_string(index) + ")";
    pyInput += QString::fromStdString(pyInStr);
  }
  pyInput += "])\n";
  runPythonCode(pyInput);
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

  const auto eLow = m_dblManager->value(m_properties["ELow"]);
  const auto eHigh = m_dblManager->value(m_properties["EHigh"]);
  if (eLow >= eHigh)
    uiv.addErrorMessage("ELow must be strictly less than EHigh.\n");

  QString message = uiv.generateErrorMessage();
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
    // If the user enters a positive value for ELow, assume they ment to add a
    if (val > 0) {
      val = -val;
      m_dblManager->setValue(m_properties["ELow"], val);
    }

    m_dblManager->setValue(m_properties["EHigh"], -val);
  }

  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(updatePropertyValues(QtProperty *, double)));

  calculateBinning();
}

/**
 * Calculates binning parameters.
 */
void Iqt::calculateBinning() {
  using namespace Mantid::API;

  QString wsName = m_uiForm.dsInput->getCurrentDataName();
  QString resName = m_uiForm.dsResolution->getCurrentDataName();
  if (wsName.isEmpty() || resName.isEmpty())
    return;

  double energyMin = m_dblManager->value(m_properties["ELow"]);
  double energyMax = m_dblManager->value(m_properties["EHigh"]);
  double numBins = m_dblManager->value(m_properties["SampleBinning"]);

  if (numBins == 0)
    return;
  if (energyMin == 0 && energyMax == 0)
    return;

  const auto paramTableName = "__IqtProperties_temp";

  IAlgorithm_sptr IqtAlg =
      AlgorithmManager::Instance().create("TransformToIqt");
  IqtAlg->initialize();
  IqtAlg->setChild(true);
  IqtAlg->setProperty("SampleWorkspace", wsName.toStdString());
  IqtAlg->setProperty("ResolutionWorkspace", resName.toStdString());
  IqtAlg->setProperty("ParameterWorkspace", paramTableName);

  IqtAlg->setProperty("EnergyMin", energyMin);
  IqtAlg->setProperty("EnergyMax", energyMax);
  IqtAlg->setProperty("BinReductionFactor", numBins);

  IqtAlg->setProperty("DryRun", true);

  IqtAlg->execute();

  // Get property table from algorithm
  ITableWorkspace_sptr propsTable = IqtAlg->getProperty("ParameterWorkspace");

  // Get data from property table
  const auto energyWidth = propsTable->getColumn("EnergyWidth")->cell<float>(0);
  const auto sampleBins =
      propsTable->getColumn("SampleOutputBins")->cell<int>(0);
  const auto resolutionBins =
      propsTable->getColumn("ResolutionBins")->cell<int>(0);

  disconnect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
             SLOT(updatePropertyValues(QtProperty *, double)));

  // Update data in property editor
  m_dblManager->setValue(m_properties["EWidth"], energyWidth);
  m_dblManager->setValue(m_properties["ResolutionBins"], resolutionBins);
  m_dblManager->setValue(m_properties["SampleBins"], sampleBins);

  connect(m_dblManager, SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(updatePropertyValues(QtProperty *, double)));

  // Warn for low number of resolution bins
  int numResolutionBins =
      static_cast<int>(m_dblManager->value(m_properties["ResolutionBins"]));
  if (numResolutionBins < 5)
    showMessageBox("Number of resolution bins is less than 5.\nResults may be "
                   "inaccurate.");

  IAlgorithm_sptr deleteAlg =
      AlgorithmManager::Instance().create("DeleteWorkspace");
  deleteAlg->initialize();
  deleteAlg->setChild(true);
  deleteAlg->setProperty("Workspace", paramTableName);
  deleteAlg->execute();
}

void Iqt::loadSettings(const QSettings &settings) {
  m_uiForm.dsInput->readSettings(settings.group());
  m_uiForm.dsResolution->readSettings(settings.group());
}

void Iqt::plotInput(const QString &wsname) {
  MatrixWorkspace_sptr workspace;
  try {
    workspace = Mantid::API::AnalysisDataService::Instance()
                    .retrieveWS<MatrixWorkspace>(wsname.toStdString());
  } catch (Mantid::Kernel::Exception::NotFoundError &) {
    showMessageBox(QString("Unable to retrieve workspace: " + wsname));
    return;
  }

  m_uiForm.ppPlot->clear();
  m_uiForm.ppPlot->addSpectrum("Sample", workspace, 0);

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

  calculateBinning();
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

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
