#include "MantidMDAlgorithms/IntegratePeaksCWSD.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidGeometry/IDetector.h"
#include "MantidDataObjects/Peak.h"
#include "MantidKernel/ArrayProperty.h"

/*
#include "MantidDataObjects/PeakShapeSpherical.h"
#include "MantidKernel/System.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/CoordTransformDistance.h"
#include "MantidKernel/ListValidator.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/TextAxis.h"
#include "MantidKernel/Utils.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/Column.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/Progress.h"
#include <fstream>
*/

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(IntegratePeaksCWSD)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

const signal_t THRESHOLD_SIGNAL = 0;

//----------------------------------------------------------------------------------------------
/** Constructor
 */
IntegratePeaksCWSD::IntegratePeaksCWSD()
    : m_haveSinglePeakCenter(false), m_doMergePeak(false) {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
IntegratePeaksCWSD::~IntegratePeaksCWSD() {}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void IntegratePeaksCWSD::init() {
  declareProperty(new WorkspaceProperty<IMDEventWorkspace>("InputWorkspace", "",
                                                           Direction::Input),
                  "An input MDEventWorkspace.");

  declareProperty(new WorkspaceProperty<PeaksWorkspace>("PeaksWorkspace", "",
                                                        Direction::Input),
                  "A PeaksWorkspace containing the peaks to integrate.");

  declareProperty(
      new WorkspaceProperty<PeaksWorkspace>("OutputWorkspace", "",
                                            Direction::Output),
      "The output PeaksWorkspace will be a copy of the input PeaksWorkspace "
      "with the peaks' integrated intensities.");

  declareProperty(
      new WorkspaceProperty<DataObjects::MaskWorkspace>(
          "MaskWorkspace", "", Direction::Input, PropertyMode::Optional),
      "Output Masking Workspace");

  declareProperty(
      new ArrayProperty<double>("PeakCentre"),
      "A comma separated list for peak centre in Q-sample frame. "
      "Its length is either 3 (Qx, Qy, Qz) or 0. "
      "It is applied in the case that there are more than 1 run numbers.");

  declareProperty("PeakRadius", EMPTY_DBL(), "Radius of a peak.");

  declareProperty(
      "MergePeaks", true,
      "In case that there are more than 1 run number in the given "
      "PeaksWorkspace "
      "and MDEVentWorkspace, if it is set to true, then the peaks' intensities "
      "will be merged.");
}

//----------------------------------------------------------------------------------------------
/**
 * @brief IntegratePeaksCWSD::exec
 */
void IntegratePeaksCWSD::exec() {
  // Process input & check
  processInputs();

  // Integrate peak with simple algorithm
  simplePeakIntegration(vecMaskedDetID, monitorCountMap);

  // Merge peak if necessary
  if (m_doMergePeak)
    mergePeaks();

  // Output
  DataObjects::PeaksWorkspace_sptr outws = createOutputs();
  setProperty("OutputWorkspace", outws);
}

//----------------------------------------------------------------------------------------------
/** Process and check input properties
 */
void IntegratePeaksCWSD::processInputs() {
  // Required input workspaces
  m_inputWS = getProperty("InputWorkspace");
  m_peaksWS = getProperty("PeaksWorkspace");

  // monitor counts
  monitorCountMap = getMonitorCounts();

  // go through peak
  getPeakInformation();
  if (m_runPeakCenterMap.size() > 1)
    m_haveMultipleRun = true;
  else
    m_haveMultipleRun = false;

  // peak related
  m_peakRadius = getProperty("PeakRadius");
  assert(m_peakRadius != EMPTY_DBL());

  // merge peak only makes sense when there is more than 1 peak
  if (m_haveMultipleRun)
    m_doMergePeak = getProperty("MergePeaks");
  else
    m_doMergePeak = false;

  std::vector<double> peak_center = getProperty("PeakCentre");
  if (peak_center.size() == 0) {
    // no single peak center
    assert(!m_doMergePeak &&
           "In order to merge peak, peak center must be given!");
  } else {
    // assigned peak center
    assert(peak_center.size() == 3 && "PeakCentre must have 3 elements.");
    m_peakCenter.setX(peak_center[0]);
    m_peakCenter.setY(peak_center[1]);
    m_peakCenter.setZ(peak_center[2]);
  }

  // optional mask workspace
  std::string maskwsname = getPropertyValue("MaskWorkspace");
  if (maskwsname.size() > 0) {
    // process mask workspace
    m_maskDets = true;
    m_maskWS = getProperty("MaskWorkspace");
    vecMaskedDetID = processMaskWorkspace(m_maskWS);
  } else {
    m_maskDets = false;
  }
}

//----------------------------------------------------------------------------------------------
/**
 * @brief IntegratePeaksCWSD::simplePeakIntegration
 * Purpose:
 *   Integrate a single crystal peak with the simplest algorithm, i.e.,
 *   by adding all the signal with normalization to monitor counts
 * Requirements:
 *   Valid MDEventWorkspace
 *   Valid PeaksWorkspace
 * Guarantees:
 *   A valid value is given
 */
void IntegratePeaksCWSD::simplePeakIntegration(
    const std::vector<detid_t> &vecMaskedDetID,
    const std::map<int, signal_t> &run_monitor_map) {
  // Check requirements
  assert(m_inputWS && "MDEventWorkspace is not defined.");
  assert(m_peaksWS && "PeaksWorkspace is not defined.");

  // Go through to get value
  API::IMDIterator *mditer = m_inputWS->createIterator();
  size_t nextindex = 1;
  bool scancell = true;
  // size_t currindex = 0;

  // Assuming that MDEvents are grouped by run number, there is no need to
  // loop up the map for peak center and monitor counts each time
  int current_run_number = -1;
  signal_t current_monitor_counts = 0;
  Kernel::V3D current_peak_center = m_peakCenter;

  // signal_t total_signal = 0;
  double min_distance = 10000000;
  double max_distance = -1;
  while (scancell) {
    // Go through all the MDEvents in one cell.
    size_t numeventincell = mditer->getNumEvents();

    for (size_t iev = 0; iev < numeventincell; ++iev) {
      // Get signal to add and skip if signal is zero
      signal_t signal = mditer->getInnerSignal(iev);
      if (signal <= THRESHOLD_SIGNAL)
        continue;

      // Check whether this detector is masked
      if (vecMaskedDetID.size() > 0) {
        detid_t detid = mditer->getInnerDetectorID(iev);
        std::vector<detid_t>::const_iterator it;

        it = find(vecMaskedDetID.begin(), vecMaskedDetID.end(), detid);
        if (it != vecMaskedDetID.end()) {
          // The detector ID is found among masked detector IDs
          // Skip this event and move to next
          continue;
        }
      }

      // Check whether to update monitor counts and peak center
      uint16_t run_number = mditer->getInnerRunIndex(iev);
      int run_number_i = static_cast<int>(run_number);
      if (current_run_number != run_number_i) {
        // update run number
        current_run_number = run_number_i;
        // update monitor counts
        std::map<int, signal_t>::const_iterator m_finder =
            run_monitor_map.find(current_run_number);
        if (m_finder != run_monitor_map.end())
          current_monitor_counts = m_finder->second;
        else {
          std::stringstream errss;
          errss << "Unable to find run number " << current_run_number
                << " in monitor counts map";
          throw std::runtime_error(errss.str());
        }

        // update peak center
        if (!m_haveSinglePeakCenter)
          current_peak_center = m_runPeakCenterMap[current_run_number];
      }

      // calculate distance
      float tempx = mditer->getInnerPosition(iev, 0);
      float tempy = mditer->getInnerPosition(iev, 1);
      float tempz = mditer->getInnerPosition(iev, 2);
      Kernel::V3D pixel_pos(tempx, tempy, tempz);
      double distance = current_peak_center.distance(pixel_pos);
      if (distance < m_peakRadius) {
        // FIXME - Is it very costly to use map each time???
        // total_signal += signal/current_monitor_counts;
        m_runPeakCountsMap[run_number] += signal / current_monitor_counts;
      }

      if (distance < min_distance)
        min_distance = distance;
      if (distance > max_distance)
        max_distance = distance;
    }

    // Advance to next cell
    if (mditer->next()) {
      // advance to next cell
      mditer->jumpTo(nextindex);
      ++nextindex;
    } else {
      // break the loop
      scancell = false;
    }
  } // END-WHILE (scan-cell)

  // Summarize
  g_log.notice() << "Distance range is " << min_distance << ", " << max_distance
                 << "\n";

  return;
}

//----------------------------------------------------------------------------------------------
/** Purpose: Process mask workspace
 *  Requirement: m_maskWS is not None
 *  Guarantees: an array will be set up for masked detectors
 * @brief IntegratePeaksCWSD::processMaskWorkspace
 * @param maskws
 */
std::vector<detid_t> IntegratePeaksCWSD::processMaskWorkspace(
    DataObjects::MaskWorkspace_const_sptr maskws) {
  std::vector<detid_t> vecMaskedDetID;

  // Add the detector IDs of all masked detector to a vector
  size_t numspec = maskws->getNumberHistograms();
  for (size_t iws = 0; iws < numspec; ++iws) {
    Geometry::IDetector_const_sptr detector = maskws->getDetector(iws);
    if (detector->isMasked()) {
      detid_t detid = detector->getID();
      vecMaskedDetID.push_back(detid);
    }
  }

  // Sort the vector for future lookup
  if (vecMaskedDetID.size() > 1)
    std::sort(vecMaskedDetID.begin(), vecMaskedDetID.end());

  return vecMaskedDetID;
}

//----------------------------------------------------------------------------------------------
/** Merge the peaks' counts
 * @brief IntegratePeaksCWSD::mergePeaks
 */
void IntegratePeaksCWSD::mergePeaks() {
  double total_intensity = 0;
  double total_monitor_counts = 0.;

  // sum over all runs
  std::map<int, signal_t>::iterator mon_iter;
  for (mon_iter = monitorCountMap.begin(); mon_iter != monitorCountMap.end();
       ++mon_iter) {
    int run_number_i = mon_iter->first;
    signal_t monitor_i = mon_iter->second;
    double intensity_i = m_runPeakCountsMap[run_number_i];
    total_intensity += monitor_i * intensity_i;
    total_monitor_counts += monitor_i;
  }

  // final merged intensity
  double merged_intensity = total_intensity / total_monitor_counts;

  // set the merged intensity to each peak
  std::map<int, double>::iterator count_iter;
  for (count_iter = m_runPeakCountsMap.begin();
       count_iter != m_runPeakCountsMap.end(); ++count_iter)
    count_iter->second = merged_intensity;
}

//----------------------------------------------------------------------------------------------
/** Create otuput workspace
 * @brief IntegratePeaksCWSD::createOutputs
 * @return
 */
DataObjects::PeaksWorkspace_sptr IntegratePeaksCWSD::createOutputs() {
  // clone the original peaks workspace
  DataObjects::PeaksWorkspace_sptr outws = boost::shared_ptr<DataObjects::PeaksWorkspace>(
        m_peaksWS->clone().release());

  size_t num_peaks = outws->getNumberPeaks();
  for (size_t i_peak = 0; i_peak < num_peaks; ++i_peak) {
    DataObjects::Peak &peak_i = outws->getPeak(static_cast<int>(i_peak));
    int run_number_i = peak_i.getRunNumber();
    double intensity_i = m_runPeakCountsMap[run_number_i];
    peak_i.setIntensity(intensity_i);
  }

  return outws;
}

//----------------------------------------------------------------------------------------------
/**
 * @brief IntegratePeaksCWSD::getMonitorCounts
 * @return
 */
std::map<int, signal_t> IntegratePeaksCWSD::getMonitorCounts() {
  std::map<int, signal_t> run_monitor_map;

  uint16_t num_expinfo = m_inputWS->getNumExperimentInfo();
  for (size_t iexpinfo = 0; iexpinfo < num_expinfo; ++iexpinfo) {
    ExperimentInfo_const_sptr expinfo =
        m_inputWS->getExperimentInfo(static_cast<uint16_t>(iexpinfo));
    std::string run_str = expinfo->run().getProperty("run_number")->value();
    uint16_t run_number = static_cast<uint16_t>(atoi(run_str.c_str()));
    std::string mon_str = expinfo->run().getProperty("monitor")->value();
    signal_t monitor = static_cast<signal_t>(atoi(mon_str.c_str()));
    run_monitor_map.insert(
        std::make_pair(static_cast<int>(run_number), monitor));
    g_log.notice() << "[DB] Add run " << run_number << ", monitor = " << monitor
                   << "\n";
  }

  return run_monitor_map;
}

//----------------------------------------------------------------------------------------------
/** Get peak information from peaks workspace
 * @return
 */
void IntegratePeaksCWSD::getPeakInformation() {
  m_vecPeaks = m_peaksWS->getPeaks();
  size_t numpeaks = m_vecPeaks.size();
  for (size_t ipeak = 0; ipeak < numpeaks; ++ipeak) {
    DataObjects::Peak &peak = m_vecPeaks[ipeak];
    int run_number = peak.getRunNumber();
    Mantid::Kernel::V3D qsample = peak.getQSampleFrame();
    m_runPeakCenterMap.insert(std::make_pair(run_number, qsample));

    // set up the data structure to store integrated peaks' counts
    m_runPeakCountsMap.insert(std::make_pair(run_number, 0.));

    g_log.notice() << "[DB] Q sample = " << qsample.toString() << "\n";
  }
}

} // namespace Mantid
} // namespace MDAlgorithms
