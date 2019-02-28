// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/EQSANSCorrectFrame.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Events.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/EmptyValues.h

#include <vector>

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(EQSANSCorrectFrame)

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using Types::Event::TofEvent;

void EQSANSCorrectFrame::init() {
  declareProperty(make_unique<WorkspaceProperty<EventWorkspace>>(
                      "InputWorkspace", "", Direction::Input,
                      boost::make_shared<WorkspaceUnitValidator>("TOF")),
                  "Workspace to apply the TOF correction to");
  declareProperty("PulsePeriod", 1.0E6 / 60.0,
                  "Period of the neutron pulse, in micro-seconds",
                  Direction::Input);
  declareProperty("MinTOF", EMPTY_DBL(),
                  "Time of flight of fastest neutrons from the lead pulse, in "
                  "micro-seconds",
                  Direction::Input);
  declareProperty("FrameWidth", 1.0E6 / 60.0,
                  "Period of the chopper system, in micro-seconds",
                  Direction::Input);
  declareProperty("FrameSkipping", false,
                  "If True, the data was taken in frame skipping mode",
                  Direction::Input);
}

void EQSANSCorrectFrame::exec() {
  processAlgProperties();
  EventWorkspace_sptr inputWS = getProperty("InputWorkspace");
  const size_t numHists = inputWS->getNumberHistograms();
  Progress progress(this, 0.0, 1.0, numHists);

  const double pulsePeriod = getProperty("PulsePeriod");
  const double minTOF = getProperty("MinTOF");
  const double minTOF_delayed = minTOF + pulsePeriod;
  const double frameWidth = getProperty("FrameWidth");
  const size_t nFrames = std::static_cast<size_t>(minTOF / frame_width);
  const double frames_offset_time =
      frame_width * std::static_cast<double>(nFrames);
  const bool isFrameSkipping = getProperty("FrameSkipping");
  const auto &spectrumInfo = inputWS->spectrumInfo();

  // Loop through the spectra and apply correction
  PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS))
  for (int64_t ispec = 0; ispec < int64_t(numHists); ++ispec) {
    PARALLEL_START_INTERUPT_REGION
    if (!spectrumInfo.hasDetectors(ispec)) {
      g_log.warning() << "Workspace index " << ispec
                      << " has no detector assigned to it - discarding\n";
      continue;
    }
    std::vector<TofEvent> &events = inputWS->getSpectrum(ispec).getEvents();
    if (events.empty())
      continue; // no events recorded in this spectrum
    std::vector<TofEvent> clean_events;
    double newTOF;
    for (auto event : events) {
      // shift times to the correct frame
      newTOF = event.tof() + frames_offset_time;
      // TOF values smaller than that of the fastest neutrons have been
      // 'folded' by the data acquisition system. They must be shifted
      if (newTOF < minTOF)
        newTOF += frameWidth;
      // Events from the skipped pulse are delayed by one pulse period
      if (isFrameSkipping && newTOF > minTOF_delayed)
        newTOF += pulsePeriod;
      clean_events.emplace_back(newtof, event.pulseTime());
    }
    events.clear();
    events.reserve(clean_events.size());
    std::copy(clean_events.begin(), clean_events.end(),
              std::back_inserter(events));
    progress.report("Correct TOF frame");
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
}

} // namespace Algorithms
} // namespace Mantid
