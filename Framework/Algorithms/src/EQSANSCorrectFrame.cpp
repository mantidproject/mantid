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
#include "MantidKernel/EmptyValues.h"

#include <vector>

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(EQSANSCorrectFrame)

using namespace Kernel;
using namespace API;
using namespace DataObjects;
using namespace Geometry;
using Types::Event::TofEvent;

EQSANSCorrectFrame::EQSANSCorrectFrame() : API::Algorithm() {}

void EQSANSCorrectFrame::init() {
  declareProperty(std::make_unique<WorkspaceProperty<EventWorkspace>>(
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
  EventWorkspace_sptr inputWS = getProperty("InputWorkspace");
  const size_t numHists = inputWS->getNumberHistograms();
  Progress progress(this, 0.0, 1.0, numHists);

  const double pulsePeriod = getProperty("PulsePeriod");
  const double minTOF = getProperty("MinTOF");
  const double frameWidth = getProperty("FrameWidth");
  const bool isFrameSkipping = getProperty("FrameSkipping");
  const auto &spectrumInfo = inputWS->spectrumInfo();

  // Creates a function that correct TOF values
  struct correctTofFactory {

    explicit correctTofFactory(double pulsePeriod, double minTOF,
                               double frameWidth, bool isFrameSkipping)
        : m_pulsePeriod(pulsePeriod), m_minTOF(minTOF),
          m_frameWidth(frameWidth), m_isFrameSkipping(isFrameSkipping) {
      // Find how many frame widths elapsed from the time the neutrons of the
      // lead pulse were emitted and the time the neutrons arrived to the
      // detector bank. This time must be added to the stored TOF values
      const size_t nf = static_cast<size_t>(minTOF / frameWidth);
      m_framesOffsetTime = frameWidth * static_cast<double>(nf);
      m_minTOF_delayed = minTOF + pulsePeriod;
    }

    double operator()(const double tof) const {
      // shift times to the correct frame
      double newTOF = tof + m_framesOffsetTime;
      // TOF values smaller than that of the fastest neutrons have been
      // 'folded' by the data acquisition system. They must be shifted
      if (newTOF < m_minTOF)
        newTOF += m_frameWidth;
      // Events from the skipped pulse are delayed by one pulse period
      if (m_isFrameSkipping && newTOF > m_minTOF_delayed)
        newTOF += m_pulsePeriod;
      return newTOF;
    }

    double m_pulsePeriod;
    double m_minTOF;
    double m_frameWidth;
    bool m_isFrameSkipping;
    double m_framesOffsetTime;
    double m_minTOF_delayed;
  };

  auto correctTof =
      correctTofFactory(pulsePeriod, minTOF, frameWidth, isFrameSkipping);

  // Loop through the spectra and apply correction
  PARALLEL_FOR_IF(Kernel::threadSafe(*inputWS))
  for (int64_t ispec = 0; ispec < int64_t(numHists); ++ispec) {
    PARALLEL_START_INTERUPT_REGION
    if (!spectrumInfo.hasDetectors(ispec)) {
      g_log.warning() << "Workspace index " << ispec
                      << " has no detector assigned to it - discarding\n";
      continue;
    }
    EventList &evlist = inputWS->getSpectrum(ispec);
    if (evlist.getNumberEvents() == 0)
      continue; // no events recorded in this spectrum
    evlist.convertTof(correctTof);
    progress.report("Correct TOF frame");
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  // Set bin boundaries to absolute minimum and maximum
  inputWS->resetAllXToSingleBin();
}

} // namespace Algorithms
} // namespace Mantid
