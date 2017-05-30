#include "MantidAlgorithms/JoinWorkspaces.h"

#include "MantidAlgorithms/RunCombinationHelpers/RunCombinationHelper.h"
#include "MantidAlgorithms/RunCombinationHelpers/SampleLogsBehaviour.h"
#include "MantidAPI/ADSValidator.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/make_unique.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/TimeSeriesProperty.h"

namespace Mantid {
namespace Algorithms {

using namespace API;
using namespace Kernel;

namespace {
static const std::string INPUTWORKSPACEPROPERTY = "InputWorkspaces";
static const std::string OUTPUTWORKSPACEPROPERTY = "OutputWorkspace";
static const std::string SAMPLELOGXAXISPROPERTY = "SampleLogAsXAxis";
}

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(JoinWorkspaces)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string JoinWorkspaces::name() const { return "JoinWorkspaces"; }

/// Algorithm's version for identification. @see Algorithm::version
int JoinWorkspaces::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string JoinWorkspaces::category() const {
  return "Transforms\\Merging";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string JoinWorkspaces::summary() const {
  return "Joins the input workspaces horizontally by appending their columns.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void JoinWorkspaces::init() {
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

  RunCombinationHelper::declareSampleLogOverrideProperties(this);

}

std::map<std::string, std::string> JoinWorkspaces::validateInputs() {
  std::map<std::string, std::string> issues;

  const std::vector<std::string> inputs_given = getProperty(INPUTWORKSPACEPROPERTY);
  const std::string log = getPropertyValue(SAMPLELOGXAXISPROPERTY);
  const bool logSpecified = !log.empty();
  std::list<MatrixWorkspace_sptr> inputWS;

  // collect here the list of input workspaces expanded from the groups if any
  std::vector<std::string> inputs = unWrapGroups(inputs_given);

  // find if there are workspaces that are not Matrix or not a point-data
  for (const auto& input : inputs) {
    MatrixWorkspace_sptr ws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(input);
    if (!ws) {
      issues[INPUTWORKSPACEPROPERTY] +=
          "Workspace " + ws->getName() + " is not a MatrixWorkspace\n";
    } else if (ws->isHistogramData()) {
      issues[INPUTWORKSPACEPROPERTY] +=
          "Workspace " + ws->getName() + " is not a point-data\n";
    } else {
        inputWS.push_back(ws);
    }
  }

  // we need at least 2 valid input workspaces to perform join operation
  if (inputWS.size() < 2) {
    issues[INPUTWORKSPACEPROPERTY] += "There are less than 2 point-data"
                                      " MatrixWorkspaces in the input list\n";
  } else {
    // extract the properties of the first workspace
    const size_t numSpec = inputWS.front()->getNumberHistograms();
    const std::string xUnit = inputWS.front()->getAxis(0)->unit()->unitID();
    const std::string yUnit = inputWS.front()->YUnit();
    const std::string spectrumAxisUnit =
        inputWS.front()->getAxis(1)->unit()->unitID();
    const std::string instrumentName =
        inputWS.front()->getInstrument()->getName();

    for (const auto &ws : inputWS) {
      // check if all the others are compatible with the first one
      if (ws != inputWS.front()) {
        if (!checkCompatibility(ws, numSpec, xUnit, yUnit, spectrumAxisUnit,
                                instrumentName)) {
          issues[INPUTWORKSPACEPROPERTY] +=
              "Workspace " + ws->getName() + " is not compatible\n";
        }
      }
      // if the log entry is given, validate it
      if (logSpecified && !checkLogEntry(ws, log)) {
        issues[SAMPLELOGXAXISPROPERTY] +=
            "Invalid sample log entry for " + ws->getName() + "\n";
      }
    }
  }
  return issues;
}

//----------------------------------------------------------------------------------------------
/** Check if the log entry is valid
* @param ws : input workspace to test
* @param log : the sample log entry name
* @return : true if the log exists, is numeric, and matches the size of the
* workspace
*/
bool JoinWorkspaces::checkLogEntry(MatrixWorkspace_sptr ws,
                                   const std::string &log) {
  const auto &run = ws->run();
  const auto blocksize = static_cast<int>(ws->blocksize());
  if (!run.hasProperty(log)) {
    return false;
  } else {
    try {
      run.getLogAsSingleValue(log);

      // try if numeric time series, then the size must match to the blocksize
      TimeSeriesProperty<double> *timeSeriesDouble(nullptr);
      TimeSeriesProperty<int> *timeSeriesInt(nullptr);
      timeSeriesDouble =
          dynamic_cast<TimeSeriesProperty<double> *>(run.getLogData(log));
      timeSeriesInt =
          dynamic_cast<TimeSeriesProperty<int> *>(run.getLogData(log));

      if (timeSeriesDouble) {
        if (blocksize != timeSeriesDouble->size()) {
          return false;
        }
      } else if (timeSeriesInt) {
        if (blocksize != timeSeriesInt->size()) {
          return false;
        }
      } else {
        // if numeric scalar, must have one bin
        if (ws->blocksize() != 1) {
          return false;
        }
      }
    } catch (std::invalid_argument &) {
      return false; // not numeric, neither numeric time series
    }
  }
  return true;
}

//----------------------------------------------------------------------------------------------
/** Tests the compatibility of the input workspaces
* @param ws : input workspace to test
* @param numSpec : number of spectra
* @param yUnit : unit of the y-values
* @param spectrumAxisUnit : name of the spectrum axis
* @param xUnit : x-axis unit
* @param instrumentName : name of the instrument
* @return : true if compatible
*/
bool JoinWorkspaces::checkCompatibility(MatrixWorkspace_sptr ws,
                                        const size_t numSpec,
                                        const std::string &xUnit,
                                        const std::string &yUnit,
                                        const std::string &spectrumAxisUnit,
                                        const std::string &instrumentName) {

  return (ws->getNumberHistograms() == numSpec && ws->YUnit() == yUnit &&
          ws->getAxis(1)->unit()->unitID() == spectrumAxisUnit &&
          ws->getAxis(0)->unit()->unitID() == xUnit &&
          ws->getInstrument()->getName() == instrumentName);
}

//----------------------------------------------------------------------------------------------
/** Flattens the list of group workspaces into list of workspaces
* @param inputs : input workspaces vector
* @return : the flat list of the input workspaces [inside] the groups
*/
std::vector<std::string> JoinWorkspaces::unWrapGroups(const std::vector<std::string> &inputs) {
    std::vector<std::string> outputs;

    for (const auto &input : inputs) {
      WorkspaceGroup_sptr wsgroup =
          AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(input);
      if (wsgroup) {
        // workspace group
        std::vector<std::string> group = wsgroup->getNames();
        outputs.insert(outputs.end(), group.begin(), group.end());
      } else {
        // single workspace
        outputs.push_back(input);
      }
    }
    return outputs;
}

//----------------------------------------------------------------------------------------------
/** Return the to-be axis of the workspace dependent on the log entry
* @param inputs : input workspace
* @param log : sample log entry
* @return : the [to-be] x-axis of the workspace
*/
std::vector<double> JoinWorkspaces::getXAxis(MatrixWorkspace_sptr ws, const std::string &log) {

  std::vector<double> axis;
  axis.reserve(ws->blocksize());

  if (log.empty()) {
    // return the actual x-axis of the first spectrum
    axis = ws->x(0).rawData();
  } else {
    auto run = ws->run();
    // try time series first
    TimeSeriesProperty<double> *timeSeriesDouble(nullptr);
    timeSeriesDouble =
        dynamic_cast<TimeSeriesProperty<double> *>(run.getLogData(log));
    if (timeSeriesDouble) {
      axis = timeSeriesDouble->filteredValuesAsVector();
    } else {
      // try int series next
      TimeSeriesProperty<int> *timeSeriesInt(nullptr);
      timeSeriesInt =
          dynamic_cast<TimeSeriesProperty<int> *>(run.getLogData(log));
      if (timeSeriesInt) {
        std::vector<int> intAxis = timeSeriesInt->filteredValuesAsVector();
        axis = std::vector<double>(intAxis.begin(), intAxis.end());
      } else {
        axis.push_back(run.getPropertyAsSingleValue(log));
      }
    }
  }
  return axis;
}

//----------------------------------------------------------------------------------------------
/** Joins the given spectrum for the list of workspaces
* @param inputs : list of input workspaces
* @param wsIndex : the workspace index
* @param out : the output workspace
*/
void JoinWorkspaces::joinSpectrum(std::list<MatrixWorkspace_sptr> inputs, long wsIndex,
                  MatrixWorkspace_sptr out) {
  std::vector<double> spectrum;
  std::vector<double> errors;
  spectrum.reserve(out->blocksize());
  errors.reserve(out->blocksize());
  size_t index = static_cast<size_t>(wsIndex);

  for (const auto &input : inputs) {
    auto y = input->y(index).rawData();
    auto e = input->e(index).rawData();
    spectrum.insert(spectrum.end(), y.begin(), y.end());
    errors.insert(errors.end(), e.begin(), e.end());
  }
  out->mutableY(index) = spectrum;
  out->mutableE(index) = errors;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void JoinWorkspaces::exec() {

  const std::vector<std::string> inputs_given =
      getProperty(INPUTWORKSPACEPROPERTY);
  const std::string log = getPropertyValue(SAMPLELOGXAXISPROPERTY);
  std::list<MatrixWorkspace_sptr> inputWS;
  size_t outBlockSize = 0;

  for (const auto &input : unWrapGroups(inputs_given)) {
    MatrixWorkspace_sptr ws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(input);
    inputWS.push_back(ws);
    outBlockSize += ws->blocksize();
  }

  std::vector<double> xAxis;
  xAxis.reserve(outBlockSize);
  auto first = inputWS.front();
  std::vector<double> axisFirst = getXAxis(first, log);

  xAxis.insert(xAxis.end(), axisFirst.begin(), axisFirst.end());

  auto outWS = WorkspaceFactory::Instance().create(
      first, first->getNumberHistograms(), outBlockSize, outBlockSize);

  SampleLogsBehaviour sampleLogsBehaviour = SampleLogsBehaviour(*first, g_log);

  auto it = inputWS.begin();

  // First sequentially build the x-axis and merge the sample logs
  for (++it; it != inputWS.end(); ++it) {
    std::vector<double> axisIt = getXAxis(*it, log);
    xAxis.insert(xAxis.end(), axisIt.begin(), axisIt.end());
    // Attempt to merge the sample logs
    // Let this throw if there is a log forbidding to merge
    sampleLogsBehaviour.mergeSampleLogs(**it, *outWS);
    sampleLogsBehaviour.setUpdatedSampleLogs(*outWS);
  }

  // Now loop in parallel over all the spectra and join the data
  PARALLEL_FOR_IF(threadSafe(outWS))
  for (long index = 0; index < static_cast<long>(first->getNumberHistograms());
       ++index) {
    PARALLEL_START_INTERUPT_REGION
    outWS->mutableX(static_cast<size_t>(index)) = xAxis;
    // TODO: think about the x-axis unit here
    joinSpectrum(inputWS, index, outWS);
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  setProperty("OutputWorkspace", outWS);
  }

} // namespace Algorithms
} // namespace Mantid
