#include "MantidQtCustomInterfaces/Reflectometry/ReflDataProcessorPresenter.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorTreeManager.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorView.h"
#include "MantidQtMantidWidgets/ProgressPresenter.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"

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
  // 2. We need to pay attention to prefixes. See how the interface names the
  // output workspaces
  // 3. For slices, we probably want to add a suffix:
  // <output_ws_name>_start_stop
  // (check that this is the suffix they're using in Max's script)
  // 4. There may be some methods defined in GenericDataProcessorPreseter that
  // we may find useful here, for instance
  // GenericDataProcessorPresenter::getReducedWorkspaceName() or
  // GenericDataProcessorPresenter::getPostprocessedWorkspaceName(). If there is
  // a private method you want to use, don't hesitate to make it protected in
  // the base class.

  // If transmission runs specified in the table
  // Load transmission runs
  // Run CreateTransmissionWorkspaceAuto, taking into account global options
  // from settings, if any

  // Get selected runs
  const auto items = m_manager->selectedData(true);

  for (const auto &item : items) {

    // Reduce rows sequentially

    for (const auto &data : item.second) {

      // The run number as a string
      std::string runno = data.second.at(0);

      loadRun(runno);

      for (size_t i = 0; i < numSlices; i++) {
        takeSlice(runno, startTimes[i], stopTimes[i]);
      }
    }

    // Post-process (if needed)
  }

  // Perform slicing, see Max's script
  // When running ReflectometryReductionOneAuto apply global options and options
  // specified in the 'Options' column

  // For each group
  // Stitch slices (if needed), applying global options

  // Finally, notebook: Don't implement this for the moment
  // If selected, print message saying that notebook will not be saved
  if (m_view->getEnableNotebook()) {
    GenericDataProcessorPresenter::giveUserWarning(
        "Notebook will not be generated for sliced data",
        "Notebook will not be created");
  }
  // If not selected, do nothing
}

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

/** Takes a slice from a run
*
* @param runno :: the run number as a string
* @param startTime :: start time
* @param stopTime :: stop time
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
*/
void ReflDataProcessorPresenter::takeSlice(const std::string &runno,
                                           double startTime, double stopTime) {

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
}
}
}
