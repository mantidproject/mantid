// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/IntegratePeaksCWSD.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayProperty.h"

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(IntegratePeaksCWSD)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

const signal_t THRESHOLD_SIGNAL = 0;

/** Constructor
 */
IntegratePeaksCWSD::IntegratePeaksCWSD()
    : m_haveMultipleRun(false), m_useSinglePeakCenterFmUser(false),
      m_peakRadius(), m_doMergePeak(false), m_normalizeByMonitor(false),
      m_normalizeByTime(false), m_scaleFactor(0), m_haveInputPeakWS(false) {}

/** Initialize the algorithm's properties.
 */
void IntegratePeaksCWSD::init() {
  declareProperty(std::make_unique<WorkspaceProperty<IMDEventWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "An input MDEventWorkspace.");

  declareProperty(
      std::make_unique<WorkspaceProperty<PeaksWorkspace>>(
          "PeaksWorkspace", "", Direction::Input, API::PropertyMode::Optional),
      "A PeaksWorkspace containing the peaks to integrate.");

  declareProperty(
      std::make_unique<WorkspaceProperty<PeaksWorkspace>>("OutputWorkspace", "",
                                                          Direction::Output),
      "The output PeaksWorkspace will be a copy of the input PeaksWorkspace "
      "with the peaks' integrated intensities.");

  declareProperty(
      std::make_unique<WorkspaceProperty<DataObjects::MaskWorkspace>>(
          "MaskWorkspace", "", Direction::Input, PropertyMode::Optional),
      "Output Masking Workspace");

  declareProperty(
      std::make_unique<ArrayProperty<double>>("PeakCentre"),
      "A comma separated list for peak centre in Q-sample frame. "
      "Its length is either 3 (Qx, Qy, Qz) or 0. "
      "If peak center is defined, then all the data among all the runs will be "
      "integrated in respect to this peak center. Otherwise, the peaks that "
      "will "
      "be integrated shall be found in the given peak workspace.");

  declareProperty("PeakRadius", EMPTY_DBL(), "Radius of a peak.");

  declareProperty(
      "MergePeaks", true,
      "In case that there are more than 1 run number in the given "
      "PeaksWorkspace "
      "and MDEVentWorkspace, if it is set to true, then the peaks' intensities "
      "will be merged.");

  declareProperty(
      "NormalizeByMonitor", false,
      "If selected, then all the signals will be normalized by monitor counts."
      "Otherwise, the output peak intensity will be just simple addition of "
      "peak intensity."
      "It is only applied to the situation that Mergepeaks is not selected.");

  declareProperty(
      "NormalizeByTime", true,
      "It selected, then all the signals will be normalized by time "
      "in the case that the counting time is very short and thus the beam "
      "monitor "
      "is not accurate.");

  declareProperty(
      "ScaleFactor", 1000.,
      "If NormalizeByMonitor or NormalizeByTime is selected, the intensity "
      "will be scaled by this factor.");
}

//----------------------------------------------------------------------------------------------
/**
 * @brief IntegratePeaksCWSD::exec
 */
void IntegratePeaksCWSD::exec() {
  // Process input & check
  processInputs();

  // Integrate peak with simple algorithm
  simplePeakIntegration(vecMaskedDetID, m_runNormMap);

  // Merge peak if necessary
  if (m_doMergePeak)
    mergePeaks();
  else
    normalizePeaksIntensities(); // normalize the intensity of each Pt.

  // Output
  DataObjects::PeaksWorkspace_sptr outws =
      (m_useSinglePeakCenterFmUser)
          ? createPeakworkspace(m_peakCenter, m_inputWS)
          : createOutputs();

  setProperty("OutputWorkspace", outws);
}

//----------------------------------------------------------------------------------------------
/** Process and check input properties
 */
void IntegratePeaksCWSD::processInputs() {
  // Required input workspaces
  m_inputWS = getProperty("InputWorkspace");

  // Input peaks
  std::vector<double> peak_center = getProperty("PeakCentre");
  if (!peak_center.empty()) {
    // assigned peak center
    if (peak_center.size() != 3)
      throw std::invalid_argument("PeakCentre must have 3 elements.");
    m_peakCenter.setX(peak_center[0]);
    m_peakCenter.setY(peak_center[1]);
    m_peakCenter.setZ(peak_center[2]);
    // no use input peak workspace
    m_haveInputPeakWS = false;
    m_useSinglePeakCenterFmUser = true;
  } else {
    // use input peak workspace
    std::string peakWSName = getPropertyValue("PeaksWorkspace");
    if (peakWSName.length() == 0)
      throw std::invalid_argument("It is not allowed that neither peak center "
                                  "nor PeaksWorkspace is specified.");
    m_peaksWS = getProperty("PeaksWorkspace");
    m_haveInputPeakWS = true;
    m_useSinglePeakCenterFmUser = false;
  }
  m_doMergePeak = getProperty("MergePeaks");
  if (m_haveInputPeakWS && m_peaksWS->getNumberPeaks() > 1 && m_doMergePeak)
    throw std::invalid_argument(
        "It is not allowed to merge peaks when there are "
        "multiple peaks present in PeaksWorkspace.");

  m_normalizeByMonitor = getProperty("NormalizeByMonitor");
  m_normalizeByTime = getProperty("NormalizeByTime");
  if (m_normalizeByMonitor && m_normalizeByTime)
    throw std::invalid_argument(
        "It is not allowed to select to be normalized both  "
        "by time and by monitor counts.");
  if (m_doMergePeak && !(m_normalizeByMonitor || m_normalizeByTime))
    throw std::invalid_argument(
        "Either being normalized by time or being normalized "
        "by monitor must be selected if merge-peak is selected.");
  m_scaleFactor = getProperty("ScaleFactor");
  g_log.warning() << "[DB...BAT] Scale factor = " << m_scaleFactor << "\n";

  // monitor counts
  if (m_normalizeByMonitor)
    m_runNormMap = getMonitorCounts();
  else if (m_normalizeByTime)
    m_runNormMap = getMeasureTime();

  // go through peak
  if (m_haveInputPeakWS)
    getPeakInformation();
  m_haveMultipleRun = (m_runPeakCenterMap.size() > 1);

  // peak related
  m_peakRadius = getProperty("PeakRadius");
  if (m_peakRadius == EMPTY_DBL())
    throw std::invalid_argument("Peak radius cannot be left empty.");

  // optional mask workspace
  std::string maskwsname = getPropertyValue("MaskWorkspace");
  if (!maskwsname.empty()) {
    // process mask workspace
    m_maskWS = getProperty("MaskWorkspace");
    vecMaskedDetID = processMaskWorkspace(m_maskWS);
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
  if (!m_inputWS)
    throw std::runtime_error("MDEventWorkspace is not defined.");

  // Go through to get value
  auto mditer = m_inputWS->createIterator();
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

      uint16_t run_number = mditer->getInnerRunIndex(iev);
      int run_number_i = static_cast<int>(run_number);

      /* debug: record raw signals
      if (run_number_i % 1000 == testrunnumber)
      {
        total_signal += signal;
        ++ num_det;
      }
      // ... debug */

      // Check whether this detector is masked
      if (!vecMaskedDetID.empty()) {
        detid_t detid = mditer->getInnerDetectorID(iev);
        std::vector<detid_t>::const_iterator it;

        it = find(vecMaskedDetID.begin(), vecMaskedDetID.end(), detid);
        if (it != vecMaskedDetID.end()) {
          // The detector ID is found among masked detector IDs
          // Skip this event and move to next

          /* debug: record masked detectors
          if (run_number_i % 1000 == testrunnumber)
          {
            num_masked_det += 1;
            g_log.warning() << "Masked detector ID = " << detid << ", Signal = "
          << signal << "\n";
          }
          // ... debug */

          continue;
        }
      }

      /* debug: record unmasked detectors
      if (run_number_i % 1000 == testrunnumber)
        num_unmasked_det += 1;
      // ... debug */

      // Check whether to update monitor counts and peak center
      if (current_run_number != run_number_i) {
        // update run number
        current_run_number = run_number_i;
        // update monitor counts
        if (m_normalizeByMonitor) {
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
        } else {
          current_monitor_counts = 1.;
        }

        // update peak center
        if (!m_useSinglePeakCenterFmUser)
          current_peak_center = m_runPeakCenterMap[current_run_number];
      }

      // calculate distance
      float tempx = mditer->getInnerPosition(iev, 0);
      float tempy = mditer->getInnerPosition(iev, 1);
      float tempz = mditer->getInnerPosition(iev, 2);
      Kernel::V3D pixel_pos(tempx, tempy, tempz);
      double distance = current_peak_center.distance(pixel_pos);

      /* debug: record unmasked signal
      if (run_number_i % 1000 == testrunnumber)
      {
        total_unmasked_signal += signal;
      }
      // ... debug */

      if (distance < m_peakRadius) {
        // FIXME - Is it very costly to use map each time???
        // total_signal += signal/current_monitor_counts;
        m_runPeakCountsMap[run_number] += signal / current_monitor_counts;
      } else {
        g_log.debug() << "Out of radius " << distance << " > " << m_peakRadius
                      << ": Center = " << current_peak_center.toString()
                      << ", Pixel  = " << pixel_pos.toString() << "\n";
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

  /*
  g_log.warning() << "Debug output: run 13: Number masked detectors = " <<
  num_masked_det
                  << ", Total signal = " << total_signal << "\n";
  g_log.warning() << "  Number of unmasked detectors = " << num_unmasked_det
                  << ", Total unmasked signal = " << total_unmasked_signal <<
  "\n";
  g_log.warning() << "  Number of total detectors = " << num_det << "\n";
  */
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
  const auto &specInfo = maskws->spectrumInfo();
  for (size_t iws = 0; iws < numspec; ++iws) {
    const auto &vecY = maskws->y(iws);
    if (vecY[0] > 0.1) {
      // vecY[] > 0 is masked.  det->isMasked() may not be reliable.
      const detid_t detid = specInfo.detector(iws).getID();
      vecMaskedDetID.push_back(detid);
    }
  }

  // Sort the vector for future lookup
  if (vecMaskedDetID.size() > 1)
    std::sort(vecMaskedDetID.begin(), vecMaskedDetID.end());

  g_log.warning() << "[DB] There are " << vecMaskedDetID.size()
                  << " detectors masked."
                  << "\n";

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
  for (mon_iter = m_runNormMap.begin(); mon_iter != m_runNormMap.end();
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
  DataObjects::PeaksWorkspace_sptr outws =
      boost::shared_ptr<DataObjects::PeaksWorkspace>(
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
 * @brief IntegratePeaksCWSD::createPeakworkspace
 * @param peakCenter
 * @param mdws :: source MDEventWorkspace where the run numbers come from
 * @return
 */
DataObjects::PeaksWorkspace_sptr
IntegratePeaksCWSD::createPeakworkspace(Kernel::V3D peakCenter,
                                        API::IMDEventWorkspace_sptr mdws) {
  g_log.notice("Create peak workspace for output ... ...");
  // new peak workspace
  DataObjects::PeaksWorkspace_sptr peakws =
      boost::make_shared<DataObjects::PeaksWorkspace>();

  // get number of runs
  size_t numruns = mdws->getNumExperimentInfo();
  for (size_t i_run = 0; i_run < numruns; ++i_run) {
    // get experiment info for run number, instrument and peak count
    API::ExperimentInfo_const_sptr expinfo =
        mdws->getExperimentInfo(static_cast<uint16_t>(i_run));
    int runnumber = expinfo->getRunNumber();
    // FIXME - This is a hack for HB3A's run number issue
    std::map<int, double>::iterator miter =
        m_runPeakCountsMap.find(runnumber % 1000);
    double peakcount(0);
    if (miter != m_runPeakCountsMap.end()) {
      peakcount = miter->second;
      g_log.notice() << "[DB] Get peak count of run " << runnumber << " as "
                     << peakcount << "\n";
    } else {
      g_log.notice() << "[DB] Unable to find run " << runnumber
                     << " in peak count map."
                     << "\n";
    }

    // Create and add a new peak to peak workspace
    DataObjects::Peak newpeak;
    try {
      Geometry::Instrument_const_sptr instrument = expinfo->getInstrument();
      newpeak.setInstrument(instrument);
      newpeak.setGoniometerMatrix(expinfo->run().getGoniometerMatrix());
    } catch (const std::exception &) {
      throw std::runtime_error(
          "Unable to set instrument and goniometer matrix.");
    }

    newpeak.setQSampleFrame(peakCenter);
    newpeak.setRunNumber(runnumber);
    newpeak.setIntensity(peakcount * m_scaleFactor);

    peakws->addPeak(newpeak);
  }

  g_log.notice("Peak workspace is generated.... ");
  return peakws;
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
    g_log.information() << "run number of exp " << iexpinfo << " is " << run_str
                        << "\n";
    int run_number = std::stoi(run_str);
    // FIXME - HACK FOE HB3A
    run_number = run_number % 1000;
    std::string mon_str = expinfo->run().getProperty("monitor")->value();
    signal_t monitor = static_cast<signal_t>(std::stod(mon_str));
    run_monitor_map.insert(std::make_pair(run_number, monitor));
    g_log.information() << "From MD workspace add run " << run_number
                        << ", monitor = " << monitor << "\n";
  }

  return run_monitor_map;
}

//----------------------------------------------------------------------------------------------
/** Get the measuring time for each run in case that it is to be normalized by
 * time
 * @brief IntegratePeaksCWSD::getMeasureTime
 * @return
 */
std::map<int, double> IntegratePeaksCWSD::getMeasureTime() {
  std::map<int, double> run_time_map;

  uint16_t num_expinfo = m_inputWS->getNumExperimentInfo();
  for (size_t iexpinfo = 0; iexpinfo < num_expinfo; ++iexpinfo) {
    ExperimentInfo_const_sptr expinfo =
        m_inputWS->getExperimentInfo(static_cast<uint16_t>(iexpinfo));
    std::string run_str = expinfo->run().getProperty("run_number")->value();
    int run_number = std::stoi(run_str);

    // FIXME - HACK FOE HB3A
    run_number = run_number % 1000;
    std::string duration_str = expinfo->run().getProperty("duration")->value();
    double duration = std::stod(duration_str);
    run_time_map.insert(std::make_pair(run_number, duration));
    g_log.warning() << "MD workspace exp info " << iexpinfo << ": run "
                    << run_number << ", measuring time = " << duration << "\n";
  }

  return run_time_map;
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

    g_log.information() << "From peak workspace: peak " << ipeak
                        << " Center (Qsample) = " << qsample.toString() << "\n";
  }
}

//----------------------------------------------------------------------------------------------
/** Normalize the peak's intensities per Pt. to either time or monitor counts
 * @brief IntegratePeaksCSWD::normalizePeaksIntensities
 */
void IntegratePeaksCWSD::normalizePeaksIntensities() {
  // go over each peak (of run)
  std::map<int, double>::iterator count_iter;
  for (count_iter = m_runPeakCountsMap.begin();
       count_iter != m_runPeakCountsMap.end(); ++count_iter) {
    int run_number_i = count_iter->first;
    // get monitor value
    std::map<int, signal_t>::iterator mon_iter =
        m_runNormMap.find(run_number_i);
    // normalize peak intensities stored in m_runNormMap
    if (mon_iter != m_runNormMap.end()) {
      signal_t monitor_i = mon_iter->second;
      count_iter->second /= monitor_i;
    }
  } // END-FOR

  return;
}

} // namespace MDAlgorithms
} // namespace Mantid
