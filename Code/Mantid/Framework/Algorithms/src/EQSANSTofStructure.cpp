//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/EQSANSTofStructure.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidDataObjects/Events.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Instrument.h"

#include <vector>

using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(EQSANSTofStructure)

using namespace Kernel;
using namespace API;
using namespace Geometry;

void EQSANSTofStructure::init() {
  declareProperty(new WorkspaceProperty<EventWorkspace>(
                      "InputWorkspace", "", Direction::Input,
                      boost::make_shared<WorkspaceUnitValidator>("TOF")),
                  "Workspace to apply the TOF correction to");
  declareProperty("FlightPathCorrection", false,
                  "If True, the neutron flight path correction will be applied",
                  Kernel::Direction::Input);
  declareProperty("LowTOFCut", 0.0, "Width of the TOF margin to cut on the "
                                    "lower end of the TOF distribution of each "
                                    "frame",
                  Kernel::Direction::Input);
  declareProperty("HighTOFCut", 0.0, "Width of the TOF margin to cut on the "
                                     "upper end of the TOF distribution of "
                                     "each frame",
                  Kernel::Direction::Input);

  // Output parameters
  declareProperty("FrameSkipping", false,
                  "If True, the data was taken in frame skipping mode",
                  Kernel::Direction::Output);
  declareProperty("TofOffset", 0.0, "TOF offset that was applied to the data",
                  Kernel::Direction::Output);
  declareProperty(
      "WavelengthMin", 0.0,
      "Lower bound of the wavelength distribution of the first frame",
      Kernel::Direction::Output);
  declareProperty(
      "WavelengthMax", 0.0,
      "Upper bound of the wavelength distribution of the first frame",
      Kernel::Direction::Output);
  declareProperty(
      "WavelengthMinFrame2", 0.0,
      "Lower bound of the wavelength distribution of the second frame",
      Kernel::Direction::Output);
  declareProperty(
      "WavelengthMaxFrame2", 0.0,
      "Upper bound of the wavelength distribution of the second frame",
      Kernel::Direction::Output);
}

void EQSANSTofStructure::exec() {
  EventWorkspace_sptr inputWS = getProperty("InputWorkspace");
  flight_path_correction = getProperty("FlightPathCorrection");
  low_tof_cut = getProperty("LowTOFCut");
  high_tof_cut = getProperty("HighTOFCut");

  // Calculate the frame width
  double frequency = dynamic_cast<TimeSeriesProperty<double> *>(
                         inputWS->run().getLogData("frequency"))
                         ->getStatistics()
                         .mean;
  double tof_frame_width = 1.0e6 / frequency;

  // Determine whether we need frame skipping or not by checking the chopper
  // speed
  bool frame_skipping = false;
  const double chopper_speed = dynamic_cast<TimeSeriesProperty<double> *>(
                                   inputWS->run().getLogData("Speed1"))
                                   ->getStatistics()
                                   .mean;
  if (std::fabs(chopper_speed - frequency / 2.0) < 1.0)
    frame_skipping = true;

  // Get TOF offset
  frame_tof0 = getTofOffset(inputWS, frame_skipping);

  // Calculate the frame width
  double tmp_frame_width =
      frame_skipping ? tof_frame_width * 2.0 : tof_frame_width;
  double frame_offset = 0.0;
  if (frame_tof0 >= tmp_frame_width)
    frame_offset = tmp_frame_width * ((int)(frame_tof0 / tmp_frame_width));

  this->execEvent(inputWS, frame_tof0, frame_offset, tof_frame_width,
                  tmp_frame_width, frame_skipping);
}

void
EQSANSTofStructure::execEvent(Mantid::DataObjects::EventWorkspace_sptr inputWS,
                              double threshold, double frame_offset,
                              double tof_frame_width, double tmp_frame_width,
                              bool frame_skipping) {
  const size_t numHists = inputWS->getNumberHistograms();
  Progress progress(this, 0.0, 1.0, numHists);

  // Get the nominal sample-to-detector distance (in mm)
  Mantid::Kernel::Property *prop =
      inputWS->run().getProperty("sample_detector_distance");
  Mantid::Kernel::PropertyWithValue<double> *dp =
      dynamic_cast<Mantid::Kernel::PropertyWithValue<double> *>(prop);
  const double SDD = *dp / 1000.0;

  // Loop through the spectra and apply correction
  PARALLEL_FOR1(inputWS)
  for (int64_t ispec = 0; ispec < int64_t(numHists); ++ispec) {

    IDetector_const_sptr det;
    try {
      det = inputWS->getDetector(ispec);
    } catch (Exception::NotFoundError &) {
      g_log.warning() << "Spectrum index " << ispec
                      << " has no detector assigned to it - discarding"
                      << std::endl;
      // 'continue' statement moved outside catch block because Mac Intel
      // compiler has a problem with it being here in an openmp block.
    }
    if (!det)
      continue;

    // Get the flight path from the sample to the detector pixel
    const V3D samplePos = inputWS->getInstrument()->getSample()->getPos();
    const V3D scattered_flight_path = det->getPos() - samplePos;

    // Sample-to-source distance
    const V3D sourcePos = inputWS->getInstrument()->getSource()->getPos();
    const V3D SSD = samplePos - sourcePos;
    double tof_factor =
        (SSD.norm() + scattered_flight_path.norm()) / (SSD.norm() + SDD);

    PARALLEL_START_INTERUPT_REGION

    // Get the pointer to the output event list
    EventList *outEL = inputWS->getEventListPtr(ispec);
    std::vector<TofEvent> &events = outEL->getEvents();
    std::vector<TofEvent>::iterator it;
    std::vector<TofEvent> clean_events;

    for (it = events.begin(); it < events.end(); ++it) {
      double newtof = it->tof();
      newtof += frame_offset;
      // Correct for the scattered neutron flight path
      if (flight_path_correction)
        newtof /= tof_factor;

      while (newtof < threshold)
        newtof += tmp_frame_width;

      // Remove events that don't fall within the accepted time window
      double rel_tof = newtof - frame_tof0;
      double x = (static_cast<int>(floor(rel_tof * 10)) %
                  static_cast<int>(floor(tof_frame_width * 10))) *
                 0.1;
      if (x < low_tof_cut || x > tof_frame_width - high_tof_cut) {
        continue;
      }
      // At this point the events in the second frame are still off by a frame
      if (frame_skipping && rel_tof > tof_frame_width)
        newtof += tof_frame_width;
      clean_events.push_back(TofEvent(newtof, it->pulseTime()));
    }
    events.clear();
    for (it = clean_events.begin(); it < clean_events.end(); ++it) {
      events.push_back(*it);
    }

    progress.report("TOF structure");
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
}

double EQSANSTofStructure::getTofOffset(EventWorkspace_const_sptr inputWS,
                                        bool frame_skipping) {
  //# Storage for chopper information read from the logs
  double chopper_set_phase[4] = {0, 0, 0, 0};
  double chopper_speed[4] = {0, 0, 0, 0};
  double chopper_actual_phase[4] = {0, 0, 0, 0};
  double chopper_wl_1[4] = {0, 0, 0, 0};
  double chopper_wl_2[4] = {0, 0, 0, 0};
  double frame_wl_1 = 0;
  double frame_srcpulse_wl_1 = 0;
  double frame_wl_2 = 0;
  double chopper_srcpulse_wl_1[4] = {0, 0, 0, 0};
  double chopper_frameskip_wl_1[4] = {0, 0, 0, 0};
  double chopper_frameskip_wl_2[4] = {0, 0, 0, 0};
  double chopper_frameskip_srcpulse_wl_1[4] = {0, 0, 0, 0};

  // Calculate the frame width
  double frequency = dynamic_cast<TimeSeriesProperty<double> *>(
                         inputWS->run().getLogData("frequency"))
                         ->getStatistics()
                         .mean;
  double tof_frame_width = 1.0e6 / frequency;

  double tmp_frame_width = tof_frame_width;
  if (frame_skipping)
    tmp_frame_width *= 2.0;

  // Choice of parameter set
  int m_set = 0;
  if (frame_skipping)
    m_set = 1;

  bool first = true;
  bool first_skip = true;
  double frameskip_wl_1 = 0;
  double frameskip_srcpulse_wl_1 = 0;
  double frameskip_wl_2 = 0;

  for (int i = 0; i < 4; i++) {
    // Read chopper information
    std::ostringstream phase_str;
    phase_str << "Phase" << i + 1;
    chopper_set_phase[i] = dynamic_cast<TimeSeriesProperty<double> *>(
                               inputWS->run().getLogData(phase_str.str()))
                               ->getStatistics()
                               .mean;
    std::ostringstream speed_str;
    speed_str << "Speed" << i + 1;
    chopper_speed[i] = dynamic_cast<TimeSeriesProperty<double> *>(
                           inputWS->run().getLogData(speed_str.str()))
                           ->getStatistics()
                           .mean;

    // Only process choppers with non-zero speed
    if (chopper_speed[i] <= 0)
      continue;

    chopper_actual_phase[i] =
        chopper_set_phase[i] - CHOPPER_PHASE_OFFSET[m_set][i];

    while (chopper_actual_phase[i] < 0)
      chopper_actual_phase[i] += tmp_frame_width;

    double x1 =
        (chopper_actual_phase[i] -
         (tmp_frame_width * 0.5 * CHOPPER_ANGLE[i] / 360.)); // opening edge
    double x2 =
        (chopper_actual_phase[i] +
         (tmp_frame_width * 0.5 * CHOPPER_ANGLE[i] / 360.)); // closing edge
    if (!frame_skipping)                                     // not skipping
    {
      while (x1 < 0) {
        x1 += tmp_frame_width;
        x2 += tmp_frame_width;
      }
    }

    if (x1 > 0) {
      chopper_wl_1[i] = 3.9560346 * x1 / CHOPPER_LOCATION[i];
      chopper_srcpulse_wl_1[i] =
          3.9560346 * (x1 - chopper_wl_1[i] * PULSEWIDTH) / CHOPPER_LOCATION[i];
    } else
      chopper_wl_1[i] = chopper_srcpulse_wl_1[i] = 0.;

    if (x2 > 0)
      chopper_wl_2[i] = 3.9560346 * x2 / CHOPPER_LOCATION[i];
    else
      chopper_wl_2[i] = 0.;

    if (first) {
      frame_wl_1 = chopper_wl_1[i];
      frame_srcpulse_wl_1 = chopper_srcpulse_wl_1[i];
      frame_wl_2 = chopper_wl_2[i];
      first = false;
    } else {
      if (frame_skipping &&
          i == 2) // ignore chopper 1 and 2 forthe shortest wl.
      {
        frame_wl_1 = chopper_wl_1[i];
        frame_srcpulse_wl_1 = chopper_srcpulse_wl_1[i];
      }
      if (frame_wl_1 < chopper_wl_1[i])
        frame_wl_1 = chopper_wl_1[i];
      if (frame_wl_2 > chopper_wl_2[i])
        frame_wl_2 = chopper_wl_2[i];
      if (frame_srcpulse_wl_1 < chopper_srcpulse_wl_1[i])
        frame_srcpulse_wl_1 = chopper_srcpulse_wl_1[i];
    }

    if (frame_skipping) {
      if (x1 > 0) {
        x1 += tof_frame_width; // skipped pulse
        chopper_frameskip_wl_1[i] = 3.9560346 * x1 / CHOPPER_LOCATION[i];
        chopper_frameskip_srcpulse_wl_1[i] =
            3.9560346 * (x1 - chopper_wl_1[i] * PULSEWIDTH) /
            CHOPPER_LOCATION[i];
      } else
        chopper_wl_1[i] = chopper_srcpulse_wl_1[i] = 0.;

      if (x2 > 0) {
        x2 += tof_frame_width;
        chopper_frameskip_wl_2[i] = 3.9560346 * x2 / CHOPPER_LOCATION[i];
      } else
        chopper_wl_2[i] = 0.;

      if (i < 2 && chopper_frameskip_wl_1[i] > chopper_frameskip_wl_2[i])
        continue;

      if (first_skip) {
        frameskip_wl_1 = chopper_frameskip_wl_1[i];
        frameskip_srcpulse_wl_1 = chopper_frameskip_srcpulse_wl_1[i];
        frameskip_wl_2 = chopper_frameskip_wl_2[i];
        first_skip = false;
      } else {
        if (i == 2) // ignore chopper 1 and 2 forthe longest wl.
          frameskip_wl_2 = chopper_frameskip_wl_2[i];

        if (chopper_frameskip_wl_1[i] < chopper_frameskip_wl_2[i] &&
            frameskip_wl_1 < chopper_frameskip_wl_1[i])
          frameskip_wl_1 = chopper_frameskip_wl_1[i];

        if (chopper_frameskip_wl_1[i] < chopper_frameskip_wl_2[i] &&
            frameskip_srcpulse_wl_1 < chopper_frameskip_srcpulse_wl_1[i])
          frameskip_srcpulse_wl_1 = chopper_frameskip_srcpulse_wl_1[i];

        if (frameskip_wl_2 > chopper_frameskip_wl_2[i])
          frameskip_wl_2 = chopper_frameskip_wl_2[i];
      }
    }
  }

  if (frame_wl_1 >= frame_wl_2) // too many frames later. So figure it out
  {
    double n_frame[4] = {0, 0, 0, 0};
    double c_wl_1[4] = {0, 0, 0, 0};
    double c_wl_2[4] = {0, 0, 0, 0};
    bool passed = false;

    do {
      frame_wl_1 = c_wl_1[0] =
          chopper_wl_1[0] +
          3.9560346 * n_frame[0] * tof_frame_width / CHOPPER_LOCATION[0];
      frame_wl_2 = c_wl_2[0] =
          chopper_wl_2[0] +
          3.9560346 * n_frame[0] * tof_frame_width / CHOPPER_LOCATION[0];

      for (int i = 1; i < 4; i++) {
        n_frame[i] = n_frame[i - 1] - 1;
        passed = false;

        do {
          n_frame[i] += 1;
          c_wl_1[i] =
              chopper_wl_1[i] +
              3.9560346 * n_frame[i] * tof_frame_width / CHOPPER_LOCATION[i];
          c_wl_2[i] =
              chopper_wl_2[i] +
              3.9560346 * n_frame[i] * tof_frame_width / CHOPPER_LOCATION[i];

          if (frame_wl_1 < c_wl_2[i] && frame_wl_2 > c_wl_1[i]) {
            passed = true;
            break;
          }
          if (frame_wl_2 < c_wl_1[i])
            break; // over shot
        } while (n_frame[i] - n_frame[i - 1] < 10);

        if (!passed) {
          n_frame[0] += 1;
          break;
        } else {
          if (frame_wl_1 < c_wl_1[i])
            frame_wl_1 = c_wl_1[i];
          if (frame_wl_2 > c_wl_2[i])
            frame_wl_2 = c_wl_2[i];
        }
      }
    } while (!passed && n_frame[0] < 99);

    if (frame_wl_2 > frame_wl_1) {
      int n = 3;
      if (c_wl_1[2] > c_wl_1[3])
        n = 2;

      frame_srcpulse_wl_1 =
          c_wl_1[n] - 3.9560346 * c_wl_1[n] * PULSEWIDTH / CHOPPER_LOCATION[n];

      for (int i = 0; i < 4; i++) {
        chopper_wl_1[i] = c_wl_1[i];
        chopper_wl_2[i] = c_wl_2[i];
        if (frame_skipping) {
          chopper_frameskip_wl_1[i] =
              c_wl_1[i] +
              3.9560346 * 2. * tof_frame_width / CHOPPER_LOCATION[i];
          chopper_frameskip_wl_2[i] =
              c_wl_2[i] +
              3.9560346 * 2. * tof_frame_width / CHOPPER_LOCATION[i];
          if (i == 0) {
            frameskip_wl_1 = chopper_frameskip_wl_1[i];
            frameskip_wl_2 = chopper_frameskip_wl_2[i];
          } else {
            if (frameskip_wl_1 < chopper_frameskip_wl_1[i])
              frameskip_wl_1 = chopper_frameskip_wl_1[i];
            if (frameskip_wl_2 > chopper_frameskip_wl_2[i])
              frameskip_wl_2 = chopper_frameskip_wl_2[i];
          }
        }
      }
    } else
      frame_srcpulse_wl_1 = 0.0;
  }
  // Get source and detector locations
  // get the name of the mapping file as set in the parameter files
  std::vector<std::string> temp =
      inputWS->getInstrument()->getStringParameter("detector-name");
  std::string det_name = "detector1";
  if (temp.empty())
    g_log.information() << "The instrument parameter file does not contain the "
                           "'detector-name' parameter: trying 'detector1'";
  else
    det_name = temp[0];

  double source_z = inputWS->getInstrument()->getSource()->getPos().Z();
  double detector_z =
      inputWS->getInstrument()->getComponentByName(det_name)->getPos().Z();

  double source_to_detector = (detector_z - source_z) * 1000.0;
  frame_tof0 = frame_srcpulse_wl_1 / 3.9560346 * source_to_detector;

  g_log.information() << "Frame width " << tmp_frame_width << std::endl;
  g_log.information() << "TOF offset = " << frame_tof0 << " microseconds"
                      << std::endl;
  g_log.information() << "Band defined by T1-T4 " << frame_wl_1 << " "
                      << frame_wl_2;
  if (frame_skipping)
    g_log.information() << " + " << frameskip_wl_1 << " " << frameskip_wl_2
                        << std::endl;
  else
    g_log.information() << std::endl;
  g_log.information() << "Chopper    Actual Phase    Lambda1    Lambda2"
                      << std::endl;
  for (int i = 0; i < 4; i++)
    g_log.information() << i << "    " << chopper_actual_phase[i] << "  "
                        << chopper_wl_1[i] << "  " << chopper_wl_2[i]
                        << std::endl;

  double low_wl_discard = 3.9560346 * low_tof_cut / source_to_detector;
  double high_wl_discard = 3.9560346 * high_tof_cut / source_to_detector;

  setProperty("FrameSkipping", frame_skipping);
  setProperty("TofOffset", frame_tof0);
  setProperty("WavelengthMin", frame_wl_1 + low_wl_discard);
  setProperty("WavelengthMax", frame_wl_2 - high_wl_discard);
  if (frame_skipping) {
    setProperty("WavelengthMinFrame2", frameskip_wl_1 + low_wl_discard);
    setProperty("WavelengthMaxFrame2", frameskip_wl_2 - high_wl_discard);
  }

  return frame_tof0;
}

} // namespace Algorithms
} // namespace Mantid
