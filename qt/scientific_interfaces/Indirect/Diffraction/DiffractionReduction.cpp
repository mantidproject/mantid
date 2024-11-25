// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "DiffractionReduction.h"
#include "Common/DetectorGroupingOptions.h"
#include "MantidQtWidgets/Spectroscopy/RunWidget/RunView.h"
#include "MantidQtWidgets/Spectroscopy/SettingsWidget/Settings.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AlgorithmRuntimeProps.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/MultiFileNameParser.h"

#include <QSignalBlocker>

using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace MantidQt::CustomInterfaces {

namespace {
Mantid::Kernel::Logger g_log("DiffractionReduction");

// Helper function for use with std::transform.
std::string toStdString(const QString &qString) { return qString.toStdString(); }

} // namespace

DECLARE_SUBWINDOW(DiffractionReduction)

using MantidQt::API::BatchAlgorithmRunner;

DiffractionReduction::DiffractionReduction(QWidget *parent)
    : InelasticInterface(parent), m_valDbl(nullptr), m_settingsGroup("CustomInterfaces/DEMON"),
      m_batchAlgoRunner(new BatchAlgorithmRunner(parent)), m_plotWorkspaces(), m_runPresenter(),
      m_plotOptionsPresenter(), m_groupingWidget() {}

DiffractionReduction::~DiffractionReduction() { saveSettings(); }

/**
 * Sets up UI components and Qt signal/slot connections.
 */
void DiffractionReduction::initLayout() {
  m_uiForm.setupUi(this);
  m_uiForm.pbSettings->setIcon(Settings::icon());

  m_runPresenter = std::make_unique<RunPresenter>(this, m_uiForm.runWidget);
  auto outputPlotOptionsModel = std::make_unique<OutputPlotOptionsModel>(std::make_unique<ExternalPlotter>());
  m_plotOptionsPresenter = std::make_unique<OutputPlotOptionsPresenter>(
      m_uiForm.ipoPlotOptions, std::move(outputPlotOptionsModel), PlotWidget::SpectraUnit, "0");

  m_groupingWidget = new DetectorGroupingOptions(m_uiForm.fDetectorGrouping);
  m_uiForm.fDetectorGrouping->layout()->addWidget(m_groupingWidget);
  m_groupingWidget->setSaveCustomVisible(false);
  m_groupingWidget->removeGroupingMethod("Individual");
  m_groupingWidget->removeGroupingMethod("IPF");
  m_groupingWidget->setGroupingMethod("All");

  connect(m_uiForm.pbSettings, SIGNAL(clicked()), this, SLOT(settings()));
  connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(help()));
  connect(m_uiForm.pbManageDirs, SIGNAL(clicked()), this, SLOT(manageUserDirectories()));

  connect(m_uiForm.iicInstrumentConfiguration,
          SIGNAL(instrumentConfigurationUpdated(const QString &, const QString &, const QString &)), this,
          SLOT(instrumentSelected(const QString &, const QString &, const QString &)));

  connect(m_uiForm.spSpecMin, SIGNAL(valueChanged(int)), this, SLOT(validateSpectrumMin(int)));
  connect(m_uiForm.spSpecMax, SIGNAL(valueChanged(int)), this, SLOT(validateSpectrumMax(int)));

  // Update run button based on state of raw files field
  connectRunButtonValidation(m_uiForm.rfSampleFiles);
  connectRunButtonValidation(m_uiForm.rfCanFiles);
  connectRunButtonValidation(m_uiForm.rfCalFile);

  connect(m_uiForm.ckUseVanadium, SIGNAL(stateChanged(int)), this, SLOT(useVanadiumStateChanged(int)));
  connect(m_uiForm.ckUseCalib, SIGNAL(stateChanged(int)), this, SLOT(useCalibStateChanged(int)));

  m_valDbl = new QDoubleValidator(this);

  m_uiForm.leRebinStart->setValidator(m_valDbl);
  m_uiForm.leRebinWidth->setValidator(m_valDbl);
  m_uiForm.leRebinEnd->setValidator(m_valDbl);

  // Handle saving
  connect(m_uiForm.pbSave, SIGNAL(clicked()), this, SLOT(saveReductions()));

  loadSettings();

  // Update invalid rebinning markers
  validateRebin();

  // Update instrument dependant widgets
  m_uiForm.iicInstrumentConfiguration->updateInstrumentConfigurations(
      m_uiForm.iicInstrumentConfiguration->getInstrumentName());
}

/**
 * Make file finding status display on the run button and enable/disable it
 */
void DiffractionReduction::connectRunButtonValidation(const MantidQt::API::FileFinderWidget *file_field) {
  connect(file_field, SIGNAL(fileTextChanged(const QString &)), this, SLOT(runFilesChanged()));
  connect(file_field, SIGNAL(findingFiles()), this, SLOT(runFilesFinding()));
  connect(file_field, SIGNAL(fileFindingFinished()), this, SLOT(runFilesFound()));
}

void DiffractionReduction::handleValidation(IUserInputValidator *validator) const {
  auto const sampleProblem = validateFileFinder(m_uiForm.rfSampleFiles);
  if (!sampleProblem.empty()) {
    validator->addErrorMessage("Sample: " + sampleProblem);
  }

  auto const vanadiumProblem = validateFileFinder(m_uiForm.rfVanFile, m_uiForm.ckUseVanadium->isChecked());
  if (!vanadiumProblem.empty()) {
    validator->addErrorMessage("Vanadium: " + vanadiumProblem);
  }

  auto const calibrationProblem = validateFileFinder(m_uiForm.rfCalFile, m_uiForm.ckUseCalib->isChecked());
  if (!calibrationProblem.empty()) {
    validator->addErrorMessage("Calibration: " + calibrationProblem);
  }

  auto const spectraMin = static_cast<std::size_t>(m_uiForm.spSpecMin->value());
  auto const spectraMax = static_cast<std::size_t>(m_uiForm.spSpecMax->value());
  if (auto const message = m_groupingWidget->validateGroupingProperties(spectraMin, spectraMax)) {
    validator->addErrorMessage(*message);
  }

  auto const &instName = m_uiForm.iicInstrumentConfiguration->getInstrumentName();
  auto const &mode = m_uiForm.iicInstrumentConfiguration->getReflectionName();

  if (instName != "OSIRIS" || mode != "diffonly") {
    if (!validateRebin()) {
      validator->addErrorMessage("Rebinning parameters are incorrect.");
    }
  }
}

void DiffractionReduction::handleRun() {
  m_plotOptionsPresenter->clearWorkspaces();

  auto const &instName = m_uiForm.iicInstrumentConfiguration->getInstrumentName();
  auto const &mode = m_uiForm.iicInstrumentConfiguration->getReflectionName();

  if (instName == "OSIRIS" && mode == "diffonly") {
    runOSIRISdiffonlyReduction();
  } else {
    runGenericReduction(instName, mode);
  }
}

/**
 * Handles completion of algorithm
 *
 * @param error True if the chain was stopped due to error
 */
void DiffractionReduction::algorithmComplete(bool error) {
  // Handles completion of the diffraction algorithm chain
  disconnect(m_batchAlgoRunner, nullptr, this, SLOT(algorithmComplete(bool)));

  m_runPresenter->setRunEnabled(true);
  setSaveEnabled(!error);

  if (!error) {
    // Ungroup the output workspace if generic reducer was used
    if (AnalysisDataService::Instance().doesExist("IndirectDiffraction_Workspaces")) {
      WorkspaceGroup_sptr diffResultsGroup =
          AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("IndirectDiffraction_Workspaces");

      m_plotWorkspaces.clear();
      m_plotWorkspaces = diffResultsGroup->getNames();

      diffResultsGroup->removeAll();
      AnalysisDataService::Instance().remove("IndirectDiffraction_Workspaces");

      m_plotOptionsPresenter->setWorkspaces(m_plotWorkspaces);
    }
  } else {
    showInformationBox("Error running diffraction reduction.\nSee Results Log for details.");
  }
}

/**
 * Handles saving the reductions from the generic algorithm.
 */
void DiffractionReduction::saveReductions() {
  for (const auto &wsName : m_plotWorkspaces) {
    const auto workspaceExists = AnalysisDataService::Instance().doesExist(wsName);
    if (workspaceExists) {
      auto workspace = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);

      if (m_uiForm.ckGSS->isChecked()) {
        std::string tofWsName = wsName;

        if (workspace->YUnit() != "TOF") {
          tofWsName = wsName + "_tof";
          m_batchAlgoRunner->addAlgorithm(convertUnitsAlgorithm(wsName, tofWsName, "TOF"));
        }

        auto runtimeInput = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();
        runtimeInput->setPropertyValue("InputWorkspace", tofWsName);
        m_batchAlgoRunner->addAlgorithm(saveGSSAlgorithm(wsName + ".gss"), std::move(runtimeInput));
      }

      if (m_uiForm.ckNexus->isChecked()) {
        // Save NEXus using SaveNexusProcessed
        m_batchAlgoRunner->addAlgorithm(saveNexusProcessedAlgorithm(wsName + ".nxs", wsName));
      }

      if (m_uiForm.ckAscii->isChecked()) {
        // Save ASCII using SaveAscii version 1
        m_batchAlgoRunner->addAlgorithm(saveASCIIAlgorithm(wsName + ".dat", wsName));
      }
    } else
      showInformationBox(QString::fromStdString("Workspace '" + wsName + "' not found\nUnable to plot workspace"));
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
IAlgorithm_sptr DiffractionReduction::saveGSSAlgorithm(const std::string &filename) {
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
IAlgorithm_sptr DiffractionReduction::saveASCIIAlgorithm(const std::string &filename, const std::string &inputWsName) {
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
IAlgorithm_sptr DiffractionReduction::saveNexusProcessedAlgorithm(const std::string &filename,
                                                                  const std::string &inputWsName) {
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
IAlgorithm_sptr DiffractionReduction::saveAlgorithm(const std::string &saveAlgName, const std::string &filename,
                                                    const std::string &inputWsName, const int &version) {
  IAlgorithm_sptr saveAlg = AlgorithmManager::Instance().create(saveAlgName, version);
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
IAlgorithm_sptr DiffractionReduction::convertUnitsAlgorithm(const std::string &inputWsName,
                                                            const std::string &outputWsName,
                                                            const std::string &target) {
  IAlgorithm_sptr convertUnits = AlgorithmManager::Instance().create("ConvertUnits");
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
void DiffractionReduction::runGenericReduction(const QString &instName, const QString &mode) {
  QString rebinStart = m_uiForm.leRebinStart->text();
  QString rebinWidth = m_uiForm.leRebinWidth->text();
  QString rebinEnd = m_uiForm.leRebinEnd->text();

  QString rebin = "";
  if (!rebinStart.isEmpty() && !rebinWidth.isEmpty() && !rebinEnd.isEmpty())
    rebin = rebinStart + "," + rebinWidth + "," + rebinEnd;

  // Get detector range
  std::vector<int> detRange;
  detRange.emplace_back(m_uiForm.spSpecMin->value());
  detRange.emplace_back(m_uiForm.spSpecMax->value());

  // Get generic reduction algorithm instance
  IAlgorithm_sptr msgDiffReduction = AlgorithmManager::Instance().create("ISISIndirectDiffractionReduction");
  msgDiffReduction->initialize();

  // Set algorithm properties
  msgDiffReduction->setProperty("Instrument", instName.toStdString());
  msgDiffReduction->setProperty("Mode", mode.toStdString());

  // Check if Cal file is used
  if (instName == "OSIRIS" && mode == "diffspec") {
    if (m_uiForm.ckUseCalib->isChecked()) {
      const auto calFile = m_uiForm.rfCalFile->getText().toStdString();
      msgDiffReduction->setProperty("CalFile", calFile);
    }
  }
  if (mode == "diffspec") {
    if (m_uiForm.ckUseVanadium->isChecked()) {
      const auto vanFile = m_uiForm.rfVanFile->getFilenames().join(",").toStdString();
      msgDiffReduction->setProperty("VanadiumFiles", vanFile);
    }
  }
  msgDiffReduction->setProperty("SumFiles", m_uiForm.ckSumFiles->isChecked());
  msgDiffReduction->setProperty("LoadLogFiles", m_uiForm.ckLoadLogs->isChecked());
  msgDiffReduction->setProperty("InputFiles", m_uiForm.rfSampleFiles->getText().toStdString());
  msgDiffReduction->setProperty("SpectraRange", detRange);
  msgDiffReduction->setProperty("RebinParam", rebin.toStdString());
  msgDiffReduction->setProperty("OutputWorkspace", "IndirectDiffraction_Workspaces");

  if (m_uiForm.ckUseCan->isChecked()) {
    msgDiffReduction->setProperty("ContainerFiles", m_uiForm.rfCanFiles->getFilenames().join(",").toStdString());
    if (m_uiForm.ckCanScale->isChecked())
      msgDiffReduction->setProperty("ContainerScaleFactor", m_uiForm.spCanScale->value());
  }

  auto groupingProps = m_groupingWidget->groupingProperties();
  m_batchAlgoRunner->addAlgorithm(msgDiffReduction, std::move(groupingProps));

  // Handles completion of the diffraction algorithm chain
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(algorithmComplete(bool)));

  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Runs a diffraction reduction for OSIRIS operating in diffonly mode using the
 * OSIRISDiffractionReduction algorithm.
 */
void DiffractionReduction::runOSIRISdiffonlyReduction() {
  // Get the files names from FileFinderWidget widget, and convert them from Qt
  // forms into stl equivalents.
  QStringList fileNames = m_uiForm.rfSampleFiles->getFilenames();
  std::vector<std::string> stlFileNames;
  stlFileNames.reserve(fileNames.size());
  std::transform(fileNames.begin(), fileNames.end(), std::back_inserter(stlFileNames), toStdString);

  // Use the file names to suggest a workspace name to use.  Report to logger
  // and stop if unable to parse correctly.
  QString drangeWsName;
  QString tofWsName;
  QString qWsName;
  try {
    QString nameBase = QString::fromStdString(Mantid::Kernel::MultiFileNameParsing::suggestWorkspaceName(stlFileNames));
    tofWsName = nameBase + "_tof";
    drangeWsName = nameBase + "_dRange";
    qWsName = nameBase + "_q";
  } catch (std::runtime_error &re) {
    g_log.error(re.what());
    return;
  }

  IAlgorithm_sptr osirisDiffReduction = AlgorithmManager::Instance().create("OSIRISDiffractionReduction");
  osirisDiffReduction->initialize();
  osirisDiffReduction->setProperty("Sample", m_uiForm.rfSampleFiles->getFilenames().join(",").toStdString());
  osirisDiffReduction->setProperty("Vanadium", m_uiForm.rfVanFile->getFilenames().join(",").toStdString());
  osirisDiffReduction->setProperty("CalFile", m_uiForm.rfCalFile->getFirstFilename().toStdString());
  osirisDiffReduction->setProperty("LoadLogFiles", m_uiForm.ckLoadLogs->isChecked());
  osirisDiffReduction->setProperty("OutputWorkspace", drangeWsName.toStdString());
  auto specMin = boost::lexical_cast<std::string, int>(m_uiForm.spSpecMin->value());
  auto specMax = boost::lexical_cast<std::string, int>(m_uiForm.spSpecMax->value());
  osirisDiffReduction->setProperty("SpectraMin", specMin);
  osirisDiffReduction->setProperty("SpectraMax", specMax);

  if (m_uiForm.ckUseCan->isChecked()) {
    osirisDiffReduction->setProperty("Container", m_uiForm.rfCanFiles->getFilenames().join(",").toStdString());
    if (m_uiForm.ckCanScale->isChecked())
      osirisDiffReduction->setProperty("ContainerScaleFactor", m_uiForm.spCanScale->value());
  }

  auto groupingProps = m_groupingWidget->groupingProperties();
  m_batchAlgoRunner->addAlgorithm(osirisDiffReduction, std::move(groupingProps));

  auto inputFromReductionProps = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();
  inputFromReductionProps->setPropertyValue("InputWorkspace", drangeWsName.toStdString());

  IAlgorithm_sptr tofConvertUnits = AlgorithmManager::Instance().create("ConvertUnits");
  tofConvertUnits->initialize();
  tofConvertUnits->setProperty("OutputWorkspace", tofWsName.toStdString());
  tofConvertUnits->setProperty("Target", "TOF");
  m_batchAlgoRunner->addAlgorithm(tofConvertUnits, std::move(inputFromReductionProps));

  inputFromReductionProps = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();
  inputFromReductionProps->setPropertyValue("InputWorkspace", drangeWsName.toStdString());

  IAlgorithm_sptr qConvertUnits = AlgorithmManager::Instance().create("ConvertUnits");
  qConvertUnits->initialize();
  qConvertUnits->setProperty("OutputWorkspace", qWsName.toStdString());
  qConvertUnits->setProperty("Target", "QSquared");
  m_batchAlgoRunner->addAlgorithm(qConvertUnits, std::move(inputFromReductionProps));

  m_plotWorkspaces.clear();
  m_plotWorkspaces.emplace_back(tofWsName.toStdString());
  m_plotWorkspaces.emplace_back(drangeWsName.toStdString());
  m_plotWorkspaces.emplace_back(qWsName.toStdString());

  // Handles completion of the diffraction algorithm chain
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(algorithmComplete(bool)));

  m_batchAlgoRunner->executeBatchAsync();
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
MatrixWorkspace_sptr DiffractionReduction::loadInstrument(const std::string &instrumentName,
                                                          const std::string &reflection) {
  std::string idfPath = Mantid::Kernel::ConfigService::Instance().getString("instrumentDefinition.directory");

  std::string parameterFilename = idfPath + instrumentName + "_Definition.xml";
  IAlgorithm_sptr loadAlg = AlgorithmManager::Instance().create("LoadEmptyInstrument");
  loadAlg->setChild(true);
  loadAlg->initialize();
  loadAlg->setProperty("Filename", parameterFilename);
  loadAlg->setProperty("OutputWorkspace", "__InDiff_Inst");
  loadAlg->execute();
  MatrixWorkspace_sptr instWorkspace = loadAlg->getProperty("OutputWorkspace");

  // Load parameter file if a reflection was given
  if (!reflection.empty()) {
    std::string ipfFilename = idfPath + instrumentName + "_diffraction_" + reflection + "_Parameters.xml";
    IAlgorithm_sptr loadParamAlg = AlgorithmManager::Instance().create("LoadParameterFile");
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
void DiffractionReduction::instrumentSelected(const QString &instrumentName, const QString &analyserName,
                                              const QString &reflectionName) {
  UNUSED_ARG(analyserName);

  // Set the search instrument for runs
  m_uiForm.rfSampleFiles->setInstrumentOverride(instrumentName);
  m_uiForm.rfCanFiles->setInstrumentOverride(instrumentName);
  m_uiForm.rfVanFile->setInstrumentOverride(instrumentName);

  MatrixWorkspace_sptr instWorkspace = loadInstrument(instrumentName.toStdString(), reflectionName.toStdString());
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

  // Require vanadium for OSIRIS diffonly
  auto vanadiumMandatory = instrumentName == "OSIRIS" && reflectionName == "diffonly";
  m_uiForm.rfVanFile->isOptional(!vanadiumMandatory);
  m_uiForm.ckUseVanadium->setChecked(vanadiumMandatory);
  m_uiForm.ckUseVanadium->setDisabled(vanadiumMandatory);

  // Hide calibration for non-OSIRIS instruments
  auto calibrationOptional = instrumentName == "OSIRIS";
  auto calibrationMandatory = calibrationOptional && reflectionName == "diffonly";
  m_uiForm.ckUseCalib->setVisible(calibrationOptional);
  m_uiForm.rfCalFile->setVisible(calibrationOptional);
  m_uiForm.rfCalFile->isOptional(!calibrationMandatory);
  m_uiForm.rfCalFile->setToolTip("Note: The calibration file will not be used for detector grouping unless explicitly "
                                 "selected in the 'File' grouping option below.");
  m_uiForm.ckUseCalib->setChecked(calibrationMandatory);
  m_uiForm.ckUseCalib->setDisabled(calibrationMandatory);

  // Hide rebin options for OSIRIS diffonly
  m_uiForm.gbDspaceRebinCalibOnly->setVisible(!(instrumentName == "OSIRIS" && reflectionName == "diffonly"));

  if (instrumentName == "OSIRIS" && reflectionName == "diffonly") {
    // Disable sum files
    m_uiForm.ckSumFiles->setToolTip("OSIRIS cannot sum files in diffonly mode");
    m_uiForm.ckSumFiles->setEnabled(false);
    m_uiForm.ckSumFiles->setChecked(false);

  } else {
    // Re-enable sum files
    m_uiForm.ckSumFiles->setToolTip("");
    m_uiForm.ckSumFiles->setEnabled(true);
    m_uiForm.ckSumFiles->setChecked(true);

    // Re-enable spectra range
    m_uiForm.spSpecMin->setEnabled(true);
    m_uiForm.spSpecMax->setEnabled(true);
  }
}

void DiffractionReduction::validateSpectrumMin(int value) {
  QSignalBlocker blocker(m_uiForm.spSpecMin);

  auto const spectraMax = m_uiForm.spSpecMax->value();
  if (value > spectraMax)
    m_uiForm.spSpecMin->setValue(spectraMax);
}

void DiffractionReduction::validateSpectrumMax(int value) {
  QSignalBlocker blocker(m_uiForm.spSpecMax);

  auto const spectraMin = m_uiForm.spSpecMin->value();
  if (value < spectraMin)
    m_uiForm.spSpecMax->setValue(spectraMin);
}

std::string DiffractionReduction::documentationPage() const { return "Indirect Diffraction"; }

void DiffractionReduction::initLocalPython() {}

void DiffractionReduction::loadSettings() {
  QSettings settings;
  QString dataDir =
      QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("datasearch.directories"))
          .split(";")[0];

  settings.beginGroup(m_settingsGroup);
  settings.setValue("last_directory", dataDir);
  m_uiForm.rfSampleFiles->readSettings(settings.group());
  m_uiForm.rfCalFile->setUserInput(settings.value("last_cal_file").toString());
  m_uiForm.rfVanFile->setUserInput(settings.value("last_van_files").toString());
  settings.endGroup();
}

void DiffractionReduction::saveSettings() {
  QSettings settings;

  settings.beginGroup(m_settingsGroup);
  settings.setValue("last_cal_file", m_uiForm.rfCalFile->getText());
  settings.setValue("last_van_files", m_uiForm.rfVanFile->getText());
  settings.endGroup();
}

/**
 * Validates the rebinning fields and updates invalid markers.
 *
 * @returns True if reining options are valid, false otherwise
 */
bool DiffractionReduction::validateRebin() const {
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
#define CHECK_VALID(text, validator)                                                                                   \
  rebinValid = !text.isEmpty();                                                                                        \
  if (!rebinValid) {                                                                                                   \
    validator->setText("*");                                                                                           \
  } else {                                                                                                             \
    validator->setText("");                                                                                            \
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
 * Checks to see if the file finder fields are valid.
 *
 * @returns A message if the file finder has a problem.
 */
std::string DiffractionReduction::validateFileFinder(const MantidQt::API::FileFinderWidget *fileFinder,
                                                     bool const isChecked) const {
  if (!fileFinder->isOptional() || isChecked) {
    return fileFinder->getFileProblem().toStdString();
  }
  return "";
}

void DiffractionReduction::useVanadiumStateChanged(int state) { m_uiForm.rfVanFile->setEnabled(state != 0); }

void DiffractionReduction::useCalibStateChanged(int state) { m_uiForm.rfCalFile->setEnabled(state != 0); }

/**
 * Disables and shows message on run button indicating that run files have been
 * changed.
 */
void DiffractionReduction::runFilesChanged() { m_runPresenter->setRunText("Editing..."); }

/**
 * Disables and shows message on run button to indicate searching for data
 * files.
 */
void DiffractionReduction::runFilesFinding() { m_runPresenter->setRunText("Finding files..."); }

/**
 * Updates run button with result of file search.
 */
void DiffractionReduction::runFilesFound() {
  bool valid = m_uiForm.rfSampleFiles->isValid();
  m_runPresenter->setRunText(valid ? "Run" : "Invalid Run");

  // Disable sum files if only one file is given
  int fileCount = m_uiForm.rfSampleFiles->getFilenames().size();
  if (fileCount < 2)
    m_uiForm.ckSumFiles->setChecked(false);
}

void DiffractionReduction::setSaveEnabled(bool enabled) {
  m_uiForm.pbSave->setEnabled(enabled);
  m_uiForm.ckAscii->setEnabled(enabled);
  m_uiForm.ckGSS->setEnabled(enabled);
  m_uiForm.ckNexus->setEnabled(enabled);
}

} // namespace MantidQt::CustomInterfaces
