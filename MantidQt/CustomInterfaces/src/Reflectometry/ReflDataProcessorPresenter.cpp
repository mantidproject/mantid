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
    const std::map<std::string, DataProcessorPreprocessingAlgorithm>
        &preprocessMap,
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

  // if uniform slicing is empty process normally, delegating to
  // GenericDataProcessorPresenter
  std::string timeSlicing = m_mainPresenter->getTimeSlicingOptions();
  if (timeSlicing.empty()) {
    GenericDataProcessorPresenter::process();
    return;
  }

  // Parse time slices
  std::vector<double> startTimes, stopTimes;
  parseTimeSlicing(timeSlicing, startTimes, stopTimes);
  size_t numSlices = startTimes.size();

  // Things we should take into account:
  // 1. We need to report progress, see GenericDataProcessorPresenter::process()

  // Get selected runs
  const auto items = m_manager->selectedData(true);

  for (const auto &item : items) {

    // Reduce rows sequentially

    for (const auto &data : item.second) {

      auto row = data.second;

      // Load the run
      std::string runno = row.at(0);
      try {
        loadRun(runno);
      } catch (...) {
        m_mainPresenter->giveUserCritical(
            "Couldn't load run " + runno + " as event workspace", "Error");
        return;
      }

      for (size_t i = 0; i < numSlices; i++) {
        auto wsName = takeSlice(runno, startTimes[i], stopTimes[i]);
        std::vector<std::string> slice(row);
        slice[0] = wsName;
        auto newData = reduceRow(slice);
        newData[0] = row[0];
        m_manager->update(item.first, data.first, newData);
      }
    }

    // Post-process (if needed)
    if (item.second.size() > 1) {
      for (size_t i = 0; i < numSlices; i++) {

        GroupData group;
        std::vector<std::string> data;
        for (const auto &row : item.second) {
          data = row.second;
          data[0] = row.second[0] + "_" + std::to_string((int)startTimes[i]) +
                    "_" + std::to_string((int)stopTimes[i]);
          group[row.first] = data;
        }
        postProcessGroup(group);
      }
    }
  }

  // Notebook not implemented yet
  if (m_view->getEnableNotebook()) {
    GenericDataProcessorPresenter::giveUserWarning(
        "Notebook not implemented for sliced data yet",
        "Notebook will not be generated");
  }
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

/** Loads a run
*
* @param runno :: the run number as a string
*/
void ReflDataProcessorPresenter::loadRun(const std::string &runno) {

  std::string runName = "TOF_" + runno;

  // Load the run
  IAlgorithm_sptr algLoadRun = AlgorithmManager::Instance().create("Load");
  algLoadRun->initialize();
  algLoadRun->setProperty("Filename", m_view->getProcessInstrument() + runno);
  algLoadRun->setProperty("OutputWorkspace", runName);
  algLoadRun->setProperty("LoadMonitors", true);
  algLoadRun->execute();
}

/** Takes a slice from a run and puts the 'sliced' workspace into the ADS
*
* @param runno :: the run number as a string
* @param startTime :: start time
* @param stopTime :: stop time
* @return :: the name of the sliced workspace (without prefix 'TOF_')
*/
std::string ReflDataProcessorPresenter::takeSlice(const std::string &runno,
                                                  double startTime,
                                                  double stopTime) {

  std::string runName = "TOF_" + runno;
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
  scale->setProperty("OutputWorkspace", monName);
  scale->execute();

  IAlgorithm_sptr rebinMon = AlgorithmManager::Instance().create("Rebin");
  rebinMon->initialize();
  rebinMon->setProperty("InputWorkspace", monName);
  rebinMon->setProperty("OutputWorkspace", monName);
  rebinMon->setProperty("Params", "0, 100, 100000");
  rebinMon->setProperty("PreserveEvents", false);
  rebinMon->execute();

  IAlgorithm_sptr rebinDet = AlgorithmManager::Instance().create("Rebin");
  rebinDet->initialize();
  rebinDet->setProperty("InputWorkspace", sliceName);
  rebinDet->setProperty("OutputWorkspace", sliceName);
  rebinDet->setProperty("Params", "0, 100, 100000");
  rebinDet->setProperty("PreserveEvents", false);
  rebinDet->execute();

  IAlgorithm_sptr append = AlgorithmManager::Instance().create("AppendSpectra");
  append->initialize();
  append->setProperty("InputWorkspace1", monName);
  append->setProperty("InputWorkspace2", sliceName);
  append->setProperty("OutputWorkspace", sliceName);
  append->setProperty("MergeLogs", true);
  append->execute();

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

      const std::string wsName =
          getReducedWorkspaceName(run.second, m_processor.prefix(0));

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
