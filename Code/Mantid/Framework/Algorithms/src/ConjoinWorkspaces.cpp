//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ConjoinWorkspaces.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/SpectraAxis.h"

namespace Mantid {
namespace Algorithms {

using std::size_t;
using namespace Kernel;
using namespace API;
using namespace DataObjects;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConjoinWorkspaces)

//----------------------------------------------------------------------------------------------
/// Default constructor
ConjoinWorkspaces::ConjoinWorkspaces()
    : WorkspaceJoiners(), m_overlapChecked(false) {}

//----------------------------------------------------------------------------------------------
/// Destructor
ConjoinWorkspaces::~ConjoinWorkspaces() {}

//----------------------------------------------------------------------------------------------
/** Initialize the properties */
void ConjoinWorkspaces::init() {
  declareProperty(
      new WorkspaceProperty<>("InputWorkspace1", "", Direction::InOut,
                              boost::make_shared<CommonBinsValidator>()),
      "The name of the first input workspace");
  declareProperty(
      new WorkspaceProperty<>("InputWorkspace2", "", Direction::Input,
                              boost::make_shared<CommonBinsValidator>()),
      "The name of the second input workspace");
  declareProperty(
      new PropertyWithValue<bool>("CheckOverlapping", true, Direction::Input),
      "Verify that the supplied data do not overlap");
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
  event_ws1 = boost::dynamic_pointer_cast<const EventWorkspace>(ws1);
  event_ws2 = boost::dynamic_pointer_cast<const EventWorkspace>(ws2);

  // Make sure that we are not mis-matching EventWorkspaces and other types of
  // workspaces
  if (((event_ws1) && (!event_ws2)) || ((!event_ws1) && (event_ws2))) {
    const std::string message("Only one of the input workspaces are of type "
                              "EventWorkspace; please use matching workspace "
                              "types (both EventWorkspace's or both "
                              "Workspace2D's).");
    g_log.error(message);
    throw std::invalid_argument(message);
  }

  if (event_ws1 && event_ws2) {
    // We do not need to check that binning is compatible, just that there is no
    // overlap
    // make sure we should bother checking
    if (this->getProperty("CheckOverlapping")) {
      this->checkForOverlap(event_ws1, event_ws2, false);
      m_overlapChecked = true;
    }

    // Both are event workspaces. Use the special method
    MatrixWorkspace_sptr output = this->execEvent();
    // Copy the history from the original workspace
    output->history().addHistory(ws1->getHistory());
    // Delete the second input workspace from the ADS
    AnalysisDataService::Instance().remove(getPropertyValue("InputWorkspace2"));
    // Set the result workspace to the first input
    setProperty("InputWorkspace1", output);
    return;
  }

  // Check that the input workspaces meet the requirements for this algorithm
  this->validateInputs(ws1, ws2);

  if (this->getProperty("CheckOverlapping")) {
    this->checkForOverlap(ws1, ws2, true);
    m_overlapChecked = true;
  }

  MatrixWorkspace_sptr output = execWS2D(ws1, ws2);
  // Copy the history from the original workspace
  output->history().addHistory(ws1->getHistory());

  // Delete the second input workspace from the ADS
  AnalysisDataService::Instance().remove(getPropertyValue("InputWorkspace2"));
  // Set the result workspace to the first input
  setProperty("InputWorkspace1", output);
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
void ConjoinWorkspaces::checkForOverlap(API::MatrixWorkspace_const_sptr ws1,
                                        API::MatrixWorkspace_const_sptr ws2,
                                        bool checkSpectra) const {
  // Loop through the first workspace adding all the spectrum numbers & UDETS to
  // a set
  std::set<specid_t> spectra;
  std::set<detid_t> detectors;
  const size_t &nhist1 = ws1->getNumberHistograms();
  for (size_t i = 0; i < nhist1; ++i) {
    const ISpectrum *spec = ws1->getSpectrum(i);
    const specid_t spectrum = spec->getSpectrumNo();
    spectra.insert(spectrum);
    const std::set<detid_t> &dets = spec->getDetectorIDs();
    std::set<detid_t>::const_iterator it;
    for (it = dets.begin(); it != dets.end(); ++it) {
      detectors.insert(*it);
    }
  }

  // Now go throught the spectrum numbers & UDETS in the 2nd workspace, making
  // sure that there's no overlap
  const size_t &nhist2 = ws2->getNumberHistograms();
  for (size_t j = 0; j < nhist2; ++j) {
    const ISpectrum *spec = ws2->getSpectrum(j);
    const specid_t spectrum = spec->getSpectrumNo();
    if (checkSpectra) {
      if (spectrum > 0 && spectra.find(spectrum) != spectra.end()) {
        g_log.error()
            << "The input workspaces have overlapping spectrum numbers "
            << spectrum << "\n";
        throw std::invalid_argument(
            "The input workspaces have overlapping spectrum numbers");
      }
    }
    const std::set<detid_t> &dets = spec->getDetectorIDs();
    std::set<detid_t>::const_iterator it;
    for (it = dets.begin(); it != dets.end(); ++it) {
      if (detectors.find(*it) != detectors.end()) {
        g_log.error() << "The input workspaces have common detectors: " << (*it)
                      << "\n";
        throw std::invalid_argument(
            "The input workspaces have common detectors");
      }
    }
  }
}

/***
 * This will ensure the spectrum numbers do not overlap by starting the second
 *on at the first + 1
 *
 * @param ws1 The first workspace supplied to the algorithm.
 * @param ws2 The second workspace supplied to the algorithm.
 * @param output The workspace that is going to be returned by the algorithm.
 */
void ConjoinWorkspaces::fixSpectrumNumbers(API::MatrixWorkspace_const_sptr ws1,
                                           API::MatrixWorkspace_const_sptr ws2,
                                           API::MatrixWorkspace_sptr output) {
  bool needsFix(false);

  if (this->getProperty("CheckOverlapping")) {
    // If CheckOverlapping is required, then either skip fixing spectrum number
    // or get stopped by an exception
    if (!m_overlapChecked)
      checkForOverlap(ws1, ws2, true);
    needsFix = false;
  } else {
    // It will be determined later whether spectrum number needs to be fixed.
    needsFix = true;
  }
  if (!needsFix)
    return;

  // is everything possibly ok?
  specid_t min;
  specid_t max;
  getMinMax(output, min, max);
  if (max - min >= static_cast<specid_t>(
                       output->getNumberHistograms())) // nothing to do then
    return;

  // information for remapping the spectra numbers
  specid_t ws1min;
  specid_t ws1max;
  getMinMax(ws1, ws1min, ws1max);

  // change the axis by adding the maximum existing spectrum number to the
  // current value
  for (size_t i = ws1->getNumberHistograms(); i < output->getNumberHistograms();
       i++) {
    specid_t origid;
    origid = output->getSpectrum(i)->getSpectrumNo();
    output->getSpectrum(i)->setSpectrumNo(origid + ws1max);
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

} // namespace Algorithm
} // namespace Mantid
