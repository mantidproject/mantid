// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/RunCombinationHelpers/RunCombinationHelper.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/Unit.h"

namespace Mantid {
namespace Algorithms {

using namespace API;
using namespace Geometry;
using namespace Kernel;

//----------------------------------------------------------------------------------------------
/** Flattens the list of group workspaces (if any) into list of workspaces
 * @param inputs : input workspaces vector [including] group workspaces (all
 * must be on ADS)
 * @return : the flat vector of the input workspaces
 * @throw : std::runtime_error if the input workspaces are neither groups nor
 * MatrixWorkspaces
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
      // MatrixWorkspace
      MatrixWorkspace_sptr matrixws =
          AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(input);
      if (matrixws)
        outputs.push_back(matrixws->getName());
      else
        throw(std::runtime_error(
            "The input " + input +
            " is neither a WorkspaceGroup nor a MatrixWorkspace"));
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
  if (m_numberSpectra) {
    m_hasDx.reserve(m_numberSpectra);
    for (unsigned int i = 0; i < m_numberSpectra; ++i)
      m_hasDx.push_back(ref->hasDx(i));
  }
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
  std::string errors;
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
  if (ws->getNumberHistograms() == m_numberSpectra) {
    if (!m_hasDx.empty()) {
      for (unsigned int i = 0; i < m_numberSpectra; ++i) {
        if (m_hasDx[i] != ws->hasDx(i)) {
          errors += "spectra must have either Dx values or not; ";
          break;
        }
      }
    }
  }
  return errors;
}

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
    inWS.emplace_back(ws);
    // Check that it has common binning
    if (!inWS.back()->isCommonBins()) {
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
  return inWS;
}

} // namespace Algorithms
} // namespace Mantid
