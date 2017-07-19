#include "MantidQtCustomInterfaces/Reflectometry/ReflDataProcessorPresenter.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorTreeManager.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorView.h"
#include "MantidQtMantidWidgets/DataProcessorUI/ParseKeyValueString.h"
#include "MantidQtMantidWidgets/ProgressPresenter.h"

using namespace MantidQt::MantidWidgets;
using namespace Mantid::API;

namespace MantidQt {
namespace CustomInterfaces {

/**
* Constructor
* @param whitelist : The set of properties we want to show as columns
* @param preprocessMap : A map containing instructions for pre-processing
* @param processor : A DataProcessorProcessingAlgorithm
* @param postprocessor : A DataProcessorPostprocessingAlgorithm
* workspaces
* @param postprocessMap : A map containing instructions for post-processing.
* This map links column name to properties of the post-processing algorithm
* @param loader : The algorithm responsible for loading data
*/
ReflDataProcessorPresenter::ReflDataProcessorPresenter(
    const DataProcessorWhiteList &whitelist,
    const std::map<std::string, DataProcessorPreprocessingAlgorithm> &
        preprocessMap,
    const DataProcessorProcessingAlgorithm &processor,
    const DataProcessorPostprocessingAlgorithm &postprocessor,
    const std::map<std::string, std::string> &postprocessMap,
    const std::string &loader)
    : GenericDataProcessorPresenter(whitelist, preprocessMap, processor,
                                    postprocessor, postprocessMap, loader) {}

/**
* Destructor
*/
ReflDataProcessorPresenter::~ReflDataProcessorPresenter() {}

/**
 Process selected data
*/
void ReflDataProcessorPresenter::process() {

  // Get selected runs
  m_selectedData = m_manager->selectedData(true);

  // Don't continue if there are no items to process
  if (m_selectedData.size() == 0)
    return;

  // If uniform slicing is empty process normally, delegating to
  // GenericDataProcessorPresenter
  auto timeSlicingValues =
      m_mainPresenter->getTimeSlicingValues().toStdString();
  if (timeSlicingValues.empty()) {
    // Check if any input event workspaces still exist in ADS
    if (proceedIfWSTypeInADS(m_selectedData, true)) {
      setPromptUser(false); // Prevent prompting user twice
      GenericDataProcessorPresenter::process();
    }
    return;
  }

  // Check if any input non-event workspaces exist in ADS
  if (!proceedIfWSTypeInADS(m_selectedData, false))
    return;

  // Get time slicing type
  auto timeSlicingType = m_mainPresenter->getTimeSlicingType().toStdString();

  // Progress report
  int progress = 0;
  int maxProgress = (int)(m_selectedData.size());
  ProgressPresenter progressReporter(progress, maxProgress, maxProgress,
                                     m_progressView);

  // True if all groups were processed as event workspaces
  bool allGroupsWereEvent = true;
  // True if errors where encountered when reducing table
  bool errors = false;

  // Loop in groups
  for (const auto &item : m_selectedData) {

    // Group of runs
    GroupData group = item.second;

    try {
      // First load the runs.
      bool allEventWS = loadGroup(group);

      if (allEventWS) {
        // Process the group
        if (processGroupAsEventWS(item.first, group, timeSlicingType,
                                  timeSlicingValues))
          errors = true;

        // Notebook not implemented yet
        if (m_view->getEnableNotebook()) {
          GenericDataProcessorPresenter::giveUserWarning(
              "Notebook not implemented for sliced data yet",
              "Notebook will not be generated");
        }

      } else {
        // Process the group
        if (processGroupAsNonEventWS(item.first, group))
          errors = true;
        // Notebook
      }

      if (!allEventWS)
        allGroupsWereEvent = false;

    } catch (...) {
      errors = true;
    }
    progressReporter.report();
  }

  if (!allGroupsWereEvent)
    m_view->giveUserWarning(
        "Some groups could not be processed as event workspaces", "Warning");
  if (errors)
    m_view->giveUserWarning("Some errors were encountered when "
                            "reducing table. Some groups may not have "
                            "been fully processed.",
                            "Warning");

  progressReporter.clear();
}

/** Loads a group of runs. Tries loading runs as event workspaces. If any of the
* workspaces in the group is not an event workspace, stops loading and re-loads
* all of them as non-event workspaces. We need the workspaces to be of the same
* type to process them together.
*
* @param group :: the group of runs
* @return :: true if all runs were loaded as event workspaces. False otherwise
*/
bool ReflDataProcessorPresenter::loadGroup(const GroupData &group) {

  // Set of runs loaded successfully
  std::set<std::string> loadedRuns;

  for (const auto &row : group) {

    // The run number
    std::string runNo = row.second.at(0);
    // Try loading as event workspace
    bool eventWS = loadEventRun(runNo);
    if (!eventWS) {
      // This run could not be loaded as event workspace. We need to load and
      // process the whole group as non-event data.
      for (const auto &rowNew : group) {
        // The run number
        std::string runNo = rowNew.second.at(0);
        // Load as non-event workspace
        loadNonEventRun(runNo);
      }
      // Remove monitors which were loaded as separate workspaces
      for (const auto &run : loadedRuns) {
        AnalysisDataService::Instance().remove("TOF_" + run + "_monitors");
      }
      return false;
    }
    loadedRuns.insert(runNo);
  }
  return true;
}

/** Processes a group of runs
*
* @param groupID :: An integer number indicating the id of this group
* @param group :: the group of event workspaces
* @param timeSlicingType :: The type of time slicing being used
* @param timeSlicingValues :: The string of values to perform time slicing with
* @return :: true if errors were encountered
*/
bool ReflDataProcessorPresenter::processGroupAsEventWS(
    int groupID, const GroupData &group, const std::string &timeSlicingType,
    const std::string &timeSlicingValues) {

  bool errors = false;
  bool multiRow = group.size() > 1;
  size_t numGroupSlices = INT_MAX;

  std::vector<double> startTimes, stopTimes;
  std::string logFilter; // Set if we are slicing by log value

  // For custom/log value slicing the start/stop times are the same for all rows
  if (timeSlicingType == "Custom")
    parseCustom(timeSlicingValues, startTimes, stopTimes);
  if (timeSlicingType == "LogValue")
    parseLogValue(timeSlicingValues, logFilter, startTimes, stopTimes);

  for (const auto &row : group) {

    const auto rowID = row.first;         // Integer ID of this row
    const auto data = row.second;         // Vector containing data for this row
    std::string runNo = row.second.at(0); // The run number

    if (timeSlicingType == "UniformEven" || timeSlicingType == "Uniform") {
      const std::string runName = "TOF_" + runNo;
      parseUniform(timeSlicingValues, timeSlicingType, runName, startTimes,
                   stopTimes);
    }

    size_t numSlices = startTimes.size();
    addNumSlicesEntry(groupID, rowID, numSlices);

    for (size_t i = 0; i < numSlices; i++) {
      try {
        RowData slice(data);
        std::string wsName =
            takeSlice(runNo, i, startTimes[i], stopTimes[i], logFilter);
        slice[0] = wsName;
        reduceRow(&slice);
        slice[0] = data[0];
        m_manager->update(groupID, rowID, slice);
      } catch (...) {
        return true;
      }
    }

    // For uniform slicing with multiple rows only the minimum number of slices
    // are common to each row
    if (multiRow && timeSlicingType == "Uniform")
      numGroupSlices = std::min(numGroupSlices, numSlices);
  }

  // Post-process (if needed)
  if (multiRow) {

    // All slices are common for uniform even, custom and log value slicing
    if (timeSlicingType != "Uniform")
      numGroupSlices = startTimes.size();

    addNumGroupSlicesEntry(groupID, numGroupSlices);

    for (size_t i = 0; i < numGroupSlices; i++) {
      GroupData groupNew;
      std::vector<std::string> data;
      for (const auto &row : group) {
        data = row.second;
        data[0] = row.second[0] + "_slice_" + std::to_string(i);
        groupNew[row.first] = data;
      }
      try {
        postProcessGroup(groupNew);
      } catch (...) {
        errors = true;
      }
    }
  }

  return errors;
}

/** Processes a group of non-event workspaces
*
* @param groupID :: An integer number indicating the id of this group
* @param group :: the group of event workspaces
* @return :: true if errors were encountered
*/
bool ReflDataProcessorPresenter::processGroupAsNonEventWS(int groupID,
                                                          GroupData &group) {

  bool errors = false;

  for (auto &row : group) {

    // Reduce this row
    reduceRow(&row.second);
    // Update the tree
    m_manager->update(groupID, row.first, row.second);
  }

  // Post-process (if needed)
  if (group.size() > 1) {
    try {
      postProcessGroup(group);
    } catch (...) {
      errors = true;
    }
  }

  return errors;
}

/** Parses a string to extract uniform time slicing
 *
 * @param timeSlicing :: The string to parse
 * @param slicingType :: The type of uniform slicing being used
 * @param wsName :: The name of the workspace to be sliced
 * @param startTimes :: Start times for the set of slices
 * @param stopTimes :: Stop times for the set of slices
 */
void ReflDataProcessorPresenter::parseUniform(const std::string &timeSlicing,
                                              const std::string &slicingType,
                                              const std::string &wsName,
                                              std::vector<double> &startTimes,
                                              std::vector<double> &stopTimes) {

  IEventWorkspace_sptr mws;
  if (AnalysisDataService::Instance().doesExist(wsName)) {
    mws = AnalysisDataService::Instance().retrieveWS<IEventWorkspace>(wsName);
    if (!mws) {
      m_view->giveUserCritical("Workspace to slice " + wsName +
                                   " is not an event workspace!",
                               "Time slicing error");
      return;
    }
  } else {
    m_view->giveUserCritical("Workspace to slice not found: " + wsName,
                             "Time slicing error");
    return;
  }

  const auto run = mws->run();
  const auto totalDuration = run.endTime() - run.startTime();
  double totalDurationSec = totalDuration.total_seconds();
  double sliceDuration = .0;
  int numSlices = 0;

  if (slicingType == "UniformEven") {
    numSlices = std::stoi(timeSlicing);
    sliceDuration = totalDurationSec / numSlices;
  } else if (slicingType == "Uniform") {
    sliceDuration = std::stod(timeSlicing);
    numSlices = static_cast<int>(ceil(totalDurationSec / sliceDuration));
  }

  // Add the start/stop times
  startTimes = std::vector<double>(numSlices);
  stopTimes = std::vector<double>(numSlices);

  for (int i = 0; i < numSlices; i++) {
    startTimes[i] = sliceDuration * i;
    stopTimes[i] = sliceDuration * (i + 1);
  }
}

/** Parses a string to extract custom time slicing
 *
 * @param timeSlicing :: The string to parse
 * @param startTimes :: Start times for the set of slices
 * @param stopTimes :: Stop times for the set of slices
 */
void ReflDataProcessorPresenter::parseCustom(const std::string &timeSlicing,
                                             std::vector<double> &startTimes,
                                             std::vector<double> &stopTimes) {

  std::vector<std::string> timesStr;
  boost::split(timesStr, timeSlicing, boost::is_any_of(","));

  std::vector<double> times;
  std::transform(timesStr.begin(), timesStr.end(), std::back_inserter(times),
                 [](const std::string &astr) { return std::stod(astr); });

  size_t numSlices = times.size() > 1 ? times.size() - 1 : 1;

  // Add the start/stop times
  startTimes = std::vector<double>(numSlices);
  stopTimes = std::vector<double>(numSlices);

  if (times.size() == 1) {
    startTimes[0] = 0;
    stopTimes[0] = times[0];
  } else {
    for (size_t i = 0; i < numSlices; i++) {
      startTimes[i] = times[i];
      stopTimes[i] = times[i + 1];
    }
  }
}

/** Parses a string to extract log value filter and time slicing
 *
 * @param inputStr :: The string to parse
 * @param logFilter :: The log filter to use
 * @param startTimes :: Start times for the set of slices
 * @param stopTimes :: Stop times for the set of slices
 */
void ReflDataProcessorPresenter::parseLogValue(const std::string &inputStr,
                                               std::string &logFilter,
                                               std::vector<double> &startTimes,
                                               std::vector<double> &stopTimes) {

  auto strMap = parseKeyValueString(inputStr);
  std::string timeSlicing = strMap.at("Slicing");
  logFilter = strMap.at("LogFilter");

  parseCustom(timeSlicing, startTimes, stopTimes);
}

/** Loads an event workspace and puts it into the ADS
*
* @param runNo :: The run number as a string
* @return :: True if algorithm was executed. False otherwise
*/
bool ReflDataProcessorPresenter::loadEventRun(const std::string &runNo) {

  bool runFound;
  std::string outName;
  std::string prefix = "TOF_";
  std::string instrument = m_view->getProcessInstrument();

  outName = findRunInADS(runNo, prefix, runFound);
  if (!runFound ||
      AnalysisDataService::Instance().doesExist(outName + "_monitors") ==
          false ||
      AnalysisDataService::Instance().retrieveWS<IEventWorkspace>(outName) ==
          NULL) {
    // Monitors must be loaded first and workspace must be an event workspace
    loadRun(runNo, instrument, prefix, "LoadEventNexus", runFound);
  }

  return runFound;
}

/** Loads a non-event workspace and puts it into the ADS
*
* @param runNo :: The run number as a string
*/
void ReflDataProcessorPresenter::loadNonEventRun(const std::string &runNo) {

  bool runFound; // unused but required
  std::string prefix = "TOF_";
  std::string instrument = m_view->getProcessInstrument();

  findRunInADS(runNo, prefix, runFound);
  if (!runFound)
    loadRun(runNo, instrument, prefix, m_loader, runFound);
}

/** Tries loading a run from disk
 *
 * @param run : The name of the run
 * @param instrument : The instrument the run belongs to
 * @param prefix : The prefix to be prepended to the run number
 * @param loader : The algorithm used for loading runs
 * @param runFound : Whether or not the run was actually found
 * @returns string name of the run
 */
std::string ReflDataProcessorPresenter::loadRun(const std::string &run,
                                                const std::string &instrument,
                                                const std::string &prefix,
                                                const std::string &loader,
                                                bool &runFound) {

  runFound = true;
  const std::string fileName = instrument + run;
  const std::string outputName = prefix + run;

  IAlgorithm_sptr algLoadRun = AlgorithmManager::Instance().create(loader);
  algLoadRun->initialize();
  algLoadRun->setProperty("Filename", fileName);
  algLoadRun->setProperty("OutputWorkspace", outputName);
  if (loader == "LoadEventNexus")
    algLoadRun->setProperty("LoadMonitors", true);
  algLoadRun->execute();
  if (!algLoadRun->isExecuted()) {
    // Run not loaded from disk
    runFound = false;
    return "";
  }

  return outputName;
}

/** Takes a slice from a run and puts the 'sliced' workspace into the ADS
*
* @param runNo :: The run number as a string
* @param sliceIndex :: The index of the slice being taken
* @param startTime :: Start time
* @param stopTime :: Stop time
* @param logFilter :: The log filter to use if slicing by log value
* @return :: the name of the sliced workspace (without prefix 'TOF_')
*/
std::string ReflDataProcessorPresenter::takeSlice(
    const std::string &runNo, size_t sliceIndex, double startTime,
    double stopTime, const std::string &logFilter) {

  std::string runName = "TOF_" + runNo;
  std::string sliceName = runName + "_slice_" + std::to_string(sliceIndex);
  std::string monName = runName + "_monitors";
  std::string filterAlg =
      logFilter.empty() ? "FilterByTime" : "FilterByLogValue";

  // Filter the run using the appropriate filter algorithm
  IAlgorithm_sptr filter = AlgorithmManager::Instance().create(filterAlg);
  filter->initialize();
  filter->setProperty("InputWorkspace", runName);
  filter->setProperty("OutputWorkspace", sliceName);
  if (filterAlg == "FilterByTime") {
    filter->setProperty("StartTime", startTime);
    filter->setProperty("StopTime", stopTime);
  } else { // FilterByLogValue
    filter->setProperty("MinimumValue", startTime);
    filter->setProperty("MaximumValue", stopTime);
    filter->setProperty("TimeTolerance", 1.0);
    filter->setProperty("LogName", logFilter);
  }

  filter->execute();

  // Obtain the normalization constant for this slice
  IEventWorkspace_sptr mws =
      AnalysisDataService::Instance().retrieveWS<IEventWorkspace>(runName);
  double total = mws->run().getProtonCharge();
  mws = AnalysisDataService::Instance().retrieveWS<IEventWorkspace>(sliceName);
  double slice = mws->run().getProtonCharge();
  double scaleFactor = slice / total;

  IAlgorithm_sptr scale = AlgorithmManager::Instance().create("Scale");
  scale->initialize();
  scale->setProperty("InputWorkspace", monName);
  scale->setProperty("Factor", scaleFactor);
  scale->setProperty("OutputWorkspace", "__" + monName + "_temp");
  scale->execute();

  IAlgorithm_sptr rebinDet =
      AlgorithmManager::Instance().create("RebinToWorkspace");
  rebinDet->initialize();
  rebinDet->setProperty("WorkspaceToRebin", sliceName);
  rebinDet->setProperty("WorkspaceToMatch", "__" + monName + "_temp");
  rebinDet->setProperty("OutputWorkspace", sliceName);
  rebinDet->setProperty("PreserveEvents", false);
  rebinDet->execute();

  IAlgorithm_sptr append = AlgorithmManager::Instance().create("AppendSpectra");
  append->initialize();
  append->setProperty("InputWorkspace1", "__" + monName + "_temp");
  append->setProperty("InputWorkspace2", sliceName);
  append->setProperty("OutputWorkspace", sliceName);
  append->setProperty("MergeLogs", true);
  append->execute();

  // Remove temporary monitor ws
  AnalysisDataService::Instance().remove("__" + monName + "_temp");

  return sliceName.substr(4);
}

/** Plots any currently selected rows */
void ReflDataProcessorPresenter::plotRow() {

  const auto items = m_manager->selectedData();
  if (items.size() == 0)
    return;

  // If slicing values are empty plot normally
  auto timeSlicingValues =
      m_mainPresenter->getTimeSlicingValues().toStdString();
  if (timeSlicingValues.empty()) {
    GenericDataProcessorPresenter::plotRow();
    return;
  }

  // Set of workspaces to plot
  std::set<std::string> workspaces;
  // Set of workspaces not found in the ADS
  std::set<std::string> notFound;

  for (const auto &item : items) {

    for (const auto &run : item.second) {

      const size_t numSlices = m_numSlicesMap.at(item.first).at(run.first);
      const std::string wsName = getReducedWorkspaceName(run.second, "IvsQ_");

      for (size_t slice = 0; slice < numSlices; slice++) {
        const std::string sliceName =
            wsName + "_slice_" + std::to_string(slice);
        if (AnalysisDataService::Instance().doesExist(sliceName))
          workspaces.insert(sliceName);
        else
          notFound.insert(sliceName);
      }
    }
  }

  if (!notFound.empty())
    m_view->giveUserWarning(
        "The following workspaces were not plotted because they were not "
        "found:\n" +
            boost::algorithm::join(notFound, "\n") +
            "\n\nPlease check that the rows you are trying to plot have been "
            "fully processed.",
        "Error plotting rows.");

  plotWorkspaces(workspaces);
}

/** This method returns, for a given set of rows, i.e. a group of runs, the name
* of the output (post-processed) workspace
*
* @param groupData : The data in a given group
* @param prefix : A prefix to be appended to the generated ws name
* @param index : The index of the slice
* @returns : The name of the workspace
*/
std::string ReflDataProcessorPresenter::getPostprocessedWorkspaceName(
    const GroupData &groupData, const std::string &prefix, size_t index) {

  std::vector<std::string> outputNames;

  for (const auto &data : groupData) {
    outputNames.push_back(getReducedWorkspaceName(data.second) + "_slice_" +
                          std::to_string(index));
  }
  return prefix + boost::join(outputNames, "_");
}

/** Plots any currently selected groups */
void ReflDataProcessorPresenter::plotGroup() {

  const auto items = m_manager->selectedData();
  if (items.size() == 0)
    return;

  // If slicing values are empty plot normally
  auto timeSlicingValues =
      m_mainPresenter->getTimeSlicingValues().toStdString();
  if (timeSlicingValues.empty()) {
    GenericDataProcessorPresenter::plotGroup();
    return;
  }

  // Set of workspaces to plot
  std::set<std::string> workspaces;
  // Set of workspaces not found in the ADS
  std::set<std::string> notFound;

  for (const auto &item : items) {

    if (item.second.size() > 1) {

      size_t numSlices = m_numGroupSlicesMap.at(item.first);

      for (size_t slice = 0; slice < numSlices; slice++) {

        const std::string wsName =
            getPostprocessedWorkspaceName(item.second, "IvsQ_", slice);

        if (AnalysisDataService::Instance().doesExist(wsName))
          workspaces.insert(wsName);
        else
          notFound.insert(wsName);
      }
    }
  }

  if (!notFound.empty())
    m_view->giveUserWarning(
        "The following workspaces were not plotted because they were not "
        "found:\n" +
            boost::algorithm::join(notFound, "\n") +
            "\n\nPlease check that the groups you are trying to plot have been "
            "fully processed.",
        "Error plotting groups.");

  plotWorkspaces(workspaces);
}

/** Asks user if they wish to proceed if the AnalysisDataService contains input
 * workspaces of a specific type
 *
 * @param data :: The data selected in the table
 * @param findEventWS :: Whether or not we are searching for event workspaces
 * @return :: Boolean - true if user wishes to proceed, false if not
 */
bool ReflDataProcessorPresenter::proceedIfWSTypeInADS(const TreeData &data,
                                                      const bool findEventWS) {

  std::vector<std::string> foundInputWorkspaces;

  for (const auto &item : data) {
    const auto group = item.second;

    for (const auto &row : group) {
      bool runFound = false;
      std::string runNo = row.second.at(0);
      std::string outName = findRunInADS(runNo, "TOF_", runFound);

      if (runFound) {
        bool isEventWS =
            AnalysisDataService::Instance().retrieveWS<IEventWorkspace>(
                outName) != NULL;
        if (findEventWS == isEventWS) {
          foundInputWorkspaces.push_back(outName);
        } else if (isEventWS) { // monitors must be loaded
          std::string monName = outName + "_monitors";
          if (AnalysisDataService::Instance().doesExist(monName) == false)
            foundInputWorkspaces.push_back(outName);
        }
      }
    }
  }

  if (foundInputWorkspaces.size() > 0) {
    // Input workspaces of type found, ask user if they wish to process
    std::string foundStr = boost::algorithm::join(foundInputWorkspaces, "\n");

    bool process = m_view->askUserYesNo(
        "Processing selected rows will replace the following workspaces:\n\n" +
            foundStr + "\n\nDo you wish to continue?",
        "Process selected rows?");

    if (process) {
      // Remove all found workspaces
      for (auto &wsName : foundInputWorkspaces) {
        AnalysisDataService::Instance().remove(wsName);
      }
    }

    return process;
  }

  // No input workspaces of type found, proceed with reduction automatically
  return true;
}

/** Add entry for the number of slices for a row in a group
*
* @param groupID :: The ID of the group
* @param rowID :: The ID of the row in group
* @param numSlices :: Number of slices
*/
void ReflDataProcessorPresenter::addNumSlicesEntry(int groupID, int rowID,
                                                   size_t numSlices) {
  m_numSlicesMap[groupID][rowID] = numSlices;
}

/** Add entry for the number of slices for all rows in a group
*
* @param groupID :: The ID of the group
* @param numSlices :: Number of slices
*/
void ReflDataProcessorPresenter::addNumGroupSlicesEntry(int groupID,
                                                        size_t numSlices) {
  m_numGroupSlicesMap[groupID] = numSlices;
}
}
}
