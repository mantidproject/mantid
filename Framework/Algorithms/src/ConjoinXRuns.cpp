// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ConjoinXRuns.h"

#include "MantidAPI/ADSValidator.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidAlgorithms/RunCombinationHelpers/RunCombinationHelper.h"
#include "MantidAlgorithms/RunCombinationHelpers/SampleLogsBehaviour.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/HistogramDx.h"
#include "MantidHistogramData/HistogramE.h"
#include "MantidHistogramData/HistogramX.h"
#include "MantidHistogramData/HistogramY.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"


namespace Mantid {
namespace Algorithms {

using namespace API;
using namespace Kernel;
using namespace RunCombinationOptions;
using namespace DataObjects;
using namespace HistogramData;

namespace {
static const std::string INPUT_WORKSPACE_PROPERTY = "InputWorkspaces";
static const std::string OUTPUT_WORKSPACE_PROPERTY = "OutputWorkspace";
static const std::string SAMPLE_LOG_X_AXIS_PROPERTY = "SampleLogAsXAxis";
} // namespace

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConjoinXRuns)

const std::string ConjoinXRuns::SUM_MERGE = "conjoin_sample_logs_sum";
const std::string ConjoinXRuns::TIME_SERIES_MERGE =
    "conjoin_sample_logs_time_series";
const std::string ConjoinXRuns::LIST_MERGE = "conjoin_sample_logs_list";
const std::string ConjoinXRuns::WARN_MERGE = "conjoin_sample_logs_warn";
const std::string ConjoinXRuns::WARN_MERGE_TOLERANCES =
    "conjoin_sample_logs_warn_tolerances";
const std::string ConjoinXRuns::FAIL_MERGE = "conjoin_sample_logs_fail";
const std::string ConjoinXRuns::FAIL_MERGE_TOLERANCES =
    "conjoin_sample_logs_fail_tolerances";

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string ConjoinXRuns::name() const { return "ConjoinXRuns"; }

/// Algorithm's version for identification. @see Algorithm::version
int ConjoinXRuns::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ConjoinXRuns::category() const {
  return "Transforms\\Merging";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ConjoinXRuns::summary() const {
  return "Joins the input workspaces horizontally by appending their columns.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ConjoinXRuns::init() {
  declareProperty(
      std::make_unique<ArrayProperty<std::string>>(
          INPUT_WORKSPACE_PROPERTY, boost::make_shared<ADSValidator>()),
      "The names of the input workspaces or workspace groups as a list. At "
      "least two point-data MatrixWorkspaces are "
      "required, having the same instrument, same number of spectra and "
      "units.");
  declareProperty(
      SAMPLE_LOG_X_AXIS_PROPERTY, "",
      "The name of the numeric sample log to become the x-axis of the output. "
      "Empty by default, in which case the x-axis of the input "
      "workspaces are stitched. "
      "If specified, this will be the x-axis. It has to be numeric, in which "
      "case all the input workspaces must have only one point or numeric "
      "time series, in which case the number "
      "of elements in the series must match the number of points for each "
      "workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<API::Workspace>>(
                      OUTPUT_WORKSPACE_PROPERTY, "", Direction::Output),
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
  declareProperty(
      "FailBehaviour", SKIP_BEHAVIOUR,
      boost::make_shared<StringListValidator>(failBehaviourOptions),
      "Choose whether to skip the workspace and continue, or stop and "
      "throw and error, when encountering a failure on merging.");
}

std::map<std::string, std::string> ConjoinXRuns::validateInputs() {
  std::map<std::string, std::string> issues;

  const std::vector<std::string> inputs_given =
      getProperty(INPUT_WORKSPACE_PROPERTY);
  m_logEntry = getPropertyValue(SAMPLE_LOG_X_AXIS_PROPERTY);

  std::vector<std::string> workspaces;
  try { // input workspace must be a group or a MatrixWorkspace
    workspaces = RunCombinationHelper::unWrapGroups(inputs_given);
  } catch (const std::exception &e) {
    issues[INPUT_WORKSPACE_PROPERTY] = std::string(e.what());
  }

  // find if there are grouped workspaces that are not Matrix or not a
  // point-data
  for (const auto &input : workspaces) {
    MatrixWorkspace_sptr ws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(input);
    if (!ws) {
      issues[INPUT_WORKSPACE_PROPERTY] +=
          "Workspace " + input + " is not a MatrixWorkspace\n";
    } else if (ws->isHistogramData()) {
      issues[INPUT_WORKSPACE_PROPERTY] +=
          "Workspace " + ws->getName() + " is not a point-data\n";
    } else {
      try {
        ws->blocksize();
      } catch (std::length_error &) {
        issues[INPUT_WORKSPACE_PROPERTY] +=
            "Workspace " + ws->getName() +
            " has different number of points per histogram\n";
      }
      m_inputWS.emplace_back(ws);
    }
  }

  if (m_inputWS.empty()) {
    issues[INPUT_WORKSPACE_PROPERTY] += "There are no point-data"
                                        " MatrixWorkspaces in the input list\n";
  } else {
    RunCombinationHelper combHelper;
    combHelper.setReferenceProperties(m_inputWS.front());

    for (const auto &ws : m_inputWS) {
      // check if all the others are compatible with the first one
      std::string compatible = combHelper.checkCompatibility(ws, true);
      if (!compatible.empty()) {
        issues[INPUT_WORKSPACE_PROPERTY] +=
            "Workspace " + ws->getName() + " is not compatible: " + compatible +
            "\n";
      }
      // if the log entry is given, validate it
      const std::string logValid = checkLogEntry(ws);
      if (!logValid.empty()) {
        issues[INPUT_WORKSPACE_PROPERTY] += "Invalid sample log entry for " +
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
std::string ConjoinXRuns::checkLogEntry(MatrixWorkspace_sptr ws) const {
  std::string result;
  if (!m_logEntry.empty()) {

    const auto &run = ws->run();

    if (!run.hasProperty(m_logEntry)) {
      result = "Log entry does not exist";
    } else {
      try {
        run.getLogAsSingleValue(m_logEntry);

        // try if numeric time series, then the size must match to the
        // blocksize
        const int blocksize = static_cast<int>(ws->y(0).size());

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
 * @param ws : the input workspace
 * @return : the x-axis to use for the output workspace
 */
std::vector<double> ConjoinXRuns::getXAxis(MatrixWorkspace_sptr ws) const {

  std::vector<double> axis;
  axis.reserve(ws->y(0).size());
  auto &run = ws->run();
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

  return axis;
}

//----------------------------------------------------------------------------------------------
/** Makes up the correct history of the output workspace
 */
void ConjoinXRuns::fillHistory() {
  // If this is not a child algorithm add the history
  if (!isChild()) {
    // Loop over the input workspaces, making the call that copies their
    // history to the output one
    for (auto &inWS : m_inputWS) {
      m_outWS->history().addHistory(inWS->getHistory());
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
void ConjoinXRuns::joinSpectrum(int64_t wsIndex) {
  std::vector<double> spectrum;
  std::vector<double> errors;
  std::vector<double> axis;
  std::vector<double> xerrors;
  const size_t index = static_cast<size_t>(wsIndex);
  const auto ySize = m_outWS->y(index).size();
  spectrum.reserve(ySize);
  errors.reserve(ySize);
  axis.reserve(m_outWS->x(index).size());
  for (const auto &input : m_inputWS) {
    const auto &y = input->y(index);
    spectrum.insert(spectrum.end(), y.begin(), y.end());
    const auto &e = input->e(index);
    errors.insert(errors.end(), e.begin(), e.end());
    if (m_logEntry.empty()) {
      const auto &x = input->x(index);
      axis.insert(axis.end(), x.begin(), x.end());
    } else {
      const auto &x = m_axisCache[input->getName()];
      axis.insert(axis.end(), x.begin(), x.end());
    }
    if (input->hasDx(index)) {
      const auto &dx = input->dx(index);
      xerrors.insert(xerrors.end(), dx.begin(), dx.end());
    }
  }
  if (!xerrors.empty())
    m_outWS->setPointStandardDeviations(index, std::move(xerrors));
  m_outWS->mutableY(index) = std::move(spectrum);
  m_outWS->mutableE(index) = std::move(errors);
  m_outWS->mutableX(index) = std::move(axis);
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ConjoinXRuns::exec() {

  const std::vector<std::string> inputs_given =
      getProperty(INPUT_WORKSPACE_PROPERTY);
  m_logEntry = getPropertyValue(SAMPLE_LOG_X_AXIS_PROPERTY);

  SampleLogsBehaviour::SampleLogNames logEntries = {};
  logEntries.sampleLogsSum = getPropertyValue(SampleLogsBehaviour::SUM_PROP);
  logEntries.sampleLogsTimeSeries =
      getPropertyValue(SampleLogsBehaviour::TIME_SERIES_PROP);
  logEntries.sampleLogsList = getPropertyValue(SampleLogsBehaviour::LIST_PROP);
  logEntries.sampleLogsWarn = getPropertyValue(SampleLogsBehaviour::WARN_PROP);
  logEntries.sampleLogsWarnTolerances =
      getPropertyValue(SampleLogsBehaviour::WARN_TOL_PROP);
  logEntries.sampleLogsFail = getPropertyValue(SampleLogsBehaviour::FAIL_PROP);
  logEntries.sampleLogsFailTolerances =
      getPropertyValue(SampleLogsBehaviour::FAIL_TOL_PROP);
  const std::string sampleLogsFailBehaviour = getProperty("FailBehaviour");

  m_inputWS.clear();

  for (const auto &input : RunCombinationHelper::unWrapGroups(inputs_given)) {
    MatrixWorkspace_sptr ws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(input);
    m_inputWS.push_back(ws);
  }

  auto first = m_inputWS.front();

  SampleLogsBehaviour::ParameterName parName = {
      ConjoinXRuns::SUM_MERGE,
      ConjoinXRuns::TIME_SERIES_MERGE,
      ConjoinXRuns::LIST_MERGE,
      ConjoinXRuns::WARN_MERGE,
      ConjoinXRuns::WARN_MERGE_TOLERANCES,
      ConjoinXRuns::FAIL_MERGE,
      ConjoinXRuns::FAIL_MERGE_TOLERANCES};

  SampleLogsBehaviour sampleLogsBehaviour =
      SampleLogsBehaviour(first, g_log, logEntries, parName);
  auto it = m_inputWS.begin();

  // Temporary workspace to carry the merged sample logs
  // This is cloned from the first workspace and does not have
  // the correct size to be the output, since the size is unknown
  // at this point. We can check only later which ones are going
  // to be skipped, to compute the size of the output respectively.
  MatrixWorkspace_sptr temp = first->clone();

  size_t outBlockSize = (*it)->y(0).size();
  // First sequentially merge the sample logs
  for (++it; it != m_inputWS.end(); ++it) {
    // attempt to merge the sample logs
    try {
      sampleLogsBehaviour.mergeSampleLogs(*it, temp);
      sampleLogsBehaviour.setUpdatedSampleLogs(temp);
      outBlockSize += (*it)->y(0).size();
    } catch (std::invalid_argument &e) {
      if (sampleLogsFailBehaviour == SKIP_BEHAVIOUR) {
        g_log.error() << "Could not join workspace: " << (*it)->getName()
                      << ". Reason: \"" << e.what() << "\". Skipping.\n";
        sampleLogsBehaviour.resetSampleLogs(temp);
        // remove the skipped one from the list
        m_inputWS.erase(it);
        --it;
      } else {
        throw std::invalid_argument(e);
      }
    }
  }

  if (m_inputWS.size() == 1) {
    g_log.warning() << "Nothing left to join [after skipping the workspaces "
                       "that failed to merge the sample logs].";
    // note, we need to continue still, since
    // the x-axis might need to be changed
  }

  if (!m_logEntry.empty()) {
    for (const auto &ws : m_inputWS) {
      m_axisCache[ws->getName()] = getXAxis(ws);
    }
  }

  // now get the size of the output
  size_t numSpec = first->getNumberHistograms();

  m_outWS = create<MatrixWorkspace>(*first, Points(outBlockSize));

  // copy over the merged sample logs from the temp
  m_outWS->mutableRun() = temp->run();

  m_progress = std::make_unique<Progress>(this, 0.0, 1.0, numSpec);

  // Now loop in parallel over all the spectra and join the data
  PARALLEL_FOR_IF(threadSafe(*m_outWS))
  for (int64_t index = 0; index < static_cast<int64_t>(numSpec); ++index) {
    PARALLEL_START_INTERUPT_REGION
    joinSpectrum(index);
    m_progress->report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  if (!m_logEntry.empty()) {
    std::string unit = first->run().getLogData(m_logEntry)->units();
    try {
      m_outWS->getAxis(0)->unit() = UnitFactory::Instance().create(unit);
    } catch (Exception::NotFoundError &) {
      m_outWS->getAxis(0)->unit() = UnitFactory::Instance().create("Empty");
    }
  }

  setProperty("OutputWorkspace", m_outWS);
  m_inputWS.clear();
  m_axisCache.clear();
}
} // namespace Algorithms
} // namespace Mantid
