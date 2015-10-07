// Includes
#include "MantidMDAlgorithms/Quantification/Resolution/ModeratorChopperResolution.h"
#include "MantidMDAlgorithms/Quantification/CachedExperimentInfo.h"

#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/ChopperModel.h"
#include "MantidAPI/ModeratorModel.h"

namespace Mantid {
namespace MDAlgorithms {
using Geometry::Instrument_const_sptr;
using Geometry::IObjComponent_const_sptr;
using Geometry::IComponent_const_sptr;
using Geometry::IDetector_const_sptr;

/**
 * Constructor taking an observation object
 * @param observation :: An event containing an experiment description &
 * detector ID
 */
ModeratorChopperResolution::ModeratorChopperResolution(
    const CachedExperimentInfo &observation)
    : m_observation(observation), m_modChopDist(0.0), m_chopSampleDist(0.0) {
  initCaches();
}

/// Destructor
ModeratorChopperResolution::~ModeratorChopperResolution() {}

/**
 * Returns the width, i.e the standard-deviation, in energy that the
 * moderator-chopper contributes to
 * energy resolution
 * @param deltaE :: The energy change of the final detector
 * @returns The standard deviation of the energy resolution in meV
 */
double ModeratorChopperResolution::energyWidth(const double deltaE) const {
  const static double mevToSpeedSq =
      2.0 * PhysicalConstants::meV / PhysicalConstants::NeutronMass;
  const double efixed = m_observation.getEFixed();
  const Kernel::DeltaEMode::Type emode =
      m_observation.experimentInfo().getEMode();
  const double sampleToDetDist = m_observation.sampleToDetectorDistance();

  double ei(0.0), ef(0.0);
  if (emode == Kernel::DeltaEMode::Elastic) {
    ei = ef = efixed;
  } else if (emode == Kernel::DeltaEMode::Direct) {
    ei = efixed;
    ef = efixed - deltaE;
  } else {
    ei = efixed + deltaE;
    ef = efixed;
  }

  const double neutronSpeed = std::sqrt(mevToSpeedSq * ei);
  const double modChopTime =
      m_observation.moderatorToFirstChopperDistance() / neutronSpeed;
  const double stdDevModPulse = std::sqrt(m_moderator->emissionTimeVariance());
  double moderatorVar =
      ((stdDevModPulse / modChopTime) *
       (1.0 + (m_chopSampleDist / sampleToDetDist) * std::pow(ef / ei, 3)));
  moderatorVar *= moderatorVar;

  const double stdDevChopPulse = m_chopper->pulseTimeVariance();
  double chopperVar = ((stdDevChopPulse / modChopTime) *
                       (1.0 +
                        ((m_modChopDist + m_chopSampleDist) / sampleToDetDist) *
                            std::pow(ef / ei, 3)));
  chopperVar *= chopperVar;

  const double energyRes = 2.0 * ei * std::sqrt(moderatorVar + chopperVar);
  return energyRes;
}

/**
 * Store required caches
 */
void ModeratorChopperResolution::initCaches() {
  Instrument_const_sptr instr = m_observation.experimentInfo().getInstrument();
  IComponent_const_sptr source = instr->getSource();

  m_modChopDist = m_observation.moderatorToFirstChopperDistance();
  m_chopSampleDist = m_observation.firstChopperToSampleDistance();
}
}
}
