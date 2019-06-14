// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/HyspecScharpfCorrection.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"

namespace Mantid {
namespace Algorithms {

using Mantid::API::WorkspaceProperty;
using Mantid::Kernel::Direction;
using namespace DataObjects;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(HyspecScharpfCorrection)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string HyspecScharpfCorrection::name() const {
  return "HyspecScharpfCorrection";
}

/// Algorithm's version for identification. @see Algorithm::version
int HyspecScharpfCorrection::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string HyspecScharpfCorrection::category() const {
  return "CorrectionFunctions\\SpecialCorrections; Inelastic\\Corrections";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string HyspecScharpfCorrection::summary() const {
  return "Apply polarization factor as part of getting the spin incoherent "
         "scattering";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void HyspecScharpfCorrection::init() {
  auto wsValidator = boost::make_shared<Mantid::Kernel::CompositeValidator>();
  wsValidator->add<Mantid::API::WorkspaceUnitValidator>("DeltaE");
  wsValidator->add<Mantid::API::InstrumentValidator>();
  declareProperty(std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input, wsValidator),
                  "An input workspace in units of energy transfer.");

  auto angleValidator =
      boost::make_shared<Mantid::Kernel::BoundedValidator<double>>();
  angleValidator->setLower(-180.0);
  angleValidator->setUpper(180.0);
  declareProperty("PolarizationAngle", EMPTY_DBL(), angleValidator,
                  "In plane angle between polatrization and incident beam"
                  "Must be between -180 and +180 degrees");
  auto precisionValidator =
      boost::make_shared<Mantid::Kernel::BoundedValidator<double>>();
  precisionValidator->setLower(0.0);
  precisionValidator->setUpper(1.0);
  declareProperty(
      "Precision", 0.1, precisionValidator,
      "If cosine of twice the "
      "Scharpf angle is closer to 0 than the precision, the intensities "
      "and errors will be set to 0");
  declareProperty(std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void HyspecScharpfCorrection::exec() {
  // Get the workspaces
  m_inputWS = this->getProperty("InputWorkspace");
  m_outputWS = this->getProperty("OutputWorkspace");
  m_angle = getProperty("PolarizationAngle");
  m_angle *= M_PI / 180.;
  m_precision = getProperty("Precision");
  if (m_inputWS->run().hasProperty("Ei")) {
    m_Ei = m_inputWS->run().getPropertyValueAsType<double>("Ei");
  } else {
    throw std::invalid_argument(
        "No Ei value has been set or stored within the run information.");
  }

  // Check if it is an event workspace
  if (dynamic_cast<const Mantid::DataObjects::EventWorkspace *>(
          m_inputWS.get()) != nullptr) {
    this->execEvent();
    return;
  }

  // If input and output workspaces are not the same, create a new workspace for
  // the output
  if (m_outputWS != m_inputWS) {
    m_outputWS = create<API::MatrixWorkspace>(*m_inputWS);
  }

  const auto &spectrumInfo = m_inputWS->spectrumInfo();

  // Get number of spectra in this workspace
  const int64_t numberOfSpectra =
      static_cast<int64_t>(m_inputWS->getNumberHistograms());
  Mantid::Kernel::V3D samplePos = spectrumInfo.samplePosition();
  const auto refFrame = m_inputWS->getInstrument()->getReferenceFrame();
  API::Progress prog(this, 0.0, 1.0, numberOfSpectra);
  PARALLEL_FOR_IF(Kernel::threadSafe(*m_inputWS, *m_outputWS))
  for (int64_t i = 0; i < numberOfSpectra; ++i) {
    PARALLEL_START_INTERUPT_REGION
    auto &yOut = m_outputWS->mutableY(i);
    auto &eOut = m_outputWS->mutableE(i);

    const auto &xIn = m_inputWS->points(i); // get the centers
    auto &yIn = m_inputWS->y(i);
    auto &eIn = m_inputWS->e(i);
    // Copy the energy transfer axis
    m_outputWS->setSharedX(i, m_inputWS->sharedX(i));

    prog.report();
    // continue if no detectors, if monitor, or is masked
    if ((!spectrumInfo.hasDetectors(i)) || spectrumInfo.isMonitor(i) ||
        spectrumInfo.isMasked(i)) {
      continue;
    }
    // get detector info and calculate the in plane angle
    Mantid::Kernel::V3D detPos = spectrumInfo.position(i);
    const auto l2 = detPos - samplePos;
    const double thPlane = std::atan2(l2[refFrame->pointingHorizontal()],
                                      l2[refFrame->pointingAlongBeam()]);
    size_t spectrumSize = xIn.size();
    for (size_t j = 0; j < spectrumSize; ++j) {
      double factor = 0.;
      if (xIn[j] < m_Ei) {
        double kfki = std::sqrt(1. - xIn[j] / m_Ei); // k_f/k_i
        factor = static_cast<double>(this->calculateFactor(kfki, thPlane));
      }
      yOut[j] = yIn[j] * factor;
      eOut[j] = eIn[j] * factor;
    }
    PARALLEL_END_INTERUPT_REGION
  } // end for i
  PARALLEL_CHECK_INTERUPT_REGION
  this->setProperty("OutputWorkspace", m_outputWS);
}

/** Execute for events
 */
void HyspecScharpfCorrection::execEvent() {
  g_log.information("Processing event workspace");

  // If input and output workspaces are not the same, create a new workspace for
  // the output
  if (m_outputWS != m_inputWS) {
    m_outputWS = m_inputWS->clone();
    setProperty("OutputWorkspace", m_outputWS);
  }

  Mantid::DataObjects::EventWorkspace_sptr eventWS =
      boost::dynamic_pointer_cast<Mantid::DataObjects::EventWorkspace>(
          m_outputWS);

  const auto &spectrumInfo = m_inputWS->spectrumInfo();

  // Get number of spectra in this workspace
  const int64_t numberOfSpectra =
      static_cast<int64_t>(m_inputWS->getNumberHistograms());
  Mantid::Kernel::V3D samplePos = spectrumInfo.samplePosition();
  const auto refFrame = m_inputWS->getInstrument()->getReferenceFrame();
  API::Progress prog(this, 0.0, 1.0, numberOfSpectra);
  PARALLEL_FOR_IF(Kernel::threadSafe(*m_inputWS, *m_outputWS))
  for (int64_t i = 0; i < numberOfSpectra; ++i) {
    PARALLEL_START_INTERUPT_REGION
    prog.report();
    // continue if no detectors, if monitor, or is masked
    if ((!spectrumInfo.hasDetectors(i)) || spectrumInfo.isMonitor(i) ||
        spectrumInfo.isMasked(i)) {
      continue;
    }
    Mantid::Kernel::V3D detPos = spectrumInfo.position(i);
    const auto l2 = detPos - samplePos;
    const double thPlane = std::atan2(l2[refFrame->pointingHorizontal()],
                                      l2[refFrame->pointingAlongBeam()]);
    // Do the correction
    auto &evlist = eventWS->getSpectrum(i);
    switch (evlist.getEventType()) {
    case Mantid::API::TOF:
      // Switch to weights if needed.
      evlist.switchTo(Mantid::API::WEIGHTED);
      /* no break */
      // Fall through

    case Mantid::API::WEIGHTED:
      ScharpfEventHelper(evlist.getWeightedEvents(), thPlane);
      break;

    case Mantid::API::WEIGHTED_NOTIME:
      ScharpfEventHelper(evlist.getWeightedEventsNoTime(), thPlane);
      break;
    }
    PARALLEL_END_INTERUPT_REGION
  } // end for i
  PARALLEL_CHECK_INTERUPT_REGION
}

template <class T>
void HyspecScharpfCorrection::ScharpfEventHelper(std::vector<T> &wevector,
                                                 double thPlane) {
  for (auto it = wevector.begin(); it < wevector.end();) {
    double Ef = m_Ei - it->tof();
    if (Ef <= 0) {
      it = wevector.erase(it);
    } else {
      double kfki = std::sqrt(Ef / m_Ei);

      float factor = this->calculateFactor(kfki, thPlane);

      it->m_weight *= factor;
      it->m_errorSquared *= factor * factor;
      ++it;
    }
  }
}

float HyspecScharpfCorrection::calculateFactor(const double kfki,
                                               const double thPlane) {
  // angle between in plane Q and z axis
  const double angleQ =
      std::atan2(-kfki * std::sin(thPlane), 1. - kfki * std::cos(thPlane));
  // Scarpf agle = angle - angleQ
  float factor = static_cast<float>(std::cos(2. * (m_angle - angleQ)));
  // set intensity to 0 if the Scarpf angle is close to 45 degrees
  if (std::abs(factor) > m_precision) {
    factor = 1.f / factor;
  } else {
    factor = 0.;
  }

  return (factor);
}

} // namespace Algorithms
} // namespace Mantid
