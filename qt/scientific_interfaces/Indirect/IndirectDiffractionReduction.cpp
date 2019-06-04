// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectDiffractionReduction.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/MultiFileNameParser.h"
#include "MantidQtWidgets/Common/SignalBlocker.h"

using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace MantidQt {
namespace CustomInterfaces {

namespace {
Mantid::Kernel::Logger g_log("IndirectDiffractionReduction");

// Helper function for use with std::transform.
std::string toStdString(const QString &qString) {
  return qString.toStdString();
}
} // namespace

DECLARE_SUBWINDOW(IndirectDiffractionReduction)

using MantidQt::API::BatchAlgorithmRunner;

IndirectDiffractionReduction::IndirectDiffractionReduction(QWidget *parent)
    : IndirectInterface(parent), m_valDbl(nullptr),
      m_settingsGroup("CustomInterfaces/DEMON"),
      m_batchAlgoRunner(new BatchAlgorithmRunner(parent)) {}

IndirectDiffractionReduction::~IndirectDiffractionReduction() {
  saveSettings();
}

/**
 * Sets up UI components and Qt signal/slot connections.
 */
void IndirectDiffractionReduction::initLayout() {
  m_uiForm.setupUi(this);

  connect(m_uiForm.pbSettings, SIGNAL(clicked()), this, SLOT(settings()));
  connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(help()));
  connect(m_uiForm.pbManageDirs, SIGNAL(clicked()), this,
          SLOT(manageUserDirectories()));
  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(run()));

  connect(m_uiForm.iicInstrumentConfiguration,
          SIGNAL(instrumentConfigurationUpdated(
              const QString &, const QString &, const QString &)),
          this,
          SLOT(instrumentSelected(const QString &, const QString &,
                                  const QString &)));

  connect(m_uiForm.spSpecMin, SIGNAL(valueChanged(int)), this,
          SLOT(validateSpectrumMin(int)));
  connect(m_uiForm.spSpecMax, SIGNAL(valueChanged(int)), this,
          SLOT(validateSpectrumMax(int)));

  // Update run button based on state of raw files field
  connectRunButtonValidation(m_uiForm.rfSampleFiles);
  connectRunButtonValidation(m_uiForm.rfCanFiles);
  connectRunButtonValidation(m_uiForm.rfCalFile);
  connectRunButtonValidation(m_uiForm.rfCalFile_only);
  connectRunButtonValidation(m_uiForm.rfVanadiumFile);
  connectRunButtonValidation(m_uiForm.rfVanFile_only);

  m_valDbl = new QDoubleValidator(this);

  m_uiForm.leRebinStart->setValidator(m_valDbl);
  m_uiForm.leRebinWidth->setValidator(m_valDbl);
  m_uiForm.leRebinEnd->setValidator(m_valDbl);
  m_uiForm.leRebinStart_CalibOnly->setValidator(m_valDbl);
  m_uiForm.leRebinWidth_CalibOnly->setValidator(m_valDbl);
  m_uiForm.leRebinEnd_CalibOnly->setValidator(m_valDbl);

  // Update the list of plot options when manual grouping is toggled
  connect(m_uiForm.ckManualGrouping, SIGNAL(stateChanged(int)), this,
          SLOT(manualGroupingToggled(int)));

  // Handle plotting
  connect(m_uiForm.pbPlot, SIGNAL(clicked()), this, SLOT(plotResults()));
  // Handle saving
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveReductions()));

  loadSettings();

  // Update invalid rebinning markers
  validateRebin();

  // Update invalid markers
  validateCalOnly();

  // Update instrument dependant widgets
  m_uiForm.iicInstrumentConfiguration->newInstrumentConfiguration();
}

/**
 * Make file finding status display on the run button and enable/disable it
 */
void IndirectDiffractionReduction::connectRunButtonValidation(
    const MantidQt::API::MWRunFiles *file_field) {
  connect(file_field, SIGNAL(fileTextChanged(const QString &)), this,
          SLOT(runFilesChanged()));
  connect(file_field, SIGNAL(findingFiles()), this, SLOT(runFilesFinding()));
  connect(file_field, SIGNAL(fileFindingFinished()), this,
          SLOT(runFilesFound()));
}

/**
 * Runs a diffraction reduction when the user clicks Run.
 */
void IndirectDiffractionReduction::run() {
  setRunIsRunning(true);

  QString instName = m_uiForm.iicInstrumentConfiguration->getInstrumentName();
  QString mode = m_uiForm.iicInstrumentConfiguration->getReflectionName();
  if (!m_uiForm.rfSampleFiles->isValid()) {
    showInformationBox("Sample files input is invalid.");
    return;
  }

  if (mode == "diffspec" && m_uiForm.ckUseVanadium->isChecked() &&
      m_uiForm.rfVanFile_only->getFilenames().isEmpty()) {
    showInformationBox("Use Vanadium File checked but no vanadium files "
                       "have been supplied.");
    return;
  }

  if (instName == "OSIRIS") {
    if (mode == "diffonly") {
      if (!validateVanCal()) {
        showInformationBox("Vanadium and Calibration input is invalid.");
        return;
      }
      runOSIRISdiffonlyReduction();
    } else {
      if (!validateCalOnly()) {
        showInformationBox(
            "Calibration and rebinning parameters are incorrect.");
        return;
      }
      runGenericReduction(instName, mode);
    }
  } else {
    if (!validateRebin()) {
      showInformationBox("Rebinning parameters are incorrect.");
      return;
    }
    runGenericReduction(instName, mode);
  }
}

/**
 * Handles completion of algorithm
 *
 * @param error True if the chain was stopped due to error
 */
void IndirectDiffractionReduction::algorithmComplete(bool error) {
  // Handles completion of the diffraction algorithm chain
  disconnect(m_batchAlgoRunner, nullptr, this, SLOT(algorithmComplete(bool)));

  // Delete grouping workspace, if created.
  if (AnalysisDataService::Instance().doesExist(m_groupingWsName)) {
    deleteGroupingWorkspace();
  }

  setRunIsRunning(false);

  if (!error) {
    // Ungroup the output workspace if generic reducer was used
    if (AnalysisDataService::Instance().doesExist(
            "IndirectDiffraction_Workspaces")) {
      WorkspaceGroup_sptr diffResultsGroup =
          AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
              "IndirectDiffraction_Workspaces");

      m_plotWorkspaces.clear();
      m_plotWorkspaces = diffResultsGroup->getNames();

      diffResultsGroup->removeAll();
      AnalysisDataService::Instance().remove("IndirectDiffraction_Workspaces");
    }
  } else {
    setPlotEnabled(false);
    setSaveEnabled(false);
    showInformationBox(
        "Error running diffraction reduction.\nSee Results Log for details.");
  }
}

/**
 * Handles plotting result spectra from algorithm chains.
 */
void IndirectDiffractionReduction::plotResults() {
  setPlotIsPlotting(true);
  const QString plotType = m_uiForm.cbPlotType->currentText();

  QString pyInput = "from mantidplot import plotSpectrum, plot2D\n";

  if (plotType == "Spectra" || plotType == "Both") {
    for (const auto &it : m_plotWorkspaces) {
      const auto workspaceExists =
          AnalysisDataService::Instance().doesExist(it);
      if (workspaceExists)
        pyInput += "plotSpectrum('" + QString::fromStdString(it) + "', 0)\n";
      else
        showInformationBox(QString::fromStdString(
            "Workspace '" + it + "' not found\nUnable to plot workspace"));
    }
  }

  if (plotType == "Contour" || plotType == "Both") {
    for (const auto &it : m_plotWorkspaces) {
      const auto workspaceExists =
          AnalysisDataService::Instance().doesExist(it);
      if (workspaceExists)
        pyInput += "plot2D('" + QString::fromStdString(it) + "')\n";
      else
        showInformationBox(QString::fromStdString(
            "Workspace '" + it + "' not found\nUnable to plot workspace"));
    }
  }

  runPythonCode(pyInput);

  setPlotIsPlotting(false);
}

/**
 * Handles saving the reductions from the generic algorithm.
 */
void IndirectDiffractionReduction::saveReductions() {
  for (const auto &wsName : m_plotWorkspaces) {
    const auto workspaceExists =
        AnalysisDataService::Instance().doesExist(wsName);
    if (workspaceExists) {
      auto workspace =
          AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);

      if (m_uiForm.ckGSS->isChecked()) {
        std::string tofWsName = wsName;

        if (workspace->YUnit() != "TOF") {
          tofWsName = wsName + "_tof";
          m_batchAlgoRunner->addAlgorithm(
              convertUnitsAlgorithm(wsName, tofWsName, "TOF"));
        }

        BatchAlgorithmRunner::AlgorithmRuntimeProps runtimeInput;
        runtimeInput["InputWorkspace"] = tofWsName;
        m_batchAlgoRunner->addAlgorithm(saveGSSAlgorithm(wsName + ".gss"),
                                        runtimeInput);
      }

      if (m_uiForm.ckNexus->isChecked()) {
        // Save NEXus using SaveNexusProcessed
        m_batchAlgoRunner->addAlgorithm(
            saveNexusProcessedAlgorithm(wsName + ".nxs", wsName));
      }

      if (m_uiForm.ckAscii->isChecked()) {
        // Save ASCII using SaveAscii version 1
        m_batchAlgoRunner->addAlgorithm(
            saveASCIIAlgorithm(wsName + ".dat", wsName));
      }
    } else
      showInformationBox(QString::fromStdString(
          "Workspace '" + wsName + "' not found\nUnable to plot workspace"));
  }
  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Creates an algorithm for saving the workspace with the specified name
 * in GSS format into the file with the specified name.
 *
 * @param filename    The name of the file to save to.
 * @param inputWsName The name of the workspace to save.
 * @return            A SaveGSS Algorithm which saves in file with the
 *                    specified name.
 */
IAlgorithm_sptr
IndirectDiffractionReduction::saveGSSAlgorithm(const std::string &filename) {
  auto alg = saveAlgorithm("SaveGSS", filename);
  alg->setProperty("Append", false);
  return alg;
}

/**
 * Creates an algorithm for saving the workspace with the specified name
 * in ASCII format into the file with the specified name.
 *
 * @param filename    The name of the file to save to.
 * @param inputWsName The name of the workspace to save.
 * @return            A SaveASCII Algorithm which saves in file with the
 *                    specified name.
 */
IAlgorithm_sptr IndirectDiffractionReduction::saveASCIIAlgorithm(
    const std::string &filename, const std::string &inputWsName) {
  return saveAlgorithm("SaveAscii", filename, inputWsName, 1);
}

/**
 * Creates an algorithm for saving the workspace with the specified name
 * in NexusProcessed format into the file with the specified name.
 *
 * @param filename    The name of the file to save to.
 * @param inputWsName The name of the workspace to save.
 * @return            A NexusProcessed Algorithm which saves in file with
 *                    the specified name.
 */
IAlgorithm_sptr IndirectDiffractionReduction::saveNexusProcessedAlgorithm(
    const std::string &filename, const std::string &inputWsName) {
  return saveAlgorithm("SaveNexusProcessed", filename, inputWsName);
}

/**
 * Creates a save algorithm with the specified name for saving the
 * workspace with the specified name into the file with the specified name.
 *
 * @param saveAlgName The name of the save algorithm to use.
 * @param filename    The name of the file to save to.
 * @param inputWsName The name of the workspace to save.
 * @param version     The version of the save algorithm to use.
 * @return            A Save algorithm for saving the workspace with
 *                    the specified name into the file with the the
 *                    specified name.
 */
IAlgorithm_sptr IndirectDiffractionReduction::saveAlgorithm(
    const std::string &saveAlgName, const std::string &filename,
    const std::string &inputWsName, const int &version) {
  IAlgorithm_sptr saveAlg =
      AlgorithmManager::Instance().create(saveAlgName, version);
  saveAlg->initialize();

  if (inputWsName != "") {
    saveAlg->setProperty("InputWorkspace", inputWsName);
  }
  saveAlg->setProperty("Filename", filename);
  return saveAlg;
}

/**
 * Creates and algorithm for converting the units of the input workspace
 * with the specified name, to the specified target, storing the result
 * in an output workspace, with the specified name.
 *
 * @param inputWsName   The name of the input workspace, on which to
 *                      perform the unit conversion.
 * @param outputWsName  The name of the output workspace, in which to
 *                      store the result of unit conversion.
 * @param target        The target units of the conversion algorithm.
 * @return              A unit conversion algorithm.
 */
IAlgorithm_sptr IndirectDiffractionReduction::convertUnitsAlgorithm(
    const std::string &inputWsName, const std::string &outputWsName,
    const std::string &target) {
  IAlgorithm_sptr convertUnits =
      AlgorithmManager::Instance().create("ConvertUnits");
  convertUnits->initialize();
  convertUnits->setProperty("InputWorkspace", inputWsName);
  convertUnits->setProperty("OutputWorkspace", outputWsName);
  convertUnits->setProperty("Target", target);
  return convertUnits;
}

/**
 * Runs a diffraction reduction for any instrument in any mode.
 *
 * @param instName Name of the instrument
 * @param mode Mode instrument is operating in (diffspec/diffonly)
 */
void IndirectDiffractionReduction::runGenericReduction(QString instName,
                                                       QString mode) {

  QString rebinStart = "";
  QString rebinWidth = "";
  QString rebinEnd = "";
  bool useManualGrouping = m_uiForm.ckManualGrouping->isChecked();

  // Get rebin string
  if (mode == "diffspec") {
    rebinStart = m_uiForm.leRebinStart_CalibOnly->text();
    rebinWidth = m_uiForm.leRebinWidth_CalibOnly->text();
    rebinEnd = m_uiForm.leRebinEnd_CalibOnly->text();
  } else if (mode == "diffonly") {
    rebinStart = m_uiForm.leRebinStart->text();
    rebinWidth = m_uiForm.leRebinWidth->text();
    rebinEnd = m_uiForm.leRebinEnd->text();
  }

  QString rebin = "";
  if (!rebinStart.isEmpty() && !rebinWidth.isEmpty() && !rebinEnd.isEmpty())
    rebin = rebinStart + "," + rebinWidth + "," + rebinEnd;

  // Get detector range
  std::vector<long> detRange;
  detRange.push_back(static_cast<long>(m_uiForm.spSpecMin->value()));
  detRange.push_back(static_cast<long>(m_uiForm.spSpecMax->value()));

  // Get generic reduction algorithm instance
  IAlgorithm_sptr msgDiffReduction =
      AlgorithmManager::Instance().create("ISISIndirectDiffractionReduction");
  msgDiffReduction->initialize();

  // Set algorithm properties
  msgDiffReduction->setProperty("Instrument", instName.toStdString());
  msgDiffReduction->setProperty("Mode", mode.toStdString());

  // Check if Cal file is used
  if (instName == "OSIRIS" && mode == "diffspec") {
    if (m_uiForm.ckUseCalib->isChecked()) {
      const auto calFile = m_uiForm.rfCalFile_only->getText().toStdString();
      msgDiffReduction->setProperty("CalFile", calFile);
    }
  }
  if (mode == "diffspec") {

    if (m_uiForm.ckUseVanadium->isChecked()) {
      const auto vanFile =
          m_uiForm.rfVanFile_only->getFilenames().join(",").toStdString();
      msgDiffReduction->setProperty("VanadiumFiles", vanFile);
    }
  }
  msgDiffReduction->setProperty("SumFiles", m_uiForm.ckSumFiles->isChecked());
  msgDiffReduction->setProperty("LoadLogFiles",
                                m_uiForm.ckLoadLogs->isChecked());
  msgDiffReduction->setProperty(
      "InputFiles", m_uiForm.rfSampleFiles->getText().toStdString());
  msgDiffReduction->setProperty("SpectraRange", detRange);
  msgDiffReduction->setProperty("RebinParam", rebin.toStdString());
  msgDiffReduction->setProperty("OutputWorkspace",
                                "IndirectDiffraction_Workspaces");

  if (m_uiForm.ckUseCan->isChecked()) {
    msgDiffReduction->setProperty(
        "ContainerFiles",
        m_uiForm.rfCanFiles->getFilenames().join(",").toStdString());
    if (m_uiForm.ckCanScale->isChecked())
      msgDiffReduction->setProperty("ContainerScaleFactor",
                                    m_uiForm.spCanScale->value());
  }

  BatchAlgorithmRunner::AlgorithmRuntimeProps diffRuntimeProps;
  m_groupingWsName = "__Grouping";
  // Add the property for grouping policy if needed
  if (useManualGrouping) {
    msgDiffReduction->setProperty("GroupingPolicy", "Workspace");
    createGroupingWorkspace(m_groupingWsName);
    diffRuntimeProps["GroupingWorkspace"] = m_groupingWsName;
  }
  m_batchAlgoRunner->addAlgorithm(msgDiffReduction, diffRuntimeProps);

  // Handles completion of the diffraction algorithm chain
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(algorithmComplete(bool)));

  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Runs a diffraction reduction for OSIRIS operating in diffonly mode using the
 * OSIRISDiffractionReduction algorithm.
 */
void IndirectDiffractionReduction::runOSIRISdiffonlyReduction() {
  // Get the files names from MWRunFiles widget, and convert them from Qt forms
  // into stl equivalents.
  QStringList fileNames = m_uiForm.rfSampleFiles->getFilenames();
  std::vector<std::string> stlFileNames;
  stlFileNames.reserve(fileNames.size());
  std::transform(fileNames.begin(), fileNames.end(),
                 std::back_inserter(stlFileNames), toStdString);

  // Use the file names to suggest a workspace name to use.  Report to logger
  // and stop if unable to parse correctly.
  QString drangeWsName;
  QString tofWsName;
  try {
    QString nameBase = QString::fromStdString(
        Mantid::Kernel::MultiFileNameParsing::suggestWorkspaceName(
            stlFileNames));
    tofWsName = nameBase + "_tof";
    drangeWsName = nameBase + "_dRange";
  } catch (std::runtime_error &re) {
    g_log.error(re.what());
    return;
  }

  IAlgorithm_sptr osirisDiffReduction =
      AlgorithmManager::Instance().create("OSIRISDiffractionReduction");
  osirisDiffReduction->initialize();
  osirisDiffReduction->setProperty(
      "Sample", m_uiForm.rfSampleFiles->getFilenames().join(",").toStdString());
  osirisDiffReduction->setProperty(
      "Vanadium",
      m_uiForm.rfVanadiumFile->getFilenames().join(",").toStdString());
  osirisDiffReduction->setProperty(
      "CalFile", m_uiForm.rfCalFile->getFirstFilename().toStdString());
  osirisDiffReduction->setProperty("LoadLogFiles",
                                   m_uiForm.ckLoadLogs->isChecked());
  osirisDiffReduction->setProperty("OutputWorkspace",
                                   drangeWsName.toStdString());
  auto specMin =
      boost::lexical_cast<std::string, int>(m_uiForm.spSpecMin->value());
  auto specMax =
      boost::lexical_cast<std::string, int>(m_uiForm.spSpecMax->value());
  osirisDiffReduction->setProperty("SpectraMin", specMin);
  osirisDiffReduction->setProperty("SpectraMax", specMax);

  if (m_uiForm.ckUseCan->isChecked()) {
    osirisDiffReduction->setProperty(
        "Container",
        m_uiForm.rfCanFiles->getFilenames().join(",").toStdString());
    if (m_uiForm.ckCanScale->isChecked())
      osirisDiffReduction->setProperty("ContainerScaleFactor",
                                       m_uiForm.spCanScale->value());
  }

  m_batchAlgoRunner->addAlgorithm(osirisDiffReduction);

  BatchAlgorithmRunner::AlgorithmRuntimeProps inputFromReductionProps;
  inputFromReductionProps["InputWorkspace"] = drangeWsName.toStdString();

  IAlgorithm_sptr convertUnits =
      AlgorithmManager::Instance().create("ConvertUnits");
  convertUnits->initialize();
  convertUnits->setProperty("OutputWorkspace", tofWsName.toStdString());
  convertUnits->setProperty("Target", "TOF");
  m_batchAlgoRunner->addAlgorithm(convertUnits, inputFromReductionProps);

  m_plotWorkspaces.clear();
  m_plotWorkspaces.push_back(tofWsName.toStdString());
  m_plotWorkspaces.push_back(drangeWsName.toStdString());

  // Handles completion of the diffraction algorithm chain
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(algorithmComplete(bool)));

  m_batchAlgoRunner->executeBatchAsync();
}

void IndirectDiffractionReduction::createGroupingWorkspace(
    const std::string &outputWsName) {
  auto instrumentConfig = m_uiForm.iicInstrumentConfiguration;
  auto const numberOfGroups = m_uiForm.spNumberGroups->value();
  auto const instrument = instrumentConfig->getInstrumentName().toStdString();
  auto const analyser = instrumentConfig->getAnalyserName().toStdString();
  auto const componentName = analyser == "diffraction" ? "bank" : analyser;

  auto groupingAlg =
      AlgorithmManager::Instance().create("CreateGroupingWorkspace");
  groupingAlg->initialize();
  groupingAlg->setProperty("FixedGroupCount", numberOfGroups);
  groupingAlg->setProperty("InstrumentName", instrument);
  groupingAlg->setProperty("ComponentName", componentName);
  groupingAlg->setProperty("OutputWorkspace", outputWsName);

  m_batchAlgoRunner->addAlgorithm(groupingAlg);
}

void IndirectDiffractionReduction::deleteGroupingWorkspace() {
  IAlgorithm_sptr deleteAlg =
      AlgorithmManager::Instance().create("DeleteWorkspace");
  deleteAlg->initialize();
  deleteAlg->setProperty("Workspace", m_groupingWsName);
  deleteAlg->executeAsync();
  m_groupingWsName = "";
}

/**
 * Loads an empty instrument and returns a pointer to the workspace.
 *
 * Optionally loads an IPF if a reflection was provided.
 *
 * @param instrumentName Name of an inelastic indirect instrument (IRIS, OSIRIS,
 *TOSCA, VESUVIO)
 * @param reflection Reflection mode to load parameters for (diffspec or
 *diffonly)
 */
MatrixWorkspace_sptr
IndirectDiffractionReduction::loadInstrument(const std::string &instrumentName,
                                             const std::string &reflection) {
  std::string idfPath = Mantid::Kernel::ConfigService::Instance().getString(
      "instrumentDefinition.directory");

  std::string parameterFilename = idfPath + instrumentName + "_Definition.xml";
  IAlgorithm_sptr loadAlg =
      AlgorithmManager::Instance().create("LoadEmptyInstrument");
  loadAlg->setChild(true);
  loadAlg->initialize();
  loadAlg->setProperty("Filename", parameterFilename);
  loadAlg->setProperty("OutputWorkspace", "__InDiff_Inst");
  loadAlg->execute();
  MatrixWorkspace_sptr instWorkspace = loadAlg->getProperty("OutputWorkspace");

  // Load parameter file if a reflection was given
  if (!reflection.empty()) {
    std::string ipfFilename = idfPath + instrumentName + "_diffraction_" +
                              reflection + "_Parameters.xml";
    IAlgorithm_sptr loadParamAlg =
        AlgorithmManager::Instance().create("LoadParameterFile");
    loadParamAlg->setChild(true);
    loadParamAlg->initialize();
    loadParamAlg->setProperty("Filename", ipfFilename);
    loadParamAlg->setProperty("Workspace", instWorkspace);
    loadParamAlg->execute();
  }

  return instWorkspace;
}

/**
 * Handles setting default spectra range when an instrument configuration is
 *selected.
 *
 * @param instrumentName Name of selected instrument
 * @param analyserName Name of selected analyser (should always be
 *"diffraction")
 * @param reflectionName Name of diffraction mode selected
 */
void IndirectDiffractionReduction::instrumentSelected(
    const QString &instrumentName, const QString &analyserName,
    const QString &reflectionName) {
  UNUSED_ARG(analyserName);

  // Set the search instrument for runs
  m_uiForm.rfSampleFiles->setInstrumentOverride(instrumentName);
  m_uiForm.rfCanFiles->setInstrumentOverride(instrumentName);
  m_uiForm.rfVanadiumFile->setInstrumentOverride(instrumentName);
  m_uiForm.rfCalFile_only->setInstrumentOverride(instrumentName);
  m_uiForm.rfVanFile_only->setInstrumentOverride(instrumentName);

  MatrixWorkspace_sptr instWorkspace = loadInstrument(
      instrumentName.toStdString(), reflectionName.toStdString());
  Instrument_const_sptr instrument = instWorkspace->getInstrument();

  // Get default spectra range
  double specMin = instrument->getNumberParameter("spectra-min")[0];
  double specMax = instrument->getNumberParameter("spectra-max")[0];

  m_uiForm.spSpecMin->setMinimum(static_cast<int>(specMin));
  m_uiForm.spSpecMin->setMaximum(static_cast<int>(specMax));
  m_uiForm.spSpecMax->setMinimum(static_cast<int>(specMin));
  m_uiForm.spSpecMax->setMaximum(static_cast<int>(specMax));

  m_uiForm.spSpecMin->setValue(static_cast<int>(specMin));
  m_uiForm.spSpecMax->setValue(static_cast<int>(specMax));

  // Determine whether we need vanadium input
  std::vector<std::string> correctionVector =
      instrument->getStringParameter("Workflow.Diffraction.Correction");
  bool vanadiumNeeded = false;
  bool calibNeeded = false;
  if (correctionVector.size() > 0) {
    vanadiumNeeded = (correctionVector[0] == "Vanadium");
    calibNeeded = (correctionVector[0] == "Calibration");
  }

  if (vanadiumNeeded)
    m_uiForm.swVanadium->setCurrentIndex(0);
  else if (calibNeeded)
    m_uiForm.swVanadium->setCurrentIndex(1);
  else if (reflectionName != "diffspec")
    m_uiForm.swVanadium->setCurrentIndex(2);
  else
    m_uiForm.swVanadium->setCurrentIndex(1);

  // Hide options that the current instrument config cannot process

  // Disable calibration for IRIS
  if (instrumentName == "IRIS") {
    m_uiForm.ckUseCalib->setEnabled(false);
    m_uiForm.ckUseCalib->setToolTip("IRIS does not support calibration files");
    m_uiForm.ckUseCalib->setChecked(false);
  } else {
    m_uiForm.ckUseCalib->setEnabled(true);
    m_uiForm.ckUseCalib->setToolTip("");
    m_uiForm.ckUseCalib->setChecked(true);
  }

  if (instrumentName == "OSIRIS" && reflectionName == "diffonly") {
    // Disable individual grouping
    m_uiForm.ckManualGrouping->setToolTip(
        "OSIRIS cannot group detectors individually in diffonly mode");
    m_uiForm.ckManualGrouping->setEnabled(false);
    m_uiForm.ckManualGrouping->setChecked(false);

    // Disable sum files
    m_uiForm.ckSumFiles->setToolTip("OSIRIS cannot sum files in diffonly mode");
    m_uiForm.ckSumFiles->setEnabled(false);
    m_uiForm.ckSumFiles->setChecked(false);

  } else {
    // Re-enable sum files
    m_uiForm.ckSumFiles->setToolTip("");
    m_uiForm.ckSumFiles->setEnabled(true);
    m_uiForm.ckSumFiles->setChecked(true);

    // Re-enable individual grouping
    m_uiForm.ckManualGrouping->setToolTip("");
    m_uiForm.ckManualGrouping->setEnabled(true);

    // Re-enable spectra range
    m_uiForm.spSpecMin->setEnabled(true);
    m_uiForm.spSpecMax->setEnabled(true);
  }
}

void IndirectDiffractionReduction::validateSpectrumMin(int value) {
  MantidQt::API::SignalBlocker blocker(m_uiForm.spSpecMin);

  auto const spectraMax = m_uiForm.spSpecMax->value();
  if (value > spectraMax)
    m_uiForm.spSpecMin->setValue(spectraMax);
}

void IndirectDiffractionReduction::validateSpectrumMax(int value) {
  MantidQt::API::SignalBlocker blocker(m_uiForm.spSpecMax);

  auto const spectraMin = m_uiForm.spSpecMin->value();
  if (value < spectraMin)
    m_uiForm.spSpecMax->setValue(spectraMin);
}

std::string IndirectDiffractionReduction::documentationPage() const {
  return "Indirect Diffraction";
}

void IndirectDiffractionReduction::initLocalPython() {}

void IndirectDiffractionReduction::loadSettings() {
  QSettings settings;
  QString dataDir = QString::fromStdString(
                        Mantid::Kernel::ConfigService::Instance().getString(
                            "datasearch.directories"))
                        .split(";")[0];

  settings.beginGroup(m_settingsGroup);
  settings.setValue("last_directory", dataDir);
  m_uiForm.rfSampleFiles->readSettings(settings.group());
  m_uiForm.rfCalFile->readSettings(settings.group());
  m_uiForm.rfCalFile->setUserInput(settings.value("last_cal_file").toString());
  m_uiForm.rfVanadiumFile->setUserInput(
      settings.value("last_van_files").toString());
  settings.endGroup();
}

void IndirectDiffractionReduction::saveSettings() {
  QSettings settings;

  settings.beginGroup(m_settingsGroup);
  settings.setValue("last_cal_file", m_uiForm.rfCalFile->getText());
  settings.setValue("last_van_files", m_uiForm.rfVanadiumFile->getText());
  settings.endGroup();
}

/**
 * Validates the rebinning fields and updates invalid markers.
 *
 * @returns True if reining options are valid, false otherwise
 */
bool IndirectDiffractionReduction::validateRebin() {
  QString rebStartTxt = m_uiForm.leRebinStart->text();
  QString rebStepTxt = m_uiForm.leRebinWidth->text();
  QString rebEndTxt = m_uiForm.leRebinEnd->text();

  bool rebinValid = true;
  // Need all or none
  if (rebStartTxt.isEmpty() && rebStepTxt.isEmpty() && rebEndTxt.isEmpty()) {
    rebinValid = true;
    m_uiForm.valRebinStart->setText("");
    m_uiForm.valRebinWidth->setText("");
    m_uiForm.valRebinEnd->setText("");
  } else {
#define CHECK_VALID(text, validator)                                           \
  rebinValid = !text.isEmpty();                                                \
  if (!rebinValid) {                                                           \
    validator->setText("*");                                                   \
  } else {                                                                     \
    validator->setText("");                                                    \
  }

    CHECK_VALID(rebStartTxt, m_uiForm.valRebinStart);
    CHECK_VALID(rebStepTxt, m_uiForm.valRebinWidth);
    CHECK_VALID(rebEndTxt, m_uiForm.valRebinEnd);

    if (rebinValid && rebStartTxt.toDouble() >= rebEndTxt.toDouble()) {
      rebinValid = false;
      m_uiForm.valRebinStart->setText("*");
      m_uiForm.valRebinEnd->setText("*");
    }
  }

  return rebinValid;
}

/**
 * Checks to see if the vanadium and cal file fields are valid.
 *
 * @returns True fo vanadium and calibration files are valid, false otherwise
 */
bool IndirectDiffractionReduction::validateVanCal() {
  if (!m_uiForm.rfCalFile->isValid())
    return false;

  if (!m_uiForm.rfVanadiumFile->isValid())
    return false;

  return true;
}

/**
 * Checks to see if the cal file and optional rebin fields are valid.
 *
 * @returns True if calibration file and rebin values are valid, false otherwise
 */
bool IndirectDiffractionReduction::validateCalOnly() {
  // Check Calib file valid
  if (m_uiForm.ckUseCalib->isChecked() && !m_uiForm.rfCalFile_only->isValid())
    return false;

  // Check rebin values valid
  QString rebStartTxt = m_uiForm.leRebinStart_CalibOnly->text();
  QString rebStepTxt = m_uiForm.leRebinWidth_CalibOnly->text();
  QString rebEndTxt = m_uiForm.leRebinEnd_CalibOnly->text();

  bool rebinValid = true;
  // Need all or none
  if (rebStartTxt.isEmpty() && rebStepTxt.isEmpty() && rebEndTxt.isEmpty()) {
    rebinValid = true;
    m_uiForm.valRebinStart_CalibOnly->setText("");
    m_uiForm.valRebinWidth_CalibOnly->setText("");
    m_uiForm.valRebinEnd_CalibOnly->setText("");
  } else {

    CHECK_VALID(rebStartTxt, m_uiForm.valRebinStart_CalibOnly);
    CHECK_VALID(rebStepTxt, m_uiForm.valRebinWidth_CalibOnly);
    CHECK_VALID(rebEndTxt, m_uiForm.valRebinEnd_CalibOnly);

    if (rebinValid && rebStartTxt.toDouble() >= rebEndTxt.toDouble()) {
      rebinValid = false;
      m_uiForm.valRebinStart_CalibOnly->setText("*");
      m_uiForm.valRebinEnd_CalibOnly->setText("*");
    }
  }

  return rebinValid;
}

/**
 * Disables and shows message on run button indicating that run files have been
 * changed.
 */
void IndirectDiffractionReduction::runFilesChanged() {
  m_uiForm.pbRun->setEnabled(false);
  m_uiForm.pbRun->setText("Editing...");
}

/**
 * Disables and shows message on run button to indicate searching for data
 * files.
 */
void IndirectDiffractionReduction::runFilesFinding() {
  m_uiForm.pbRun->setEnabled(false);
  m_uiForm.pbRun->setText("Finding files...");
}

/**
 * Updates run button with result of file search.
 */
void IndirectDiffractionReduction::runFilesFound() {
  bool valid = m_uiForm.rfSampleFiles->isValid();
  m_uiForm.pbRun->setEnabled(valid);

  if (valid)
    m_uiForm.pbRun->setText("Run");
  else
    m_uiForm.pbRun->setText("Invalid Run");

  // Disable sum files if only one file is given
  int fileCount = m_uiForm.rfSampleFiles->getFilenames().size();
  if (fileCount < 2)
    m_uiForm.ckSumFiles->setChecked(false);
}

/**
 * Handles the user toggling the manual grouping check box.
 *
 * @param state The selection state of the check box
 */
void IndirectDiffractionReduction::manualGroupingToggled(int state) {
  int itemCount = m_uiForm.cbPlotType->count();

  switch (state) {
  case Qt::Unchecked:
    if (itemCount == 3) {
      m_uiForm.cbPlotType->removeItem(1);
      m_uiForm.cbPlotType->removeItem(2);
    }
    break;

  case Qt::Checked:
    if (itemCount == 1) {
      m_uiForm.cbPlotType->insertItem(1, "Contour");
      m_uiForm.cbPlotType->insertItem(2, "Both");
    }
    break;

  default:
    return;
  }
}

void IndirectDiffractionReduction::setRunIsRunning(bool running) {
  m_uiForm.pbRun->setText(running ? "Running..." : "Run");
  setButtonsEnabled(!running);
}

void IndirectDiffractionReduction::setPlotIsPlotting(bool plotting) {
  m_uiForm.pbPlot->setText(plotting ? "Plotting..." : "Plot");
  setButtonsEnabled(!plotting);
}

void IndirectDiffractionReduction::setButtonsEnabled(bool enabled) {
  setRunEnabled(enabled);
  setPlotEnabled(enabled);
  setSaveEnabled(enabled);
}

void IndirectDiffractionReduction::setRunEnabled(bool enabled) {
  m_uiForm.pbRun->setEnabled(enabled);
}

void IndirectDiffractionReduction::setPlotEnabled(bool enabled) {
  m_uiForm.pbPlot->setEnabled(enabled);
  m_uiForm.cbPlotType->setEnabled(enabled);
}

void IndirectDiffractionReduction::setSaveEnabled(bool enabled) {
  m_uiForm.pbSave->setEnabled(enabled);
  m_uiForm.ckAscii->setEnabled(enabled);
  m_uiForm.ckGSS->setEnabled(enabled);
  m_uiForm.ckNexus->setEnabled(enabled);
}

} // namespace CustomInterfaces
} // namespace MantidQt
