#include "MantidAlgorithms/JoinWorkspaces.h"

#include "MantidAlgorithms/MergeRuns/SampleLogsBehaviour.h"
#include "MantidAPI/ADSValidator.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/make_unique.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/TimeSeriesProperty.h"

namespace Mantid {
namespace Algorithms {

using namespace Mantid::API;
using namespace Mantid::Kernel;

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
}

std::map<std::string, std::string> JoinWorkspaces::validateInputs() {
  std::map<std::string, std::string> issues;

  std::vector<std::string> inputs_given = getProperty(INPUTWORKSPACEPROPERTY);
  std::string log = getPropertyValue(SAMPLELOGXAXISPROPERTY);

  // collect here the list of input workspaces expanded from the groups if any
  std::vector<std::string> inputs;

  for (const auto &input : inputs_given) {
    WorkspaceGroup_sptr wsgroup =
        AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(input);
    if (wsgroup) {
      // workspace group
      std::vector<std::string> group = wsgroup->getNames();
      inputs.insert(inputs.end(), group.begin(), group.end());
    } else {
      // single workspace
      inputs.emplace_back(input);
    }
  }

  // find the workspaces that are not Matrix or not a distribution
  for (const auto& input : inputs) {
    MatrixWorkspace_const_sptr ws =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(input);
    if (!ws) {
      issues[INPUTWORKSPACEPROPERTY] +=
          "Workspace " + ws->getName() + " is not a MatrixWorkspace\n";
    } else if (!ws->isDistribution()) {
      issues[INPUTWORKSPACEPROPERTY] +=
          "Workspace " + ws->getName() + " is not a distribution\n";
    } else {
        m_inputWS.emplace_back(ws);
    }
  }

  // we need at least 2 valid input workspaces to perform join operation
  if (m_inputWS.size() < 2) {
    issues[INPUTWORKSPACEPROPERTY] += "There are less than 2 point data"
                                      " MatrixWorkspaces in the input list\n";
  } else {
    // extracts the properties of the first workspace
    const size_t numSpec = m_inputWS.front()->getNumberHistograms();
    const std::string yUnit = m_inputWS.front()->YUnit();
    const std::string spectrumAxisUnit =
        m_inputWS.front()->getAxis(1)->unit()->unitID();
    const std::string xUnit = m_inputWS.front()->getAxis(0)->unit()->unitID();
    const std::string instrumentName =
        m_inputWS.front()->getInstrument()->getName();

    for (const auto &ws : m_inputWS) {

      // check if all others are compatible with the first one
      if (ws != m_inputWS.front()) {
        if (!testCompatibility(ws, numSpec, yUnit, spectrumAxisUnit, xUnit,
                               instrumentName)) {
          issues[INPUTWORKSPACEPROPERTY] +=
              "Workspace " + ws->getName() + " is not compatible\n.";
        }
      }

      // if the log entry is given, validate it
      if (!log.empty() && !testLogEntry(ws, log)) {
        issues[SAMPLELOGXAXISPROPERTY] +=
            "Invalid sample log entry for " + ws->getName() + "\n.";
      }
    }
  }
  return issues;
}

//----------------------------------------------------------------------------------------------
/** Tests the compatibility of the input workspaces
* @param ws : input workspace to test
* @param log : the sample log entry name
* @return : true if the log exists, is numeric, and matches the size of the
* workspace
*/
bool JoinWorkspaces::testLogEntry(MatrixWorkspace_const_sptr ws,
                                  const std::string &log) {
  auto &run = ws->run();
  if (!run.hasProperty(log))
    return false;
  else {
    const std::string value = run.getLogData(log)->value();
    double doubleValue;
    int intValue;
    if (Strings::convert(value, doubleValue) ||
        Strings::convert(value, intValue)) {
      if (ws->blocksize() != 1) {
        return false;
      }
    } else {
      TimeSeriesProperty<double> *timeSeriesDouble(nullptr);
      TimeSeriesProperty<int> *timeSeriesInt(nullptr);
      timeSeriesDouble =
          dynamic_cast<TimeSeriesProperty<double> *>(run.getLogData(log));
      timeSeriesInt =
          dynamic_cast<TimeSeriesProperty<int> *>(run.getLogData(log));
      if (timeSeriesDouble &&
          static_cast<int>(ws->blocksize()) != timeSeriesDouble->size()) {
        return false;
      }
      if (timeSeriesInt &&
          static_cast<int>(ws->blocksize()) != timeSeriesInt->size()) {
        return false;
      }
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
bool JoinWorkspaces::testCompatibility(MatrixWorkspace_const_sptr ws,
                                       const size_t numSpec,
                                       const std::string &yUnit,
                                       const std::string &spectrumAxisUnit,
                                       const std::string &xUnit,
                                       const std::string &instrumentName) {

  return (ws->getNumberHistograms() != numSpec || ws->YUnit() != yUnit ||
          ws->getAxis(1)->unit()->unitID() != spectrumAxisUnit ||
          ws->getAxis(0)->unit()->unitID() != xUnit ||
          ws->getInstrument()->getName() != instrumentName);
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void JoinWorkspaces::exec() {
  // TODO Auto-generated execute stub
}

} // namespace Algorithms
} // namespace Mantid
