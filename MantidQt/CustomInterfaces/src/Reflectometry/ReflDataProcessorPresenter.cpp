#include "MantidQtCustomInterfaces/Reflectometry/ReflDataProcessorPresenter.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorTreeManager.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorView.h"
#include "MantidQtMantidWidgets/ProgressPresenter.h"

#include <boost/algorithm/string.hpp>

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

  // If uniform slicing is empty process normally, delegating to
  // GenericDataProcessorPresenter
  std::string timeSlicing = m_mainPresenter->getTimeSlicingOptions();
  if (timeSlicing.empty()) {
    GenericDataProcessorPresenter::process();
    return;
  }

  // Parse time slices
  std::vector<double> startTimes, stopTimes;
  parseTimeSlicing(timeSlicing, startTimes, stopTimes);

  // Get selected runs
  const auto items = m_manager->selectedData(true);

  // Progress report
  int progress = 0;
  int maxProgress = (int)(items.size());
  ProgressPresenter progressReporter(progress, maxProgress, maxProgress,
                                     m_progressView);

  // True if all groups were processed as event workspaces
  bool allGroupsWereEvent = true;
  // True if errors where encountered when reducing table
  bool errors = false;

  // Loop in groups
  for (const auto &item : items) {

    // Group of runs
    GroupData group = item.second;

    try {
      // First load the runs.
      bool allEventWS = loadGroup(group);

      if (allEventWS) {
        // Process the group
        if (processGroupAsEventWS(item.first, group, startTimes, stopTimes))
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
    m_mainPresenter->giveUserWarning(
        "Some groups could not be processed as event workspaces", "Warning");
  if (errors)
    m_mainPresenter->giveUserWarning("Some errors were encountered when "
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
* @param startTimes :: start times for the set of slices
* @param stopTimes :: stop times for the set of slices
* @return :: true if errors were encountered
*/
bool ReflDataProcessorPresenter::processGroupAsEventWS(
    int groupID, const GroupData &group, const std::vector<double> &startTimes,
    const std::vector<double> &stopTimes) {

  bool errors = false;
  size_t numSlices = startTimes.size();

  for (const auto &row : group) {

    // Vector containing data for this row
    auto data = row.second;
    // The run number
    std::string runNo = row.second.at(0);

    for (size_t i = 0; i < numSlices; i++) {
      try {
        auto wsName = takeSlice(runNo, startTimes[i], stopTimes[i]);
        std::vector<std::string> slice(data);
        slice[0] = wsName;
        auto newData = reduceRow(slice);
        newData[0] = data[0];
        m_manager->update(groupID, row.first, newData);
      } catch (...) {
        return true;
      }
    }
  }

  // Post-process (if needed)
  if (group.size() > 1) {
    for (size_t i = 0; i < numSlices; i++) {

      GroupData groupNew;
      std::vector<std::string> data;
      for (const auto &row : group) {
        data = row.second;
        data[0] = row.second[0] + "_" + std::to_string((int)startTimes[i]) +
                  "_" + std::to_string((int)stopTimes[i]);
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
bool ReflDataProcessorPresenter::processGroupAsNonEventWS(
    int groupID, const GroupData &group) {

  bool errors = false;

  for (const auto &row : group) {

    // Reduce this row
    auto newData = reduceRow(row.second);
    // Update the tree
    m_manager->update(groupID, row.first, newData);
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

/** Parses a string to extract time slicing
*
* @param timeSlicing :: the string to parse
* @param startTimes :: [output] A vector containing the start time for each
*slice
* @param stopTimes :: [output] A vector containing the stop time for each
*slice
*/
void ReflDataProcessorPresenter::parseTimeSlicing(
    const std::string &timeSlicing, std::vector<double> &startTimes,
    std::vector<double> &stopTimes) {

  std::vector<std::string> timesStr;
  boost::split(timesStr, timeSlicing, boost::is_any_of(","));

  std::vector<double> times;
  std::transform(timesStr.begin(), timesStr.end(), std::back_inserter(times),
                 [](const std::string &astr) { return std::stod(astr); });

  size_t numTimes = times.size();

  if (numTimes == 1) {
    startTimes.push_back(0);
    stopTimes.push_back(times[0]);
  } else if (numTimes == 2) {
    startTimes.push_back(times[0]);
    stopTimes.push_back(times[1]);
  } else {
    for (size_t i = 0; i < numTimes - 1; i++) {
      startTimes.push_back(times[i]);
      stopTimes.push_back(times[i + 1]);
    }
  }

  if (startTimes.size() != stopTimes.size())
    m_mainPresenter->giveUserCritical("Error parsing time slices",
                                      "Time slicing error");
}

/** Loads an event workspace and puts it into the ADS
*
* @param runNo :: the run number as a string
* @return :: True if algorithm was executed. False otherwise
*/
bool ReflDataProcessorPresenter::loadEventRun(const std::string &runNo) {

  std::string runName = "TOF_" + runNo;

  IAlgorithm_sptr algLoadRun =
      AlgorithmManager::Instance().create("LoadEventNexus");
  algLoadRun->initialize();
  algLoadRun->setProperty("Filename", m_view->getProcessInstrument() + runNo);
  algLoadRun->setProperty("OutputWorkspace", runName);
  algLoadRun->setProperty("LoadMonitors", true);
  algLoadRun->execute();
  return algLoadRun->isExecuted();
}

/** Loads a non-event workspace and puts it into the ADS
*
* @param runNo :: the run number as a string
*/
void ReflDataProcessorPresenter::loadNonEventRun(const std::string &runNo) {

  std::string runName = "TOF_" + runNo;

  IAlgorithm_sptr algLoadRun =
      AlgorithmManager::Instance().create("LoadISISNexus");
  algLoadRun->initialize();
  algLoadRun->setProperty("Filename", m_view->getProcessInstrument() + runNo);
  algLoadRun->setProperty("OutputWorkspace", runName);
  algLoadRun->execute();
}

/** Takes a slice from a run and puts the 'sliced' workspace into the ADS
*
* @param runNo :: the run number as a string
* @param startTime :: start time
* @param stopTime :: stop time
* @return :: the name of the sliced workspace (without prefix 'TOF_')
*/
std::string ReflDataProcessorPresenter::takeSlice(const std::string &runNo,
                                                  double startTime,
                                                  double stopTime) {

  std::string runName = "TOF_" + runNo;
  std::string sliceName = runName + "_" + std::to_string((int)startTime) + "_" +
                          std::to_string((int)stopTime);
  std::string monName = runName + "_monitors";

  // Filter by time
  IAlgorithm_sptr filter = AlgorithmManager::Instance().create("FilterByTime");
  filter->initialize();
  filter->setProperty("InputWorkspace", runName);
  filter->setProperty("OutputWorkspace", sliceName);
  filter->setProperty("StartTime", startTime);
  filter->setProperty("StopTime", stopTime);
  filter->execute();

  // Get the normalization constant for this slice
  MatrixWorkspace_sptr mws =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(runName);
  double total = mws->run().getProtonCharge();
  mws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(sliceName);
  double slice = mws->run().getProtonCharge();
  double fraction = slice / total;

  IAlgorithm_sptr scale = AlgorithmManager::Instance().create("Scale");
  scale->initialize();
  scale->setProperty("InputWorkspace", monName);
  scale->setProperty("Factor", fraction);
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

  // if uniform slicing is empty plot normally
  std::string timeSlicing = m_mainPresenter->getTimeSlicingOptions();
  if (timeSlicing.empty()) {
    GenericDataProcessorPresenter::plotRow();
    return;
  }

  // Parse time slices
  std::vector<double> startTimes, stopTimes;
  parseTimeSlicing(timeSlicing, startTimes, stopTimes);
  size_t numSlices = startTimes.size();

  // Set of workspaces to plot
  std::set<std::string> workspaces;
  // Set of workspaces not found in the ADS
  std::set<std::string> notFound;

  const auto items = m_manager->selectedData();

  for (const auto &item : items) {
    for (const auto &run : item.second) {

      const std::string wsName = getReducedWorkspaceName(run.second, "IvsQ_");

      for (size_t slice = 0; slice < numSlices; slice++) {
        const std::string sliceName =
            wsName + "_" + std::to_string((int)startTimes[slice]) + "_" +
            std::to_string((int)stopTimes[slice]);
        if (AnalysisDataService::Instance().doesExist(sliceName))
          workspaces.insert(sliceName);
        else
          notFound.insert(sliceName);
      }
    }
  }

  if (!notFound.empty())
    m_mainPresenter->giveUserWarning(
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
* @param startTime : start time of the slice
* @param stopTime : stop time of the slice
* @returns : The name of the workspace
*/
std::string ReflDataProcessorPresenter::getPostprocessedWorkspaceName(
    const GroupData &groupData, const std::string &prefix, double startTime,
    double stopTime) {

  std::vector<std::string> outputNames;

  for (const auto &data : groupData) {
    outputNames.push_back(getReducedWorkspaceName(data.second) + "_" +
                          std::to_string((int)startTime) + "_" +
                          std::to_string((int)stopTime));
  }
  return prefix + boost::join(outputNames, "_");
}

/** Plots any currently selected groups */
void ReflDataProcessorPresenter::plotGroup() {

  // if uniform slicing is empty plot normally
  std::string timeSlicing = m_mainPresenter->getTimeSlicingOptions();
  if (timeSlicing.empty()) {
    GenericDataProcessorPresenter::plotGroup();
    return;
  }

  // Parse time slices
  std::vector<double> startTimes, stopTimes;
  parseTimeSlicing(timeSlicing, startTimes, stopTimes);
  size_t numSlices = startTimes.size();

  // Set of workspaces to plot
  std::set<std::string> workspaces;
  // Set of workspaces not found in the ADS
  std::set<std::string> notFound;

  const auto items = m_manager->selectedData();

  for (const auto &item : items) {

    if (item.second.size() > 1) {

      for (size_t slice = 0; slice < numSlices; slice++) {

        const std::string wsName = getPostprocessedWorkspaceName(
            item.second, "IvsQ_", startTimes[slice], stopTimes[slice]);

        if (AnalysisDataService::Instance().doesExist(wsName))
          workspaces.insert(wsName);
        else
          notFound.insert(wsName);
      }
    }
  }

  if (!notFound.empty())
    m_mainPresenter->giveUserWarning(
        "The following workspaces were not plotted because they were not "
        "found:\n" +
            boost::algorithm::join(notFound, "\n") +
            "\n\nPlease check that the groups you are trying to plot have been "
            "fully processed.",
        "Error plotting groups.");

  plotWorkspaces(workspaces);
}
}
}
