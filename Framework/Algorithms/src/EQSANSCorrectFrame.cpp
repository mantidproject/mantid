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

#include <functional>
#include <vector>
#include <string>

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
  declareProperty("PathToPixel", true,
                  "If True, use path from moderator to individual pixel instead"
                  "of to center of the detector panel",
                  Direction::Input);
  declareProperty("DetectorName", "detector1", "Name of the double panel");
}

void EQSANSCorrectFrame::exec() {
  EventWorkspace_sptr inputWS = getProperty("InputWorkspace");
  const size_t numHists = inputWS->getNumberHistograms();
  Progress progress(this, 0.0, 1.0, numHists);

  const double pulsePeriod = getProperty("PulsePeriod");
  const double minTOF = getProperty("MinTOF");
  const double frameWidth = getProperty("FrameWidth");
  const bool isFrameSkipping = getProperty("FrameSkipping");
  const bool pathToPixel = getProperty("PathToPixel");
  const std::string detectorName = getProperty("DetectorName");

  const auto &spectrumInfo = inputWS->spectrumInfo();
  auto ins = inputWS->getInstrument();
  auto sam = ins->getSample();
  auto mod = ins->getSource();
  const auto msd = mod->getDistance(*sam);
  const auto det = ins->getComponentByName(detectorName);
  const auto mdd = mod->getDistance(*det);
  // Creates a function that correct TOF values
  struct correctTofFactory {

    explicit correctTofFactory(double pulsePeriod, double minTOF,
                               double frameWidth, bool isFrameSkipping)
        : m_pulsePeriod(pulsePeriod), m_minTOF(minTOF),
          m_frameWidth(frameWidth), m_isFrameSkipping(isFrameSkipping) {
      // Find how many frame widths elapsed from the time the neutrons of the
      // lead pulse were emitted and the time the neutrons arrived to the
      // detector bank. This time must be added to the stored TOF values
      const double nf = std::floor(minTOF / frameWidth);
      m_framesOffsetTime = frameWidth * nf;
    }

    double operator()(const double tof,
                      const double pathToPixelFactor = 1.0) const {
      // shift times to the correct frame
      double newTOF = tof + m_framesOffsetTime;
      // TOF values smaller than that of the fastest neutrons have been
      // 'folded' by the data acquisition system. They must be shifted
      double minTOF = m_minTOF * pathToPixelFactor;
      if (newTOF < minTOF)
        newTOF += m_frameWidth;
      // Events from the skipped pulse are delayed by one pulse period
      if (m_isFrameSkipping && newTOF > minTOF + m_pulsePeriod)
        newTOF += m_pulsePeriod;
      return newTOF;
    }

    double m_pulsePeriod;
    double m_minTOF;
    double m_frameWidth;
    double m_framesOffsetTime;
    bool m_isFrameSkipping;
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

    // Determine the factor by which to enlarge the minimum time-of-flight
    // if considering the path to the pixel instead of to the detector center
    double pathToPixelFactor(1.0); // factor for path to detector center
    if (pathToPixel) {
      pathToPixelFactor = (msd + spectrumInfo.l2(ispec)) / mdd;
    }
    auto tofCorrector =
        std::bind(correctTof, std::placeholders::_1, pathToPixelFactor);

    evlist.convertTof(tofCorrector);
    progress.report("Correct TOF frame");
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  // Set bin boundaries to absolute minimum and maximum
  inputWS->resetAllXToSingleBin();
}

} // namespace Algorithms
} // namespace Mantid
