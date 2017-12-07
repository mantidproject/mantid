#include "MantidAlgorithms/RunCombinationHelpers/RunCombinationHelper.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Unit.h"

namespace Mantid {
namespace Algorithms {

using namespace API;
using namespace Geometry;
using namespace Kernel;

//----------------------------------------------------------------------------------------------
/** Flattens the list of group workspaces (if any) into list of workspaces
* @param inputs : input workspaces vector [including] group workspaces (all must
* be on ADS)
* @return : the flat vector of the input workspaces
*/
std::vector<std::string>
RunCombinationHelper::unWrapGroups(const std::vector<std::string> &inputs) {
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
/** Sets the properties of the reference (usually first) workspace,
 * to later check the compatibility of the others with the reference
 * @param ref : the reference workspace
 */
void RunCombinationHelper::setReferenceProperties(MatrixWorkspace_sptr ref) {
  m_numberSpectra = ref->getNumberHistograms();
  m_numberDetectors = ref->detectorInfo().size();
  m_xUnit = ref->getAxis(0)->unit()->unitID();
  m_spectrumAxisUnit = ref->getAxis(1)->unit()->unitID();
  m_yUnit = ref->YUnit();
  m_isHistogramData = ref->isHistogramData();
  m_isScanning = ref->detectorInfo().isScanning();
  m_instrumentName = ref->getInstrument()->getName();
}

//----------------------------------------------------------------------------------------------
/** Compares the properties of the input workspace with the reference
 * @param ws : the testee workspace
 * @param checkNumberHistograms : whether to check also the number of histograms
 * @return : empty if compatible, error message otherwises
 */
std::string
RunCombinationHelper::checkCompatibility(MatrixWorkspace_sptr ws,
                                         bool checkNumberHistograms) {
  std::string errors = "";
  if (ws->getNumberHistograms() != m_numberSpectra && checkNumberHistograms)
    errors += "different number of histograms; ";
  if (ws->getAxis(0)->unit()->unitID() != m_xUnit)
    errors += "different X units; ";
  if (ws->getAxis(1)->unit()->unitID() != m_spectrumAxisUnit)
    errors += "different spectrum axis units; ";
  if (ws->YUnit() != m_yUnit)
    errors += "different Y units; ";
  if (ws->isHistogramData() != m_isHistogramData)
    errors += "different distribution or histogram type; ";
  if (ws->detectorInfo().isScanning() != m_isScanning)
    errors += "a mix of workspaces with and without detector scans; ";
  if (m_isScanning && ws->detectorInfo().size() != m_numberDetectors)
    errors += "workspaces with detectors scans have different number of "
              "detectors; ";
  if (ws->getInstrument()->getName() != m_instrumentName)
    errors += "different instrument names; ";
  return errors;
}

/// @cond
// Local function used within validateInputWorkspaces() below in a call to
// std::list::sort(compare) to order the input workspaces by the start of their
// frame (i.e. the first X value).
static bool compare(MatrixWorkspace_sptr &first, MatrixWorkspace_sptr &second) {
  return (first->x(0).front() < second->x(0).front());
}
/// @endcond

/** Checks that the input workspace all exist, that they are the same size, have
 * the same units
 *  and the same instrument name. Will throw if they don't.
 *  @param  inputWorkspaces The names of the input workspaces
 *  @param g_log A logger to send error messages to
 *  @return A list of pointers to the input workspace, ordered by increasing
 * frame starting point
 *  @throw  Exception::NotFoundError If an input workspace doesn't exist
 *  @throw  std::invalid_argument    If the input workspaces are not compatible
 */
std::list<API::MatrixWorkspace_sptr>
RunCombinationHelper::validateInputWorkspaces(
    const std::vector<std::string> &inputWorkspaces, Logger &g_log) {
  std::list<MatrixWorkspace_sptr> inWS;

  for (size_t i = 0; i < inputWorkspaces.size(); ++i) {
    MatrixWorkspace_sptr ws;
    // Fetch the next input workspace - throw an error if it's not there
    ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        inputWorkspaces[i]);
    if (!ws) {
      throw std::runtime_error(
          "Could not find a MatrixWorkspace with the name " +
          inputWorkspaces[i]);
    }
    inWS.push_back(ws);
    // Check that it has common binning
    if (!WorkspaceHelpers::commonBoundaries(*inWS.back())) {
      g_log.error("Input workspaces must have common binning for all spectra");
      throw std::invalid_argument(
          "Input workspaces must have common binning for all spectra");
    }
    // Check a few things are the same for all input workspaces
    if (i == 0) {
      setReferenceProperties(ws);
    } else {
      std::string compatibility = checkCompatibility(ws);
      if (!compatibility.empty()) {
        g_log.error("Input workspaces are not compatible: " + compatibility);
        throw std::invalid_argument("Input workspaces are not compatible: " +
                                    compatibility);
      }
    }
  }

  // Order the workspaces by ascending frame (X) starting point
  inWS.sort(compare);

  return inWS;
}

} // namespace Algorithms
} // namespace Mantid
