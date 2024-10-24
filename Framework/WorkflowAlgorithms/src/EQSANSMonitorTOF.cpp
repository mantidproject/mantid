// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidWorkflowAlgorithms/EQSANSMonitorTOF.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/TimeSeriesProperty.h"

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

namespace Mantid::WorkflowAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(EQSANSMonitorTOF)

using namespace Kernel;
using namespace API;
using namespace Geometry;

void EQSANSMonitorTOF::init() {
  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input,
                                                        std::make_shared<WorkspaceUnitValidator>("TOF")),
                  "Workspace to apply the TOF correction to");

  // Output parameters
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "Workspace to store the corrected data in");
  declareProperty("FrameSkipping", false, "True if the data was taken in frame-skipping mode",
                  Kernel::Direction::Output);
}

void EQSANSMonitorTOF::exec() {
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

  // Now create the output workspace
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  if (outputWS != inputWS) {
    outputWS = WorkspaceFactory::Instance().create(inputWS);
    setProperty("OutputWorkspace", outputWS);
  }

  // Get the nominal sample-to-detector distance (in mm)
  // const double MD = MONITORPOS/1000.0;

  // Get the monitor
  const std::vector<detid_t> monitor_list = inputWS->getInstrument()->getMonitors();
  if (monitor_list.size() != 1)
    g_log.error() << "EQSANS workspace does not have exactly ones monitor! "
                     "This should not happen\n";

  const auto &detInfo = inputWS->detectorInfo();
  const size_t monIndex0 = detInfo.indexOf(0);
  if (!detInfo.isMonitor(monIndex0)) {
    g_log.error() << "Spectrum number " << monIndex0 << " has no detector assigned to it - discarding\n";
    return;
  }

  // Get the source to monitor distance in mm
  double source_z = inputWS->getInstrument()->getSource()->getPos().Z();
  double monitor_z = detInfo.position(monIndex0).Z();
  double source_to_monitor = (monitor_z - source_z) * 1000.0;

  // Calculate the frame width
  auto log = inputWS->run().getTimeSeriesProperty<double>("frequency");
  double frequency = log->getStatistics().mean;
  double tof_frame_width = 1.0e6 / frequency;

  // Determine whether we need frame skipping or not by checking the chopper
  // speed
  bool frame_skipping = false;
  log = inputWS->run().getTimeSeriesProperty<double>("Speed1");
  const double chopper_speed = log->getStatistics().mean;
  if (std::fabs(chopper_speed - frequency / 2.0) < 1.0)
    frame_skipping = true;

  // Get TOF offset
  // this is the call to the chopper code to say where
  // the start of the data frame is relative to the native facility frame
  double frame_tof0 = getTofOffset(inputWS, frame_skipping, source_to_monitor);

  // Calculate the frame width
  // none of this changes in response to just looking at the monitor
  double tmp_frame_width = frame_skipping ? tof_frame_width * 2.0 : tof_frame_width;
  double frame_offset = 0.0;
  if (frame_tof0 >= tmp_frame_width)
    frame_offset = tmp_frame_width * (static_cast<int>(frame_tof0 / tmp_frame_width));

  // Find the new binning first
  const MantidVec XIn = inputWS->readX(0); // Copy here to avoid holding on to
                                           // reference for too long (problem
                                           // with managed workspaces)

  // Since we are swapping the low-TOF and high-TOF regions around the cutoff
  // value,
  // there is the potential for having an overlap between the two regions. We
  // exclude
  // the region beyond a single frame by considering only the first 1/60 sec of
  // the
  // TOF histogram. (Bins 1 to 1666, as opposed to 1 to 2000)
  const auto nTOF = static_cast<int>(XIn.size());

  // Loop through each bin to find the cutoff where the TOF distribution wraps
  // around
  int cutoff = 0;
  double threshold = frame_tof0 - frame_offset;
  int tof_bin_range = 0;
  double frame = 1000000.0 / frequency;
  for (int i = 0; i < nTOF; i++) {
    if (XIn[i] < threshold)
      cutoff = i;
    if (XIn[i] < frame)
      tof_bin_range = i;
  }
  g_log.information() << "Cutoff=" << cutoff << "; Threshold=" << threshold << '\n';
  g_log.information() << "Low TOFs: old = [" << (cutoff + 1) << ", " << (tof_bin_range - 2) << "]  ->  new = [0, "
                      << (tof_bin_range - 3 - cutoff) << "]\n";
  g_log.information() << "High bin boundary of the Low TOFs: old = " << tof_bin_range - 1
                      << "; new = " << (tof_bin_range - 2 - cutoff) << '\n';
  g_log.information() << "High TOFs: old = [0, " << (cutoff - 1) << "]  ->  new = [" << (tof_bin_range - 1 - cutoff)
                      << ", " << (tof_bin_range - 2) << "]\n";
  g_log.information() << "Overlap: new = [" << (tof_bin_range - 1) << ", " << (nTOF - 2) << "]\n";

  // Keep a copy of the input data since we may end up overwriting it
  // if the input workspace is equal to the output workspace.
  // This is necessary since we are shuffling around the TOF bins.
  const MantidVec YIn = MantidVec(inputWS->readY(0));
  const MantidVec EIn = MantidVec(inputWS->readE(0));

  MantidVec &XOut = outputWS->dataX(0);
  MantidVec &YOut = outputWS->dataY(0);
  MantidVec &EOut = outputWS->dataE(0);

  // Here we modify the TOF according to the offset we calculated.
  // Since this correction will change the order of the TOF bins,
  // we do it in sequence so that we obtain a valid distribution
  // as our result (with increasing TOF values).

  // Move up the low TOFs
  for (int i = 0; i < cutoff; i++) {
    XOut[i + tof_bin_range - 1 - cutoff] = XIn[i] + frame_offset + tmp_frame_width;
    YOut[i + tof_bin_range - 1 - cutoff] = YIn[i];
    EOut[i + tof_bin_range - 1 - cutoff] = EIn[i];
  }

  // Get rid of extra bins
  for (int i = tof_bin_range - 1; i < nTOF - 1; i++) {
    XOut[i] = XOut[i - 1] + 10.0;
    YOut[i] = 0.0;
    EOut[i] = 0.0;
  }
  XOut[nTOF - 1] = XOut[nTOF - 2] + 10.0;

  // Move down the high TOFs
  for (int i = cutoff + 1; i < tof_bin_range - 1; i++) {
    XOut[i - cutoff - 1] = XIn[i] + frame_offset;
    YOut[i - cutoff - 1] = YIn[i];
    EOut[i - cutoff - 1] = EIn[i];
  }
  // Don't forget the low boundary
  XOut[tof_bin_range - 2 - cutoff] = XIn[tof_bin_range] + frame_offset;

  // Zero out the cutoff bin, which no longer makes sense because
  // len(x) = len(y)+1
  YOut[tof_bin_range - 2 - cutoff] = 0.0;
  EOut[tof_bin_range - 2 - cutoff] = 0.0;

  setProperty("OutputWorkspace", outputWS);
}

double EQSANSMonitorTOF::getTofOffset(const MatrixWorkspace_const_sptr &inputWS, bool frame_skipping,
                                      double source_to_monitor) {
  // # Storage for chopper information read from the logs
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
  auto log = inputWS->run().getTimeSeriesProperty<double>("frequency");
  double frequency = log->getStatistics().mean;
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
    log = inputWS->run().getTimeSeriesProperty<double>(phase_str.str());
    chopper_set_phase[i] = log->getStatistics().mean;
    std::ostringstream speed_str;
    speed_str << "Speed" << i + 1;
    log = inputWS->run().getTimeSeriesProperty<double>(speed_str.str());
    chopper_speed[i] = log->getStatistics().mean;

    // Only process choppers with non-zero speed
    if (chopper_speed[i] <= 0)
      continue;

    chopper_actual_phase[i] = chopper_set_phase[i] - CHOPPER_PHASE_OFFSET[m_set][i];

    while (chopper_actual_phase[i] < 0)
      chopper_actual_phase[i] += tmp_frame_width;

    double x1 = (chopper_actual_phase[i] - (tmp_frame_width * 0.5 * CHOPPER_ANGLE[i] / 360.)); // opening edge
    double x2 = (chopper_actual_phase[i] + (tmp_frame_width * 0.5 * CHOPPER_ANGLE[i] / 360.)); // closing edge
    if (!frame_skipping)                                                                       // not skipping
    {
      while (x1 < 0) {
        x1 += tmp_frame_width;
        x2 += tmp_frame_width;
      }
    }

    if (x1 > 0) {
      chopper_wl_1[i] = 3.9560346 * x1 / CHOPPER_LOCATION[i];
      chopper_srcpulse_wl_1[i] = 3.9560346 * (x1 - chopper_wl_1[i] * PULSEWIDTH) / CHOPPER_LOCATION[i];
    } else
      chopper_wl_1[i] = chopper_srcpulse_wl_1[i] = 0.;

    chopper_wl_2[i] = (x2 > 0) ? 3.9560346 * x2 / CHOPPER_LOCATION[i] : 0.;

    if (first) {
      frame_wl_1 = chopper_wl_1[i];
      frame_srcpulse_wl_1 = chopper_srcpulse_wl_1[i];
      frame_wl_2 = chopper_wl_2[i];
      first = false;
    } else {
      if (frame_skipping && i == 2) // ignore chopper 1 and 2 forthe shortest wl.
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
        chopper_frameskip_srcpulse_wl_1[i] = 3.9560346 * (x1 - chopper_wl_1[i] * PULSEWIDTH) / CHOPPER_LOCATION[i];
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

        if (chopper_frameskip_wl_1[i] < chopper_frameskip_wl_2[i] && frameskip_wl_1 < chopper_frameskip_wl_1[i])
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
      frame_wl_1 = c_wl_1[0] = chopper_wl_1[0] + 3.9560346 * n_frame[0] * tof_frame_width / CHOPPER_LOCATION[0];
      frame_wl_2 = c_wl_2[0] = chopper_wl_2[0] + 3.9560346 * n_frame[0] * tof_frame_width / CHOPPER_LOCATION[0];

      for (int i = 1; i < 4; i++) {
        n_frame[i] = n_frame[i - 1] - 1;
        passed = false;

        do {
          n_frame[i] += 1;
          c_wl_1[i] = chopper_wl_1[i] + 3.9560346 * n_frame[i] * tof_frame_width / CHOPPER_LOCATION[i];
          c_wl_2[i] = chopper_wl_2[i] + 3.9560346 * n_frame[i] * tof_frame_width / CHOPPER_LOCATION[i];

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

      frame_srcpulse_wl_1 = c_wl_1[n] - 3.9560346 * c_wl_1[n] * PULSEWIDTH / CHOPPER_LOCATION[n];

      for (int i = 0; i < 4; i++) {
        chopper_wl_1[i] = c_wl_1[i];
        chopper_wl_2[i] = c_wl_2[i];
        if (frame_skipping) {
          chopper_frameskip_wl_1[i] = c_wl_1[i] + 3.9560346 * 2. * tof_frame_width / CHOPPER_LOCATION[i];
          chopper_frameskip_wl_2[i] = c_wl_2[i] + 3.9560346 * 2. * tof_frame_width / CHOPPER_LOCATION[i];
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

  double frame_tof0 = frame_srcpulse_wl_1 / 3.9560346 * source_to_monitor;

  g_log.information() << "Frame width " << tmp_frame_width << '\n';
  g_log.information() << "TOF offset = " << frame_tof0 << " microseconds\n";
  g_log.information() << "Band defined by T1-T4 " << frame_wl_1 << " " << frame_wl_2;
  if (frame_skipping)
    g_log.information() << " + " << frameskip_wl_1 << " " << frameskip_wl_2 << '\n';
  else
    g_log.information() << '\n';
  g_log.information() << "Chopper    Actual Phase    Lambda1    Lambda2\n";
  for (int i = 0; i < 4; i++)
    g_log.information() << i << "    " << chopper_actual_phase[i] << "  " << chopper_wl_1[i] << "  " << chopper_wl_2[i]
                        << '\n';

  setProperty("FrameSkipping", frame_skipping);

  return frame_tof0;
}

} // namespace Mantid::WorkflowAlgorithms
