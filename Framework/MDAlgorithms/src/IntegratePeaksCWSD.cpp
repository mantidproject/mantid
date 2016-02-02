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
IntegratePeaksCWSD::IntegratePeaksCWSD() {}

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

  declareProperty(new WorkspaceProperty<DataObjects::MaskWorkspace>(
                      "MaskWorkspace", "", Direction::Input, PropertyMode::Optional),
                  "Output Masking Workspace");

  declareProperty(new ArrayProperty<double>("PeakCentre"),
                  "A comma separated list for peak centre in Q-sample frame. "
                  "Its length is either 3 (Qx, Qy, Qz) or 0. "
                  "It is applied in the case that there are more than 1 run numbers.");

  declareProperty("PeakRadius", EMPTY_DBL(),
                  "Radius of a peak.");

  declareProperty("MergePeaks", true,
                  "In case that there are more than 1 run number in the given PeaksWorkspace "
                  "and MDEVentWorkspace, if it is set to true, then the peaks' intensities "
                  "will be merged.");
}

//----------------------------------------------------------------------------------------------
/**
 * @brief IntegratePeaksCWSD::exec
 */
void IntegratePeaksCWSD::exec()
{
  // Process input & check
  m_inputWS = getProperty("InputWorkspace");
  m_peaksWS = getProperty("PeaksWorkspace");
  bool merge_peaks = getProperty("MergePeaks");

  std::string maskwsname = getPropertyValue("MaskWorkspace");
  std::vector<detid_t> vecMaskedDetID;
  if (maskwsname.size() > 0)
  {
    // process mask workspace
    m_maskDets = true;
    m_maskWS = getProperty("MaskWorkspace");
    vecMaskedDetID = processMaskWorkspace(m_maskWS);
  }
  else
  {
    m_maskDets = false;
  }

  // Process peaks
  std::map<int, signal_t> monitorCountMap = getMonitorCounts();
  getPeakInformation();
  simplePeakIntegration(vecMaskedDetID, monitorCountMap);
  if (merge_peaks)
    mergePeaks();


  DataObjects::PeaksWorkspace_sptr outws = createOutputs();
  setProperty("OutputWorkspace", outws);

} //

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
void IntegratePeaksCWSD::simplePeakIntegration(const std::vector<detid_t> &vecMaskedDetID,
                                               const std::map<int, signal_t> &run_monitor_map)
{
  // Check requirements
  assert(m_inputWS && "MDEventWorkspace is not defined.");
  assert(m_peaksWS && "PeaksWorkspace is not defined.");

  // Define data structures
  // FIXME :: can this be moved to outer scope of this method?
  // std::vector<Kernel::V3D> vec_event_qsample;
  // std::vector<float> vec_event_signal;
  // std::vector<detid_t> vec_event_det;

  // Go through to get value
  API::IMDIterator *mditer = m_inputWS->createIterator();
  size_t nextindex = 1;
  bool scancell = true;
  // size_t currindex = 0;

  // Assuming that MDEvents are grouped by run number, there is no need to
  // loop up the map for peak center and monitor counts each time
  int current_run_number = -1;
  signal_t current_monitor_counts = 0;
  Kernel::V3D current_peak_center;

  signal_t total_signal = 0;
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
      if (vecMaskedDetID.size() > 0)
      {
        detid_t detid = mditer->getInnerDetectorID(iev);
        std::vector<detid_t>::const_iterator it;

        it = find (vecMaskedDetID.begin(), vecMaskedDetID.end(), detid);
        if (it != vecMaskedDetID.end())
        {
          // The detector ID is found among masked detector IDs
          // Skip this event and move to next
          continue;
        }
      }

      // Check whether to update monitor counts and peak center
      uint16_t run_number = mditer->getInnerRunIndex(iev);
      int run_number_i = static_cast<int>(run_number);
      if (current_run_number != run_number_i)
      {
        current_run_number = run_number_i;
        std::map<int, signal_t>::const_iterator m_finder = run_monitor_map.find(current_run_number);
        if (m_finder != run_monitor_map.end())
          current_monitor_counts = m_finder->second;
        else
          throw std::runtime_error("Unable to find run number.");
        // current_monitor_counts = run_monitor_map[current_run_number];
        current_peak_center = m_runPeakCenterMap[current_run_number];
      }


      float tempx = mditer->getInnerPosition(iev, 0);
      float tempy = mditer->getInnerPosition(iev, 1);
      float tempz = mditer->getInnerPosition(iev, 2);

      Kernel::V3D pixel_pos(tempx, tempy, tempz);
      double distance = current_peak_center.distance(pixel_pos);

      total_signal += signal/current_monitor_counts;

      if (distance < min_distance)
        min_distance = distance;
      if (distance > max_distance)
        max_distance = distance;

      // g_log.notice() << "[DB] Event: run = " << run_number << ", signal = " << signal
      //  << ", distance = " << distance << "\n";

      // Check
      // if (currindex >= vec_event_qsample.size())
      //  throw std::runtime_error("Logic error in event size!");
      /*

      // FIXME/TODO/NOW - Continue from here!
      throw std::runtime_error("Need to find out how to deal with detid and signal here!");

      Kernel::V3D qsample(tempx, tempy, tempz);
      vec_event_qsample[currindex] = qsample;
      vec_event_signal[currindex] = signal;
      vec_event_det[currindex] = detid;
      */

      // ++currindex;
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
  g_log.notice() << "Total normalized signal = " << total_signal << " Distance range is "
                 << min_distance << ", " << max_distance << "\n";

  return;
}

/** Purpose: Process mask workspace
 *  Requirement: m_maskWS is not None
 *  Guarantees: an array will be set up for masked detectors
 * @brief IntegratePeaksCWSD::processMaskWorkspace
 * @param maskws
 */
std::vector<detid_t> IntegratePeaksCWSD::processMaskWorkspace(
    DataObjects::MaskWorkspace_const_sptr maskws)
{
  std::vector<detid_t> vecMaskedDetID;

  // Add the detector IDs of all masked detector to a vector
  size_t numspec = maskws->getNumberHistograms();
  for (size_t iws = 0; iws < numspec; ++iws)
  {
    Geometry::IDetector_const_sptr detector = maskws->getDetector(iws);
    if (detector->isMasked())
    {
      detid_t detid = detector->getID();
      vecMaskedDetID.push_back(detid);
    }
  }

  // Sort the vector for future lookup
  if (vecMaskedDetID.size() > 1)
    std::sort(vecMaskedDetID.begin(), vecMaskedDetID.end());

  return vecMaskedDetID;
}

void IntegratePeaksCWSD::mergePeaks()
{

}

DataObjects::PeaksWorkspace_sptr IntegratePeaksCWSD::createOutputs()
{
  DataObjects::PeaksWorkspace_sptr outws = m_peaksWS->clone();
  DataObjects::Peak &peak0 = outws->getPeak(0);
  peak0.setIntensity(100);

  return outws;
}

/**
 * @brief IntegratePeaksCWSD::getMonitorCounts
 * @return
 */
std::map<int, signal_t> IntegratePeaksCWSD::getMonitorCounts()
{
  std::map<int, signal_t> run_monitor_map;

  uint16_t num_expinfo = m_inputWS->getNumExperimentInfo();
  for (size_t iexpinfo = 0; iexpinfo < num_expinfo; ++iexpinfo)
  {
    ExperimentInfo_const_sptr expinfo = m_inputWS->getExperimentInfo(static_cast<uint16_t>(iexpinfo));
    std::string run_str = expinfo->run().getProperty("run_number")->value();
    uint16_t run_number = static_cast<uint16_t>(atoi(run_str.c_str()));
    std::string mon_str = expinfo->run().getProperty("monitor")->value();
    signal_t monitor = static_cast<signal_t>(atoi(mon_str.c_str()));
    run_monitor_map.insert(std::make_pair(static_cast<int>(run_number), monitor));
    g_log.notice() << "[DB] Add run " << run_number << ", monitor = " << monitor << "\n";
  }

  return run_monitor_map;
}


void IntegratePeaksCWSD::getPeakInformation()
{
  // TODO/NOW: If the number of peaks are more than 1!
  std::vector<Peak> peaks = m_peaksWS->getPeaks();
  size_t numpeaks = peaks.size();
  for (size_t ipeak = 0; ipeak < numpeaks; ++ipeak)
  {
    DataObjects::Peak &peak = peaks[ipeak];
    int run_number = peak.getRunNumber();
    Mantid::Kernel::V3D qsample = peak.getQSampleFrame();
    m_runPeakCenterMap.insert(std::make_pair(run_number, qsample));
    g_log.notice() <<"[DB] Q sample = " << qsample.toString() << "\n";
  }
}

} // namespace Mantid
} // namespace MDAlgorithms
