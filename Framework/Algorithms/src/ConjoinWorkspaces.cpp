// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ConjoinWorkspaces.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/CommonBinsValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectraAxis.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidDataObjects/EventWorkspace.h"

namespace Mantid::Algorithms {

using std::size_t;
using namespace Kernel;
using namespace API;
using namespace DataObjects;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConjoinWorkspaces)

//----------------------------------------------------------------------------------------------
/** Initialize the properties */
void ConjoinWorkspaces::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace1", "", Direction::InOut),
                  "The name of the first input workspace");
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace2", "", Direction::Input),
                  "The name of the second input workspace");
  declareProperty(std::make_unique<PropertyWithValue<bool>>("CheckOverlapping", true, Direction::Input),
                  "Verify that the supplied data do not overlap");
  declareProperty(std::make_unique<PropertyWithValue<std::string>>("YAxisLabel", "", Direction::Input),
                  "The label to set the Y axis to");
  declareProperty(std::make_unique<PropertyWithValue<std::string>>("YAxisUnit", "", Direction::Input),
                  "The unit to set the Y axis to");
  declareProperty(std::make_unique<PropertyWithValue<bool>>("CheckMatchingBins", true, Direction::Input),
                  "If true, the algorithm will check that the two input workspaces have matching bins.");
}

//----------------------------------------------------------------------------------------------
/** Executes the algorithm
 *  @throw std::invalid_argument If the input workspaces do not meet the
 * requirements of this algorithm
 */
void ConjoinWorkspaces::exec() {
  // Retrieve the input workspaces
  MatrixWorkspace_const_sptr ws1 = getProperty("InputWorkspace1");
  MatrixWorkspace_const_sptr ws2 = getProperty("InputWorkspace2");
  DataObjects::EventWorkspace_const_sptr eventWs1 = std::dynamic_pointer_cast<const EventWorkspace>(ws1);
  DataObjects::EventWorkspace_const_sptr eventWs2 = std::dynamic_pointer_cast<const EventWorkspace>(ws2);

  // Make sure that we are not mis-matching EventWorkspaces and other types of
  // workspaces
  if (((eventWs1) && (!eventWs2)) || ((!eventWs1) && (eventWs2))) {
    const std::string message("Only one of the input workspaces are of type "
                              "EventWorkspace; please use matching workspace "
                              "types (both EventWorkspace or both "
                              "Workspace2D).");
    g_log.error(message);
    throw std::invalid_argument(message);
  }

  // Check whether bins match
  bool checkBins = getProperty("CheckMatchingBins");
  if (checkBins && !checkBinning(ws1, ws2)) {
    const std::string message("The bins do not match in the input workspaces. "
                              "Consider using RebinToWorkspace to preprocess "
                              "the workspaces before conjoining them.");
    g_log.error(message);
    throw std::invalid_argument(message);
  }

  if (eventWs1 && eventWs2) {
    this->checkCompatibility(*eventWs1, *eventWs2);
    auto output = conjoinEvents(*eventWs1, *eventWs2);
    setYUnitAndLabel(*output);
    // Set the result workspace to the first input
    setProperty("InputWorkspace1", output);
  } else {
    auto output = conjoinHistograms(*ws1, *ws2);
    setYUnitAndLabel(*output);
    // Set the result workspace to the first input
    setProperty("InputWorkspace1", output);
  }

  // Delete the second input workspace from the ADS
  AnalysisDataService::Instance().remove(getPropertyValue("InputWorkspace2"));
}

//----------------------------------------------------------------------------------------------
/** Checks whether the binning is consistent between two workspaces
 *  @param ws1 :: The first input workspace
 *  @param ws2 :: The second input workspace
 *  @return :: true if both workspaces have consistent binning.
 */
bool ConjoinWorkspaces::checkBinning(const API::MatrixWorkspace_const_sptr &ws1,
                                     const API::MatrixWorkspace_const_sptr &ws2) const {
  if (ws1->isRaggedWorkspace() || ws2->isRaggedWorkspace()) {
    return false;
  }

  // If neither workspace is ragged, we only need to check the first specrum.
  // Otherwise the matchingBins() function requires the two workspaces to have
  // the same number of spectra.
  return WorkspaceHelpers::matchingBins(ws1, ws2, true);
}

//----------------------------------------------------------------------------------------------
/** Checks that the two input workspaces have non-overlapping spectra numbers
 * and contributing detectors
 *  @param ws1 :: The first input workspace
 *  @param ws2 :: The second input workspace
 *  @param checkSpectra :: set to true to check for overlapping spectra numbers
 * (non-sensical for event workspaces)
 *  @throw std::invalid_argument If there is some overlap
 */
void ConjoinWorkspaces::checkForOverlap(const MatrixWorkspace &ws1, const MatrixWorkspace &ws2,
                                        bool checkSpectra) const {
  // Loop through the first workspace adding all the spectrum numbers & UDETS to
  // a set
  std::set<specnum_t> spectra;
  std::set<detid_t> detectors;
  const size_t &nhist1 = ws1.getNumberHistograms();
  for (size_t i = 0; i < nhist1; ++i) {
    const auto &spec = ws1.getSpectrum(i);
    const specnum_t spectrum = spec.getSpectrumNo();
    spectra.insert(spectrum);
    const auto &dets = spec.getDetectorIDs();
    for (auto const &det : dets) {
      detectors.insert(det);
    }
  }

  // Now go throught the spectrum numbers & UDETS in the 2nd workspace, making
  // sure that there's no overlap
  const size_t &nhist2 = ws2.getNumberHistograms();
  for (size_t j = 0; j < nhist2; ++j) {
    const auto &spec = ws2.getSpectrum(j);
    const specnum_t spectrum = spec.getSpectrumNo();
    if (checkSpectra) {
      if (spectrum > 0 && spectra.find(spectrum) != spectra.end()) {
        g_log.error() << "The input workspaces have overlapping spectrum numbers " << spectrum << "\n";
        throw std::invalid_argument("The input workspaces have overlapping spectrum numbers");
      }
    }
    const auto &dets = spec.getDetectorIDs();
    const auto it = std::find_if(dets.cbegin(), dets.cend(),
                                 [&detectors](const auto &det) { return detectors.find(det) != detectors.cend(); });
    if (it != dets.cend()) {
      g_log.error() << "The input workspaces have common detectors: " << (*it) << std::endl;
      throw std::invalid_argument("The input workspaces have common detectors");
    }
  }
}

/**
 * Conjoin two event workspaces together, including the history
 *
 * @param ws1:: The first workspace
 * @param ws2:: The second workspace
 * @return :: A new workspace containing the conjoined workspaces
 */
API::MatrixWorkspace_sptr ConjoinWorkspaces::conjoinEvents(const DataObjects::EventWorkspace &ws1,
                                                           const DataObjects::EventWorkspace &ws2) {
  this->checkCompatibility(ws1, ws2);

  // Check there is no overlap
  if (this->getProperty("CheckOverlapping")) {
    this->checkForOverlap(ws1, ws2, false);
    m_overlapChecked = true;
  }

  // Both are event workspaces. Use the special method
  auto output = this->execEvent(ws1, ws2);

  // Copy the history from the original workspace
  output->history().addHistory(ws1.getHistory());
  return output;
}

/**
 * Conjoin two histogram workspaces together, including the history
 *
 * @param ws1:: The first workspace
 * @param ws2:: The second workspace
 * @return :: A new workspace containing the conjoined workspaces
 */
API::MatrixWorkspace_sptr ConjoinWorkspaces::conjoinHistograms(const API::MatrixWorkspace &ws1,
                                                               const API::MatrixWorkspace &ws2) {
  // Check that the input workspaces meet the requirements for this algorithm
  this->checkCompatibility(ws1, ws2);

  if (this->getProperty("CheckOverlapping")) {
    this->checkForOverlap(ws1, ws2, true);
    m_overlapChecked = true;
  }

  auto output = execWS2D(ws1, ws2);

  // Copy the history from the original workspace
  output->history().addHistory(ws1.getHistory());
  return output;
}

/***
 * This will ensure the spectrum numbers do not overlap by starting the second
 *on at the first + 1
 *
 * @param ws1 The first workspace supplied to the algorithm.
 * @param ws2 The second workspace supplied to the algorithm.
 * @param output The workspace that is going to be returned by the algorithm.
 */
void ConjoinWorkspaces::fixSpectrumNumbers(const MatrixWorkspace &ws1, const MatrixWorkspace &ws2,
                                           MatrixWorkspace &output) {

  if (this->getProperty("CheckOverlapping")) {
    // If CheckOverlapping is required, then either skip fixing spectrum number
    // or get stopped by an exception
    if (!m_overlapChecked)
      // This throws if the spectrum numbers overlap
      checkForOverlap(ws1, ws2, true);
    // At this point, we don't have to do anything
    return;
  }

  // Because we were told not to check overlapping, fix up any errors we might
  // run into
  specnum_t min = -1;
  specnum_t max = -1;
  getMinMax(output, min, max);
  if (max - min >= static_cast<specnum_t>(output.getNumberHistograms())) // nothing to do then
    return;

  // information for remapping the spectra numbers
  specnum_t ws1min = -1;
  specnum_t ws1max = -1;
  getMinMax(ws1, ws1min, ws1max);

  // change the axis by adding the maximum existing spectrum number to the
  // current value
  for (size_t i = ws1.getNumberHistograms(); i < output.getNumberHistograms(); i++) {
    specnum_t origid = output.getSpectrum(i).getSpectrumNo();
    output.getSpectrum(i).setSpectrumNo(origid + ws1max);
  }
}

/// Appends the removal of the empty group after execution to the
/// Algorithm::processGroups() method
bool ConjoinWorkspaces::processGroups() {
  // Call the base class method for most of the functionality
  const bool retval = Algorithm::processGroups();

  // If that was successful, remove the now empty group in the second input
  // workspace property
  if (retval)
    AnalysisDataService::Instance().remove(getPropertyValue("InputWorkspace2"));

  return retval;
}

void ConjoinWorkspaces::setYUnitAndLabel(API::MatrixWorkspace &ws) const {
  const std::string yLabel = getPropertyValue("YAXisLabel");
  const std::string yUnit = getPropertyValue("YAxisUnit");

  // Unit must be moved before label, as changing the unit resets the label
  if (!yUnit.empty())
    ws.setYUnit(yUnit);

  if (!yLabel.empty())
    ws.setYUnitLabel(yLabel);
}

} // namespace Mantid::Algorithms
