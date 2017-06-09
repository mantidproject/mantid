#include "MantidAlgorithms/JoinRuns.h"

#include "MantidAlgorithms/RunCombinationHelpers/RunCombinationHelper.h"
#include "MantidAlgorithms/RunCombinationHelpers/SampleLogsBehaviour.h"
#include "MantidAPI/ADSValidator.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/make_unique.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/TimeSeriesProperty.h"

namespace Mantid {
namespace Algorithms {

using namespace API;
using namespace Kernel;
using namespace MergeRunsOptions;

namespace {
static const std::string INPUTWORKSPACEPROPERTY = "InputWorkspaces";
static const std::string OUTPUTWORKSPACEPROPERTY = "OutputWorkspace";
static const std::string SAMPLELOGXAXISPROPERTY = "SampleLogAsXAxis";
}

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(JoinRuns)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string JoinRuns::name() const { return "JoinRuns"; }

/// Algorithm's version for identification. @see Algorithm::version
int JoinRuns::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string JoinRuns::category() const {
  return "Transforms\\Merging";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string JoinRuns::summary() const {
  return "Joins the input workspaces horizontally by appending their columns.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void JoinRuns::init() {
  declareProperty(
      Kernel::make_unique<ArrayProperty<std::string>>(
          INPUTWORKSPACEPROPERTY, boost::make_shared<ADSValidator>()),
      "The names of the input workspaces or workspace groups as a list. At "
      "least two point-data MatrixWorkspaces are "
      "required, having the same instrument, same number of spectra and "
      "units.");
  declareProperty(
      SAMPLELOGXAXISPROPERTY, "",
      "The name of the numeric sample log to become the x-axis of the output. "
      "Empty by default, in which case the x-axis of the input "
      "workspaces are stitched."
      "If specified, this will be the x-axis. It has to be numeric, in which "
      "case all the input workspaces must have only one point(bin) or numeric "
      "time series, in which case the number"
      "of elements in the series must match the blocksize for each workspace.");
  declareProperty(Kernel::make_unique<WorkspaceProperty<API::Workspace>>(
                      OUTPUTWORKSPACEPROPERTY, "", Direction::Output),
                  "The output workspace.");

  declareProperty(SampleLogsBehaviour::TIME_SERIES_PROP, "",
                  SampleLogsBehaviour::TIME_SERIES_DOC);
  declareProperty(SampleLogsBehaviour::LIST_PROP, "",
                  SampleLogsBehaviour::LIST_DOC);
  declareProperty(SampleLogsBehaviour::WARN_PROP, "",
                  SampleLogsBehaviour::WARN_DOC);
  declareProperty(SampleLogsBehaviour::WARN_TOL_PROP, "",
                  SampleLogsBehaviour::WARN_TOL_DOC);
  declareProperty(SampleLogsBehaviour::FAIL_PROP, "",
                  SampleLogsBehaviour::FAIL_DOC);
  declareProperty(SampleLogsBehaviour::FAIL_TOL_PROP, "",
                  SampleLogsBehaviour::FAIL_TOL_DOC);
  declareProperty(SampleLogsBehaviour::SUM_PROP, "",
                  SampleLogsBehaviour::SUM_DOC);

  const std::vector<std::string> failBehaviourOptions = {SKIP_BEHAVIOUR,
                                                         STOP_BEHAVIOUR};
  declareProperty("FailBehaviour", SKIP_BEHAVIOUR,
                  boost::make_shared<StringListValidator>(failBehaviourOptions),
                  "Choose whether to skip the workspace and continue, or stop and "
                  "throw and error, when encountering a failure on merging.");

}

std::map<std::string, std::string> JoinRuns::validateInputs() {
  std::map<std::string, std::string> issues;

  const std::vector<std::string> inputs_given = getProperty(INPUTWORKSPACEPROPERTY);
  m_logEntry = getPropertyValue(SAMPLELOGXAXISPROPERTY);

  // find if there are workspaces that are not Matrix or not a point-data
  for (const auto &input : RunCombinationHelper::unWrapGroups(inputs_given)) {
    MatrixWorkspace_sptr ws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(input);
    if (!ws) {
      issues[INPUTWORKSPACEPROPERTY] +=
          "Workspace " + ws->getName() + " is not a MatrixWorkspace\n";
    } else if (ws->isHistogramData()) {
      issues[INPUTWORKSPACEPROPERTY] +=
          "Workspace " + ws->getName() + " is not a point-data\n";
    } else {
      m_inputWS.push_back(ws);
    }
  }

  // we need at least 2 valid input workspaces to perform join operation
  if (m_inputWS.size() < 2) {
    issues[INPUTWORKSPACEPROPERTY] += "There are less than 2 point-data"
                                      " MatrixWorkspaces in the input list\n";
  } else {
    RunCombinationHelper combHelper;
    combHelper.setReferenceProperties(m_inputWS.front());

    for (const auto &ws : m_inputWS) {
      // check if all the others are compatible with the first one
      std::string compatible = combHelper.checkCompatibility(ws, true);
      if (!compatible.empty()) {
        issues[INPUTWORKSPACEPROPERTY] += "Workspace " + ws->getName() +
                                          " is not compatible: " + compatible +
                                          "\n";
      }
      // if the log entry is given, validate it
      const std::string logValid = checkLogEntry(ws);
      if (!logValid.empty()) {
        issues[INPUTWORKSPACEPROPERTY] += "Invalid sample log entry for " +
                                          ws->getName() + ": " + logValid +
                                          "\n";
      }
    }
  }
  m_inputWS.clear();

  return issues;
}

//----------------------------------------------------------------------------------------------
/** Check if the log entry is valid
* @param ws : input workspace to test
* @return : empty if the log exists, is numeric, and matches the size of the
* workspace, error message otherwise
*/
std::string JoinRuns::checkLogEntry(MatrixWorkspace_sptr ws) const {
  std::string result;
  if (!m_logEntry.empty()) {

    const auto &run = ws->run();

    if (!run.hasProperty(m_logEntry)) {
      result = "Log entry does not exist";
    } else {
      try {
        run.getLogAsSingleValue(m_logEntry);

        // try if numeric time series, then the size must match to the blocksize
        const int blocksize = static_cast<int>(ws->blocksize());

        TimeSeriesProperty<double> *timeSeriesDouble(nullptr);
        TimeSeriesProperty<int> *timeSeriesInt(nullptr);
        timeSeriesDouble = dynamic_cast<TimeSeriesProperty<double> *>(
            run.getLogData(m_logEntry));
        timeSeriesInt =
            dynamic_cast<TimeSeriesProperty<int> *>(run.getLogData(m_logEntry));

        if (timeSeriesDouble) {
          if (blocksize != timeSeriesDouble->size()) {
            result =
                "Size of the double time series does not match the blocksize";
          }
        } else if (timeSeriesInt) {
          if (blocksize != timeSeriesInt->size()) {
            result = "Size of the int time series does not match the blocksize";
          }
        } else {
          // if numeric scalar, must have one bin
          if (ws->blocksize() != 1) {
            result =
                "One bin workspaces is required if the log is numeric scalar";
          }
        }
      } catch (std::invalid_argument &) {
        result = "Log entry must be numeric or numeric time series";
      }
    }
  }
  return result;
}

//----------------------------------------------------------------------------------------------
/** Return the to-be axis of the workspace dependent on the log entry
* @param inputs : input workspace
* @return : the [to-be] x-axis of the workspace
*/
std::vector<double> JoinRuns::getXAxis(MatrixWorkspace_sptr ws) const {

  std::vector<double> axis;
  axis.reserve(ws->blocksize());

  if (m_logEntry.empty()) {
    // return the actual x-axis of the first spectrum
    axis = ws->x(0).rawData();
  } else {
    auto& run = ws->run();
    // try time series first
    TimeSeriesProperty<double> *timeSeriesDouble(nullptr);
    timeSeriesDouble =
        dynamic_cast<TimeSeriesProperty<double> *>(run.getLogData(m_logEntry));
    if (timeSeriesDouble) {
      // try double series
      axis = timeSeriesDouble->filteredValuesAsVector();
    } else {
      // try int series next
      TimeSeriesProperty<int> *timeSeriesInt(nullptr);
      timeSeriesInt =
          dynamic_cast<TimeSeriesProperty<int> *>(run.getLogData(m_logEntry));
      if (timeSeriesInt) {
        std::vector<int> intAxis = timeSeriesInt->filteredValuesAsVector();
        axis = std::vector<double>(intAxis.begin(), intAxis.end());
      } else {
        // then scalar
        axis.push_back(run.getPropertyAsSingleValue(m_logEntry));
      }
    }
  }
  return axis;
}

//----------------------------------------------------------------------------------------------
/** Makes up the correct history of the output workspace
*/
void JoinRuns::fillHistory() {
  // If this is not a child algorithm add the history
  if (!isChild()) {
    // Loop over the input workspaces, making the call that copies their
    // history to the output one
    for (auto inWS = m_inputWS.begin(); inWS != m_inputWS.end(); ++inWS) {
      m_outWS->history().addHistory((*inWS)->getHistory());
    }
    // Add the history for the current algorithm to the output
    m_outWS->history().addHistory(m_history);
  }
  // this is a child algorithm, but we still want to keep the history.
  else if (isRecordingHistoryForChild() && m_parentHistory) {
    m_parentHistory->addChildHistory(m_history);
  }
}

//----------------------------------------------------------------------------------------------
/** Joins the given spectrum for the list of workspaces
* @param wsIndex : the workspace index
*/
void JoinRuns::joinSpectrum(long wsIndex) {
  std::vector<double> spectrum;
  std::vector<double> errors;
  spectrum.reserve(m_outWS->blocksize());
  errors.reserve(m_outWS->blocksize());
  size_t index = static_cast<size_t>(wsIndex);

  for (const auto &input : m_inputWS) {
    auto y = input->y(index).rawData();
    auto e = input->e(index).rawData();
    spectrum.insert(spectrum.end(), y.begin(), y.end());
    errors.insert(errors.end(), e.begin(), e.end());
  }

  m_outWS->mutableY(index) = spectrum;
  m_outWS->mutableE(index) = errors;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void JoinRuns::exec() {

  const std::vector<std::string> inputs_given =
      getProperty(INPUTWORKSPACEPROPERTY);
  m_logEntry = getPropertyValue(SAMPLELOGXAXISPROPERTY);

  const std::string sampleLogsSum = getProperty(SampleLogsBehaviour::SUM_PROP);
  const std::string sampleLogsTimeSeries =
      getProperty(SampleLogsBehaviour::TIME_SERIES_PROP);
  const std::string sampleLogsList =
      getProperty(SampleLogsBehaviour::LIST_PROP);
  const std::string sampleLogsWarn =
      getProperty(SampleLogsBehaviour::WARN_PROP);
  const std::string sampleLogsWarnTolerances =
      getProperty(SampleLogsBehaviour::WARN_TOL_PROP);
  const std::string sampleLogsFail =
      getProperty(SampleLogsBehaviour::FAIL_PROP);
  const std::string sampleLogsFailTolerances =
      getProperty(SampleLogsBehaviour::FAIL_TOL_PROP);
  const std::string sampleLogsFailBehaviour = getProperty("FailBehaviour");

  m_inputWS.clear();

  for (const auto &input : RunCombinationHelper::unWrapGroups(inputs_given)) {
    MatrixWorkspace_sptr ws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(input);
    m_inputWS.push_back(ws);
  }

  auto first = m_inputWS.front();
  std::vector<double> axisFirst = getXAxis(first);
  std::vector<double> xAxis;
  xAxis.insert(xAxis.end(), axisFirst.begin(), axisFirst.end());

  SampleLogsBehaviour sampleLogsBehaviour = SampleLogsBehaviour(
      *first, g_log, sampleLogsSum, sampleLogsTimeSeries, sampleLogsList,
      sampleLogsWarn, sampleLogsWarnTolerances, sampleLogsFail,
      sampleLogsFailTolerances);

  auto it = m_inputWS.begin();

  // First sequentially merge the sample logs and build the x-axis
  for(++it; it != m_inputWS.end(); ++it) {
    // attempt to merge the sample logs
    try {
    sampleLogsBehaviour.mergeSampleLogs(**it, *m_outWS);
    sampleLogsBehaviour.setUpdatedSampleLogs(*m_outWS);
    std::vector<double> axisIt = getXAxis(*it);
    xAxis.insert(xAxis.end(), axisIt.begin(), axisIt.end());
    } catch (std::invalid_argument &e) {
        if (sampleLogsFailBehaviour == SKIP_BEHAVIOUR) {
        g_log.error()
            << "Could not join workspace: " << (*it)->getName()
            << ". Reason: \"" << e.what()
            << "\". Skipping.\n";
        sampleLogsBehaviour.resetSampleLogs(*m_outWS);
        // remove the skipped one from the list
        m_inputWS.erase(it);
        --it;
        } else {
            throw std::invalid_argument(e);
        }
    }
  }

  if (m_inputWS.size() == 1) {
    g_log.warning() << "Nothing left to join after skipping the workspaces "
                       "that failed to merge the sample logs.";
    // note, we need to continue still, since
    // the x-axis might need to be changed
  }

  size_t outBlockSize = xAxis.size();
  size_t numSpec = first->getNumberHistograms();

  m_outWS = WorkspaceFactory::Instance().create(
      first, numSpec, outBlockSize, outBlockSize);

  m_progress = make_unique<Progress>(this, 0.0, 1.0, numSpec);

  // Now loop in parallel over all the spectra and join the data
  PARALLEL_FOR_IF(threadSafe(m_outWS))
  for (long index = 0; index < static_cast<long>(numSpec); ++index) {
    PARALLEL_START_INTERUPT_REGION
    m_outWS->mutableX(static_cast<size_t>(index)) = xAxis;
    joinSpectrum(index);
    m_progress->report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  if (!m_logEntry.empty()) {
    std::string unit = m_inputWS.front()->run().getLogData(m_logEntry)->units();
    try {
      m_outWS->getAxis(0)->unit() = UnitFactory::Instance().create(unit);
    } catch (Exception::NotFoundError &) {
      m_outWS->getAxis(0)->unit() = UnitFactory::Instance().create("Empty");
    }
  }

  setProperty("OutputWorkspace", m_outWS);
}
} // namespace Algorithms
} // namespace Mantid
