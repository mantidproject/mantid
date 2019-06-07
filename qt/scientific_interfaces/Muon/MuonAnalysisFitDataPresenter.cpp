// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MuonAnalysisFitDataPresenter.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/GroupingLoader.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/Workspace_fwd.h"
#include "MantidQtWidgets/Common/MuonFitPropertyBrowser.h"
#include "MuonAnalysisHelper.h"
#include "MuonSequentialFitDialog.h"
#include <boost/lexical_cast.hpp>

#include "MantidAPI/ITableWorkspace.h"

using Mantid::API::AnalysisDataService;
using Mantid::API::ITableWorkspace;
using Mantid::API::MatrixWorkspace;
using Mantid::API::TableRow;
using Mantid::API::WorkspaceGroup;
using MantidQt::MantidWidgets::IMuonFitDataModel;
using MantidQt::MantidWidgets::IMuonFitDataSelector;
using MantidQt::MantidWidgets::IWorkspaceFitControl;
using RebinType =
    MantidQt::CustomInterfaces::Muon::MuonAnalysisOptionTab::RebinType;

namespace {
/// static logger
Mantid::Kernel::Logger g_log("MuonAnalysisFitDataPresenter");
/// log a warning
void logWarning(const std::string &message) { g_log.warning(message); }

/// suffix for raw data workspaces
const std::string RAW_DATA_SUFFIX("_Raw");
const size_t RAW_SUFFIX_LENGTH(4);

/// Test if workspace contains raw data
bool isRawData(const std::string &name) {
  const size_t nameLength = name.length();
  if (nameLength > RAW_SUFFIX_LENGTH) {
    return 0 == name.compare(nameLength - RAW_SUFFIX_LENGTH, RAW_SUFFIX_LENGTH,
                             RAW_DATA_SUFFIX);
  } else {
    return false;
  }
}

/// Take off the "_Raw" suffix, if present
std::string removeRawSuffix(const std::string &name) {
  if (isRawData(name)) {
    return name.substr(0, name.length() - RAW_SUFFIX_LENGTH);
  } else {
    return name;
  }
}
} // namespace

namespace MantidQt {
namespace CustomInterfaces {

/**
 * Constructor
 * Defaults values for time zero (0.0) and rebinning (none)
 * @param fitBrowser :: [input] Pointer to fit browser to update
 * @param dataSelector :: [input] Pointer to data selector to get input from
 * @param dataLoader :: [input] Data loader (shared with MuonAnalysis)
 * @param grouping :: [input] Grouping set in interface for data
 * @param plotType :: [input] Plot type set in interface
 */
MuonAnalysisFitDataPresenter::MuonAnalysisFitDataPresenter(
    IWorkspaceFitControl *fitBrowser, IMuonFitDataSelector *dataSelector,
    MuonAnalysisDataLoader &dataLoader, const Mantid::API::Grouping &grouping,
    const Muon::PlotType &plotType)
    : MuonAnalysisFitDataPresenter(fitBrowser, dataSelector, dataLoader,
                                   grouping, plotType, 0.0,
                                   RebinOptions(RebinType::NoRebin, "")) {}

/**
 * Constructor
 * Defaults value for rebinning (none)
 * @param fitBrowser :: [input] Pointer to fit browser to update
 * @param dataSelector :: [input] Pointer to data selector to get input from
 * @param dataLoader :: [input] Data loader (shared with MuonAnalysis)
 * @param grouping :: [input] Grouping set in interface for data
 * @param timeZero :: [input] Time zero from MuonAnalysis interface (optional)
 * @param plotType :: [input] Plot type set in interface
 */
MuonAnalysisFitDataPresenter::MuonAnalysisFitDataPresenter(
    IWorkspaceFitControl *fitBrowser, IMuonFitDataSelector *dataSelector,
    MuonAnalysisDataLoader &dataLoader, const Mantid::API::Grouping &grouping,
    const Muon::PlotType &plotType, double timeZero)
    : MuonAnalysisFitDataPresenter(fitBrowser, dataSelector, dataLoader,
                                   grouping, plotType, timeZero,
                                   RebinOptions(RebinType::NoRebin, "")) {}

/**
 * Constructor
 * @param fitBrowser :: [input] Pointer to fit browser to update
 * @param dataSelector :: [input] Pointer to data selector to get input from
 * @param dataLoader :: [input] Data loader (shared with MuonAnalysis)
 * @param grouping :: [input] Grouping set in interface for data
 * @param plotType :: [input] Plot type set in interface
 * @param timeZero :: [input] Time zero from MuonAnalysis interface (optional)
 * @param rebinArgs :: [input] Rebin args from MuonAnalysis interface (optional)
 */
MuonAnalysisFitDataPresenter::MuonAnalysisFitDataPresenter(
    IWorkspaceFitControl *fitBrowser, IMuonFitDataSelector *dataSelector,
    MuonAnalysisDataLoader &dataLoader, const Mantid::API::Grouping &grouping,
    const Muon::PlotType &plotType, double timeZero,
    const RebinOptions &rebinArgs)
    : m_fitBrowser(fitBrowser), m_fitModel(nullptr),
      m_dataSelector(dataSelector), m_dataLoader(dataLoader),
      m_timeZero(timeZero), m_rebinArgs(rebinArgs), m_grouping(grouping),
      m_plotType(plotType), m_fitRawData(fitBrowser->rawData()),
      m_overwrite(false) {
  // Make sure the FitPropertyBrowser passed in implements the required
  // interfaces
  m_fitModel = dynamic_cast<IMuonFitDataModel *>(m_fitBrowser);
  if (!m_fitModel) {
    throw std::invalid_argument(
        "Fit property browser does not implement required interface");
  }

  // Ensure this is set correctly at the start
  handleSimultaneousFitLabelChanged();
  doConnect();
}

/**
 * Connect up signals and slots
 * Abstract base class is not a QObject, so attempt a cast.
 * (Its derived classes are QObjects).
 */
void MuonAnalysisFitDataPresenter::doConnect() {
  if (const QObject *fitBrowser = dynamic_cast<QObject *>(m_fitBrowser)) {
    connect(fitBrowser, SIGNAL(fittingDone(const QString &)), this,
            SLOT(handleFitFinished(const QString &)));
    connect(fitBrowser, SIGNAL(xRangeChanged(double, double)), this,
            SLOT(handleXRangeChangedGraphically(double, double)));
    connect(fitBrowser, SIGNAL(sequentialFitRequested()), this,
            SLOT(openSequentialFitDialog()));
    connect(fitBrowser, SIGNAL(preFitChecksRequested(bool)), this,
            SLOT(doPreFitChecks(bool)));
    connect(fitBrowser, SIGNAL(fitRawDataClicked(bool)), this,
            SLOT(handleFitRawData(bool)));
  }
  if (const QObject *dataSelector = dynamic_cast<QObject *>(m_dataSelector)) {
    connect(dataSelector, SIGNAL(dataPropertiesChanged()), this,
            SLOT(handleDataPropertiesChanged()));
    connect(dataSelector, SIGNAL(simulLabelChanged()), this,
            SLOT(handleSimultaneousFitLabelChanged()));
    connect(dataSelector, SIGNAL(datasetIndexChanged(int)), this,
            SLOT(handleDatasetIndexChanged(int)));
  }
}

/**
 * Called when data selector reports "data properties changed"
 * Updates WS index, startX, endX
 */
void MuonAnalysisFitDataPresenter::handleDataPropertiesChanged() {
  // update workspace index: always 0
  m_fitBrowser->setWorkspaceIndex(0);

  // update start and end times
  const double start = m_dataSelector->getStartTime();
  const double end = m_dataSelector->getEndTime();
  m_fitBrowser->setStartX(start);
  m_fitBrowser->setEndX(end);
}

/**
 * Called when data selector reports "selected data changed"
 * @param overwrite :: [input] Whether overwrite is on or off in interface
 */
void MuonAnalysisFitDataPresenter::handleSelectedDataChanged(bool overwrite) {
  const auto names = generateWorkspaceNames(overwrite);

  if (!names.empty()) {
    createWorkspacesToFit(names);
    updateWorkspaceNames(names);
    m_fitBrowser->allowSequentialFits(!isMultipleRuns());
    updateFitLabelFromRuns();
  }
}

/**
 * Called when user drags lines to set fit range
 * Update the text boxes silently (no event)
 * @param start :: [input] start of fit range
 * @param end :: [input] end of fit range
 */
void MuonAnalysisFitDataPresenter::handleXRangeChangedGraphically(double start,
                                                                  double end) {
  m_dataSelector->setStartTimeQuietly(start);
  m_dataSelector->setEndTimeQuietly(end);
}

/**
 * Called by selectMultiPeak: fit browser has been reassigned to a new
 * workspace.
 * Sets run number and instrument in the data selector.
 * If multiple runs selected, disable sequential fit option.
 * @param wsName :: [input] Name of new workspace
 * @param filePath :: [input] Optional path to workspace in case of load current
 * run, when it has a temporary name like MUSRauto_E.tmp
 */
void MuonAnalysisFitDataPresenter::setAssignedFirstRun(
    const QString &wsName, const boost::optional<QString> &filePath) {
  if (wsName == m_PPAssignedFirstRun)
    return;

  m_PPAssignedFirstRun = wsName;
  setUpDataSelector(wsName, filePath);
}

/**
 * Creates all workspaces that don't yet exist in the ADS and adds them.
 * @param names :: [input] Names of workspaces to create
 */
void MuonAnalysisFitDataPresenter::createWorkspacesToFit(
    const std::vector<std::string> &names) const {
  // For each name, if not in the ADS, create it
  for (const auto &name : names) {
    if (AnalysisDataService::Instance().doesExist(name)) {
      // We already have it! Leave it there
    } else {
      // Create here and add to the ADS
      std::string groupLabel;
      const auto ws = createWorkspace(name, groupLabel);
      if (ws) {
        AnalysisDataService::Instance().add(name, ws);
        if (!groupLabel.empty()) {
          MuonAnalysisHelper::groupWorkspaces(groupLabel, {name});
          if (Mantid::API::AnalysisDataService::Instance().doesExist(
                  "tmp_unNorm")) {
            const std::string unnorm = "_unNorm";
            std::string wsName = name;
            auto raw = wsName.find("_Raw");

            if (raw == std::string::npos) {
              wsName += unnorm;
            } else {
              wsName.insert(raw, unnorm);
            }

            Mantid::API::AnalysisDataService::Instance().rename("tmp_unNorm",
                                                                wsName);
            MuonAnalysisHelper::groupWorkspaces(groupLabel, {wsName});
          }
        }
      }
    }
  }
}

/**
 * After new workspaces have been created, update the fit browser and data
 * selector with their names. Sets the workspace name, which
 * sends a signal to update the peak picker.
 * @param names :: [input] List of workspace names
 */
void MuonAnalysisFitDataPresenter::updateWorkspaceNames(
    const std::vector<std::string> &names) const {
  QStringList qNames;
  std::transform(
      names.begin(), names.end(), std::back_inserter(qNames),
      [](const std::string &s) { return QString::fromStdString(s); });
  m_fitModel->setWorkspaceNames(qNames);
  m_dataSelector->setDatasetNames(qNames);

  // Quietly update the workspace name set in the fit property browser
  // (Don't want the signal to change what's selected in the view)
  auto *fitBrowser = dynamic_cast<QObject *>(m_fitBrowser);
  if (fitBrowser) {
    fitBrowser->blockSignals(true);
  }
  m_fitBrowser->setWorkspaceName(qNames.first());
  if (fitBrowser) {
    fitBrowser->blockSignals(false);
  }
}

/**
 * Gets names of all workspaces required by asking the view
 * This overload gets the instrument/runs from the view
 * @param overwrite :: [input] Whether overwrite is on or off in interface
 * @returns :: list of workspace names
 */
std::vector<std::string>
MuonAnalysisFitDataPresenter::generateWorkspaceNames(bool overwrite) const {
  const auto instrument = m_dataSelector->getInstrumentName().toStdString();
  const auto runs = m_dataSelector->getRuns().toStdString();
  return generateWorkspaceNames(instrument, runs, overwrite);
}

/**
 * Gets names of all workspaces required by asking the view
 * Instrument/runs passed in separately, so can be used by sequential fits too
 * @param instrument :: [input] Name of instrument
 * @param runString :: [input] Range of runs as a string
 * @param overwrite :: [input] Whether overwrite is on or off in interface
 * @returns :: list of workspace names
 */
std::vector<std::string> MuonAnalysisFitDataPresenter::generateWorkspaceNames(
    const std::string &instrument, const std::string &runString,
    bool overwrite) const {
  // If no instrument or runs, no workspaces needed
  if (instrument.empty() || runString.empty()) {
    return {};
  }

  // From view, get names of all workspaces needed
  std::vector<std::string> workspaceNames;
  const auto groups = m_dataSelector->getChosenGroups();
  const auto periods = m_dataSelector->getPeriodSelections();

  Muon::DatasetParams params;
  std::string runNumber = runString;
  auto index = runString.find(instrument);
  if (index != std::string::npos) {
    // trim path
    runNumber = runString.substr(index + instrument.size());
    // trim extension
    runNumber = runNumber.substr(0, runNumber.find_first_of("."));
  }
  const std::string instRuns = instrument + runNumber;
  std::vector<int> selectedRuns;
  try {
    MuonAnalysisHelper::parseRunLabel(instRuns, params.instrument,
                                      selectedRuns);
  } catch (...) {
    params.instrument = instrument;
    try {
      MuonAnalysisHelper::parseRunLabel(instRuns, params.instrument,
                                        selectedRuns);
    } catch (...) {
      g_log.error("Cannot Parse workspace " + instRuns);
    }
  }
  params.version = 1;
  params.plotType = m_plotType;

  // Find if given name is a group or a pair - defaults to group.
  // If it is not in the groups or pairs list, we will produce a workspace name
  // with "Group" in it rather than throwing.
  const auto grouping = m_grouping;
  const auto getItemType = [&grouping](const std::string &name) {
    if (std::find(grouping.pairNames.begin(), grouping.pairNames.end(), name) !=
        grouping.pairNames.end()) {
      return Muon::ItemType::Pair;
    } else { // If it's not a pair, assume it's a group
      return Muon::ItemType::Group;
    }
  };

  // Generate a unique name from the given parameters
  const auto getUniqueName = [](Muon::DatasetParams &params) {
    std::string workspaceName =
        MuonAnalysisHelper::generateWorkspaceName(params);
    while (AnalysisDataService::Instance().doesExist(workspaceName)) {
      params.version++;
      workspaceName = MuonAnalysisHelper::generateWorkspaceName(params);
    }
    return workspaceName;
  };

  // generate workspace names
  std::vector<std::vector<int>> runNumberVectors;
  if (m_dataSelector->getFitType() == IMuonFitDataSelector::FitType::CoAdd) {
    // Analyse all the runs in one go
    runNumberVectors.emplace_back(selectedRuns);
  } else { // Analyse the runs one by one
    runNumberVectors.reserve(selectedRuns.size());
    for (const int run : selectedRuns) {
      runNumberVectors.emplace_back(std::vector<int>(1, run));
    }
  }

  for (const auto &runsVector : runNumberVectors) {
    params.runs = runsVector;
    for (const auto &group : groups) {
      params.itemType = getItemType(group.toStdString());
      params.itemName = group.toStdString();
      for (const auto &period : periods) {
        params.periods = period.toStdString();
        const std::string wsName =
            overwrite ? MuonAnalysisHelper::generateWorkspaceName(params)
                      : getUniqueName(params);
        workspaceNames.push_back(m_fitRawData ? wsName + RAW_DATA_SUFFIX
                                              : wsName);
      }
    }
  }

  return workspaceNames;
}

/**
 * Create an analysis workspace given the required name.
 * @param name :: [input] Name of workspace to create (in format INST0001234;
 * Pair; long; Asym; 1; #1)
 * @param groupLabel :: [output] Label to group workspace under
 * @returns :: workspace
 */
Mantid::API::Workspace_sptr
MuonAnalysisFitDataPresenter::createWorkspace(const std::string &name,
                                              std::string &groupLabel) const {
  Mantid::API::Workspace_sptr outputWS;

  // parse name to get runs, periods, groups etc
  auto params = MuonAnalysisHelper::parseWorkspaceName(removeRawSuffix(name));

  // load original data - need to get filename(s) of individual run(s)
  QStringList filenames;
  for (const int run : params.runs) {
    // Check if this run is the "current run"
    if (m_currentRun && m_currentRun->run == run) {
      filenames.append(m_currentRun->filePath);
    } else {
      filenames.append(QString::fromStdString(MuonAnalysisHelper::getRunLabel(
                                                  params.instrument, {run}))
                           .append(".nxs"));
    }
  }
  try {
    // This will sum multiple runs together
    const auto loadedData = m_dataLoader.loadFiles(filenames);
    groupLabel = loadedData.label;
    // correct and group the data
    const auto correctedData =
        m_dataLoader.correctAndGroup(loadedData, m_grouping);

    // run analysis to generate workspace
    Muon::AnalysisOptions analysisOptions(m_grouping);
    // Periods
    if (params.periods.empty()) {
      analysisOptions.summedPeriods = "1";
    } else {
      // need a comma seperated list
      std::replace(params.periods.begin(), params.periods.end(), '+', ',');
      const size_t minus = params.periods.find('-');
      analysisOptions.summedPeriods = params.periods.substr(0, minus);
      if (minus != std::string::npos && minus != params.periods.size()) {
        analysisOptions.subtractedPeriods =
            params.periods.substr(minus + 1, std::string::npos);
      }
    }
    // Rebin params: use the same as MuonAnalysis uses, UNLESS this is raw data
    analysisOptions.rebinArgs =
        isRawData(name) ? "" : getRebinParams(correctedData);
    analysisOptions.loadedTimeZero = loadedData.timeZero;
    analysisOptions.timeZero = m_timeZero;
    analysisOptions.timeLimits.first = m_dataSelector->getStartTime();
    analysisOptions.timeLimits.second = m_dataSelector->getEndTime();
    analysisOptions.groupPairName = params.itemName;
    analysisOptions.plotType = params.plotType;
    analysisOptions.wsName = name;
    outputWS =
        m_dataLoader.createAnalysisWorkspace(correctedData, analysisOptions);

  } catch (const std::exception &ex) {
    std::ostringstream err;
    err << "Failed to create analysis workspace " << name << ": " << ex.what();
    g_log.error(err.str());
  }

  return outputWS;
}

/**
 * Generates rebin parameter string from options passed in by MuonAnalysis.
 * On error, returns empty params (no rebinning).
 * @param ws :: [input] Workspace to get bin size from
 * @returns :: parameter string for rebinning
 */
std::string MuonAnalysisFitDataPresenter::getRebinParams(
    const Mantid::API::Workspace_sptr ws) const {
  // First check for workspace group. If it is, use first entry
  if (const auto &group = boost::dynamic_pointer_cast<WorkspaceGroup>(ws)) {
    if (group->size() > 0) {
      return getRebinParams(group->getItem(0));
    } else {
      logWarning("Could not get rebin params from empty group");
      return "";
    }
  }

  std::string params = "";
  if (m_rebinArgs.first == RebinType::FixedRebin) {
    try {
      const double step = std::stod(m_rebinArgs.second);
      const auto &mws = boost::dynamic_pointer_cast<MatrixWorkspace>(ws);
      if (mws) {
        const double binSize = mws->x(0)[1] - mws->x(0)[0];
        params = boost::lexical_cast<std::string>(step * binSize);
      }
    } catch (const std::exception &err) {
      logWarning("Could not get rebin params: " + std::string(err.what()));
      params = "";
    }
  } else if (m_rebinArgs.first == RebinType::VariableRebin) {
    params = m_rebinArgs.second;
  }
  return params;
}

/**
 * Set the label for simultaneous fit results based on input
 */
void MuonAnalysisFitDataPresenter::handleSimultaneousFitLabelChanged() const {
  const QString label = m_dataSelector->getSimultaneousFitLabel();
  m_fitModel->setSimultaneousLabel(label.toStdString());
}

/**
 * When a simultaneous fit finishes, transform the results so the results table
 * can be easily generated:
 * - rename fitted workspaces
 * - extract from group to one level up
 * - split parameter table
 * @param status :: [input] Fit status (unused)
 */
void MuonAnalysisFitDataPresenter::handleFitFinished(
    const QString &status) const {
  Q_UNUSED(status);
  // If fitting was simultaneous, transform the results.
  if (isSimultaneousFit()) {
    const auto label = m_dataSelector->getSimultaneousFitLabel();
    const auto groupName =
        MantidWidgets::MuonFitPropertyBrowser::SIMULTANEOUS_PREFIX +
        label.toStdString();
    try {
      handleFittedWorkspaces(groupName);
      extractFittedWorkspaces(groupName);
    } catch (const Mantid::Kernel::Exception::NotFoundError &notFound) {
      g_log.error()
          << "Failed to process fitted workspaces as they could not be found ("
          << groupName << ").\n"
          << notFound.what();
    }
  }
}

/**
 * Rename fitted workspaces so they can be linked to the input and found by the
 * results table generation code. Add special logs and generate params table.
 * @param baseName :: [input] Base name for workspaces
 * @param groupName :: [input] Name of group that workspaces belong to. Leave
 * empty to be the same as baseName (usual case).
 * @throws Mantid::Kernel::Exception::NotFoundError if _Workspaces or
 * _Parameters are not in the ADS
 */
void MuonAnalysisFitDataPresenter::handleFittedWorkspaces(
    const std::string &baseName, const std::string &groupName) const {
  auto &ads = AnalysisDataService::Instance();
  const auto resultsGroup =
      ads.retrieveWS<WorkspaceGroup>(baseName + "_Workspaces");
  const auto paramsTable =
      ads.retrieveWS<ITableWorkspace>(baseName + "_Parameters");
  if (resultsGroup && paramsTable) {
    const size_t offset = paramsTable->rowCount() - resultsGroup->size();
    for (size_t i = 0; i < resultsGroup->size(); i++) {
      const std::string oldName = resultsGroup->getItem(i)->getName();
      auto wsName = paramsTable->cell<std::string>(offset + i, 0);
      wsName = wsName.substr(wsName.find_first_of('=') + 1); // strip the "f0="
      const auto wsDetails = MuonAnalysisHelper::parseWorkspaceName(wsName);
      // Add group and period as log values so they appear in the table
      addSpecialLogs(oldName, wsDetails);
      // Generate new name and rename workspace
      std::ostringstream newName;
      newName << baseName << "_" << wsDetails.label << "_"
              << wsDetails.itemName;
      if (!wsDetails.periods.empty()) {
        newName << "_" << wsDetails.periods;
      }
      ads.rename(oldName, newName.str() + "_Workspace");
      // Generate new parameters table for this dataset
      const auto fitTable = generateParametersTable(wsName, paramsTable);
      if (fitTable) {
        const std::string fitTableName = newName.str() + "_Parameters";
        ads.addOrReplace(fitTableName, fitTable);
        // If user has specified a group to add to, add to that.
        // Otherwise the group is called the same thing as the base name.
        const std::string groupToAddTo =
            groupName.empty() ? baseName : groupName;
        ads.addToGroup(groupToAddTo, fitTableName);
      }
    }
    // Now that we have split parameters table, can delete it
    ads.remove(baseName + "_Parameters");
  }
}

/**
 * Moves all workspaces in group "groupName_Workspaces" up a level into
 * "groupName"
 * @param baseName :: [input] Base name for workspaces
 * @param groupName :: [input] Name of upper group e.g. "MuonSimulFit_Label". If
 * empty, use same as baseName.
 * @throws Mantid::Kernel::Exception::NotFoundError if _Workspaces is not in the
 * ADS
 */
void MuonAnalysisFitDataPresenter::extractFittedWorkspaces(
    const std::string &baseName, const std::string &groupName) const {
  auto &ads = AnalysisDataService::Instance();
  const std::string resultsGroupName = baseName + "_Workspaces";
  const auto resultsGroup = ads.retrieveWS<WorkspaceGroup>(resultsGroupName);
  // If user has specified a group to add to, add to that.
  // Otherwise the group is called the same thing as the base name.
  const std::string groupToAddTo = groupName.empty() ? baseName : groupName;
  if (ads.doesExist(groupToAddTo) && resultsGroup) {
    for (const auto &name : resultsGroup->getNames()) {
      ads.removeFromGroup(resultsGroupName, name);
      ads.addToGroup(groupToAddTo, name);
    }
    ads.remove(resultsGroupName); // should be empty now
  }
}

/**
 * Add extra logs to the named workspace, using supplied parameters.
 * This is so they can be selected to add to the results table.
 * At present these are:
 * - "group": group name
 * - "period": period string
 * @param wsName :: [input] Name of workspace to add logs to
 * @param wsParams :: [input] Parameters to get log values from
 * @throws Mantid::Kernel::Exception::NotFoundError if wsName not in ADS
 */
void MuonAnalysisFitDataPresenter::addSpecialLogs(
    const std::string &wsName, const Muon::DatasetParams &wsParams) const {
  auto matrixWs =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);
  if (matrixWs) {
    matrixWs->mutableRun().addProperty<std::string>("group", wsParams.itemName);
    matrixWs->mutableRun().addProperty<std::string>("period", wsParams.periods);
  }
}

/**
 * Extract fit parameters into an individual parameters table for this dataset.
 * This enables the Results Table tab of MuonAnalysis to do its work on them.
 * @param wsName :: [input] Workspace name to extract parameters for
 * @param inputTable :: [input] Fit parameters table for all datasets
 * @returns :: individual table for the given workspace
 */
Mantid::API::ITableWorkspace_sptr
MuonAnalysisFitDataPresenter::generateParametersTable(
    const std::string &wsName,
    Mantid::API::ITableWorkspace_sptr inputTable) const {
  Mantid::API::ITableWorkspace_sptr fitTable =
      Mantid::API::WorkspaceFactory::Instance().createTable("TableWorkspace");
  auto nameCol = fitTable->addColumn("str", "Name");
  nameCol->setPlotType(6); // label
  auto valCol = fitTable->addColumn("double", "Value");
  valCol->setPlotType(2); // Y
  auto errCol = fitTable->addColumn("double", "Error");
  errCol->setPlotType(5); // YErr

  // Get "f0"/"f1" index for this workspace name
  const std::string fIndex = [&inputTable, &wsName]() {
    for (size_t i = 0; i < inputTable->rowCount(); i++) {
      auto title = inputTable->cell<std::string>(i, 0);
      const size_t eqPos = title.find_first_of('=');
      if (eqPos == std::string::npos)
        continue;
      if (title.substr(eqPos + 1) == wsName) {
        return title.substr(0, eqPos) + '.';
      }
    }
    return std::string();
  }();

  static const std::string costFuncVal = "Cost function value";
  TableRow row = inputTable->getFirstRow();
  do {
    std::string key;
    double value;
    double error;
    row >> key >> value >> error;
    const size_t fPos = key.find(fIndex);
    if (fPos == 0) {
      TableRow outputRow = fitTable->appendRow();
      outputRow << key.substr(fPos + fIndex.size()) << value << error;
    } else if (key == costFuncVal) {
      TableRow outputRow = fitTable->appendRow();
      outputRow << key << value << error;
    }
  } while (row.next());

  return fitTable;
}

/**
 * Called when user changes the selected dataset index
 * Notify model of this change, which will update function browser
 * @param index :: [input] Selected dataset index
 */
void MuonAnalysisFitDataPresenter::handleDatasetIndexChanged(int index) {
  m_fitModel->userChangedDataset(index);
}

/**
 * Called when user requests a sequential fit.
 * Open the dialog and set up the correct options.
 */
void MuonAnalysisFitDataPresenter::openSequentialFitDialog() {
  // Make sure we have a real fit browser, not a testing mock
  auto *fitBrowser =
      dynamic_cast<MantidWidgets::MuonFitPropertyBrowser *>(m_fitBrowser);
  if (!fitBrowser) {
    return;
  }

  // Check that only one run is selected. If not, disable sequential fits and
  // don't open the dialog
  if (isMultipleRuns()) {
    m_fitBrowser->allowSequentialFits(false);
    return;
  }

  // Open the dialog
  fitBrowser->blockSignals(true);
  MuonSequentialFitDialog dialog(fitBrowser, this);
  dialog.exec();
  fitBrowser->blockSignals(false);
}

/**
 * Called when user requests a fit. Before fit begins, if the fit label is
 * active (simultaneous fit), check if it has already been used. If so,
 * ask the user whether to overwrite.
 * If user chooses not to overwrite, increment the label to avoid overwriting
 * the previous fit.
 *
 * If fit is sequential, do nothing because we will not use this label. Instead,
 * user will choose a label in the sequential fit dialog.
 *
 * @param sequentialFit :: [input] Whether fit is sequential or not
 */
void MuonAnalysisFitDataPresenter::checkAndUpdateFitLabel(bool sequentialFit) {
  if (sequentialFit) {
    return; // label not used so no need to check it
  }

  if (isSimultaneousFit()) {
    auto &ads = AnalysisDataService::Instance();
    const auto &label = m_dataSelector->getSimultaneousFitLabel().toStdString();

    std::string uniqueName = label;
    if (ads.doesExist(
            MantidWidgets::MuonFitPropertyBrowser::SIMULTANEOUS_PREFIX +
            label)) {
      const bool overwrite = m_dataSelector->askUserWhetherToOverwrite();
      if (!overwrite) {
        // Take off '#n' suffix if already present, otherwise add one
        const auto pos = uniqueName.find_last_of('#');
        if (pos == std::string::npos) {
          uniqueName += '#';
        } else {
          uniqueName.erase(pos + 1);
        }
        size_t version(2);
        while (ads.doesExist(
            MantidWidgets::MuonFitPropertyBrowser::SIMULTANEOUS_PREFIX +
            uniqueName + std::to_string(version))) {
          ++version;
        }
        uniqueName += std::to_string(version);
      }
    }

    m_dataSelector->setSimultaneousFitLabel(QString::fromStdString(uniqueName));
    m_fitModel->setSimultaneousLabel(uniqueName);
  }
}

/**
 * Test if this was a simultaneous fit, or a co-add fit with multiple
 * groups/periods
 *
 * @returns :: True for simultaneous fit, else false
 */
bool MuonAnalysisFitDataPresenter::isSimultaneousFit() const {
  if (m_dataSelector->getFitType() ==
      IMuonFitDataSelector::FitType::Simultaneous) {
    return true;
  } else {
    return m_dataSelector->getChosenGroups().size() > 1 ||
           m_dataSelector->getPeriodSelections().size() > 1;
  }
}

/**
 * Called by Muon Analysis when tab changes from Home to Data Analysis.
 * Resets the input of the data selector to the workspace that's selected on the
 * Home tab.
 * @param wsName :: [input] Current workspace name
 * @param filePath :: [input] Path to the data file - this is important in the
 * case of "load current run" when the file may have a special name like
 * MUSRauto_E.tmp
 */
void MuonAnalysisFitDataPresenter::setSelectedWorkspace(
    const QString &wsName, const boost::optional<QString> &filePath) {
  updateWorkspaceNames(std::vector<std::string>{wsName.toStdString()});
  setUpDataSelector(wsName, filePath);
}

/**
 * Based on the given workspace name, set UI of data selector
 * @param wsName :: [input] Workspace name
 * @param filePath :: [input] (optional) Path to the data file - this is
 * important in the case of "load current run" when the file may have a special
 * name like MUSRauto_E.tmp.
 */
void MuonAnalysisFitDataPresenter::setUpDataSelector(
    const QString &wsName, const boost::optional<QString> &filePath) {
  // Parse workspace name here for run number and instrument name
  const auto wsParams =
      MuonAnalysisHelper::parseWorkspaceName(wsName.toStdString());
  const QString instRun = QString::fromStdString(wsParams.label);
  const int firstZero = instRun.indexOf("0");
  const QString numberString = instRun.right(instRun.size() - firstZero);
  m_dataSelector->setWorkspaceDetails(
      numberString, QString::fromStdString(wsParams.instrument), filePath);

  // If given an optional file path to "current run", cache it for later use
  if (filePath && !wsParams.runs.empty()) {
    m_currentRun = Muon::CurrentRun(wsParams.runs.front(), filePath.get());
  } else {
    m_currentRun = boost::none;
  }
}

/**
 * Check if multiple runs (co-add or simultaneous) are selected
 * @returns :: True if multiple runs selected
 */
bool MuonAnalysisFitDataPresenter::isMultipleRuns() const {
  return m_dataSelector->getRuns().contains(QRegExp("-|,"));
}

/**
 * Handle "fit raw data" selected/deselected
 * Update stored value
 * Create raw workspaces if necessary
 * @param enabled :: [input] Whether option has been selected or unselected
 * @param updateWorkspaces :: [input] Whether to create workspaces if they don't
 * exist
 */
void MuonAnalysisFitDataPresenter::handleFitRawData(bool enabled,
                                                    bool updateWorkspaces) {
  m_fitRawData = enabled;
  if (updateWorkspaces) {
    handleSelectedDataChanged(m_overwrite);
  }
}

/**
 * When run numbers are changed, update the simultaneous fit label.
 * If it's a user-set label, leave it alone, otherwise set the label to the run
 * number string.
 *
 * Assume labels with digits, '-', ',' are default (e.g. "15189-91") and
 * anything else is user-set.
 */
void MuonAnalysisFitDataPresenter::updateFitLabelFromRuns() {
  // Don't change the fit label if it's a user-set one
  const auto &label = m_dataSelector->getSimultaneousFitLabel().toStdString();
  const bool isDefault =
      label.find_first_not_of("0123456789-,") == std::string::npos;
  if (isDefault) {
    // replace with current run string
    const auto &runString = m_dataSelector->getRuns();
    m_dataSelector->setSimultaneousFitLabel(runString);
    m_fitModel->setSimultaneousLabel(runString.toStdString());
  }
}

/**
 * Perform pre-fit checks and, if OK, tell the model it can go ahead with the
 * fit.
 * Checks are:
 * - Has the fit label already been used? If so, ask user whether to overwrite.
 * - Is the input run string valid?
 * @param sequential :: [input] Whether fit is sequential or not
 */
void MuonAnalysisFitDataPresenter::doPreFitChecks(bool sequential) {
  checkAndUpdateFitLabel(sequential);
  if (isRunStringValid()) {
    m_fitModel->continueAfterChecks(sequential);
  } else {
    g_log.error("Pre-fit checks failed: run string is not valid.\nCheck that "
                "the data files are in Mantid's data search path.");
  }
}

/**
 * Check if the user has input a valid range of runs, i.e. that the red star is
 * not shown on the interface
 * @returns :: whether the runs are valid or not
 */
bool MuonAnalysisFitDataPresenter::isRunStringValid() {
  return !m_dataSelector->getRuns().isEmpty();
}

} // namespace CustomInterfaces
} // namespace MantidQt
