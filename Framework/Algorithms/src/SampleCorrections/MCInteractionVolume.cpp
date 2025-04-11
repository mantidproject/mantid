// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/SampleCorrections/MCInteractionVolume.h"
#include "MantidAPI/Sample.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidKernel/PseudoRandomNumberGenerator.h"
#include <iomanip>

namespace Mantid {
using Geometry::IObject_sptr;
using Geometry::Track;
using Kernel::V3D;

namespace Algorithms {

/**
 * Construct the volume encompassing the sample + any environment kit. The
 * active region defines a bounding region for the sampling of the scattering
 * position.
 */
void MCInteractionVolume::init() {
  m_activeRegion = getFullBoundingBox();
  if (m_env) {
    try {
      if (m_env->nelements() == 0) {
        throw std::invalid_argument("MCInteractionVolume() - Sample enviroment has zero components.");
      }
    } catch (std::runtime_error &) {
      // swallow this as no defined environment from getEnvironment
    }
  }

  bool atLeastOneValidShape = m_sample->hasValidShape();
  if (!atLeastOneValidShape && m_env) {
    for (size_t i = 0; i < m_env->nelements(); i++) {
      if (m_env->getComponent(i).hasValidShape()) {
        atLeastOneValidShape = true;
        break;
      }
    }
  }
  if (!atLeastOneValidShape) {
    throw std::invalid_argument("MCInteractionVolume() - Either the Sample or one of the "
                                "environment parts must have a valid shape.");
  }
}

/**
 * Factory Method for constructing the volume encompassing the sample + any environment kit. The
 * active region defines a bounding region for the sampling of the scattering
 * position.
 * @param sample A reference to a sample object that defines a valid shape
 * & material
 * @param maxScatterAttempts The maximum number of tries to generate a random
 * point within the object. [Default=5000]
 * @param pointsIn Where to generate the scattering point in
 * @param gaugeVolume Pointer to any gauge volume object defined for the interaction
 * @return returns a shared pointer to interaction volume object
 */
std::shared_ptr<IMCInteractionVolume>
MCInteractionVolume::create(const API::Sample &sample, const size_t maxScatterAttempts,
                            const MCInteractionVolume::ScatteringPointVicinity pointsIn, IObject_sptr gaugeVolume) {
  auto interactionVol =
      std::shared_ptr<MCInteractionVolume>(new MCInteractionVolume(sample, maxScatterAttempts, pointsIn, gaugeVolume));
  // init calls member functions that require instantiation of virtual functions so deferring this until after
  // construction through a factory is preferable.
  interactionVol->init();
  return interactionVol;
}

/**
 * Construct the volume encompassing the sample + any environment kit. The
 * active region defines a bounding region for the sampling of the scattering
 * position.
 * @param sample A reference to a sample object that defines a valid shape
 * & material
 * @param maxScatterAttempts The maximum number of tries to generate a random
 * point within the object. [Default=5000]
 * @param pointsIn Where to generate the scattering point in
 * @param gaugeVolume Pointer to any gauge volume object defined for the interaction
 */
MCInteractionVolume::MCInteractionVolume(const API::Sample &sample, const size_t maxScatterAttempts,
                                         const MCInteractionVolume::ScatteringPointVicinity pointsIn,
                                         IObject_sptr gaugeVolume)
    : m_sample(sample.getShape().clone()), m_env(sample.hasEnvironment() ? &sample.getEnvironment() : nullptr),
      m_maxScatterAttempts(maxScatterAttempts), m_pointsIn(pointsIn), m_gaugeVolume(gaugeVolume) {}

/**
 * Returns the defined gauge volume if one is present, otherwise returns nullptr
 * @return Shared pointer to gauge volume object
 */
Geometry::IObject_sptr MCInteractionVolume::getGaugeVolume() const { return m_gaugeVolume; }

/**
 * Sets the gauge volume on the InteractionVolume
 * @param gaugeVolume A shared pointer to the gauge volume shape object
 */
void MCInteractionVolume::setGaugeVolume(Geometry::IObject_sptr gaugeVolume) { m_gaugeVolume = gaugeVolume; }

/**
 * Returns the defined gauge volume if one is present, otherwise returns axis-aligned bounding box
 * for the volume including env if m_pointsIn != SampleOnly
 * @return The bounding box of the interaction volume
 */
const Geometry::BoundingBox MCInteractionVolume::getFullBoundingBox() const {
  auto sampleBox = m_sample->getBoundingBox();
  if (m_gaugeVolume != nullptr) {
    sampleBox = m_gaugeVolume->getBoundingBox();
  } else if (m_pointsIn != ScatteringPointVicinity::SAMPLEONLY && m_env) {
    const auto &envBox = m_env->boundingBox();
    sampleBox.grow(envBox);
  }
  return sampleBox;
}

/**
 * Sets the active region volume on the InteractionVolume
 * @param region A reference to a Bounding Box for the active region
 */
void MCInteractionVolume::setActiveRegion(const Geometry::BoundingBox &region) { m_activeRegion = region; }

/**
 * Randomly select a component across the sample/environment
 * @param rng A reference to a PseudoRandomNumberGenerator where
 * nextValue should return a flat random number between 0.0 & 1.0
 * @return The randomly selected component index
 */
int MCInteractionVolume::getComponentIndex(Kernel::PseudoRandomNumberGenerator &rng) const {
  // the sample has componentIndex -1, env components are number 0 upwards
  const int sampleIndex = -1;
  const int firstEnvIndex = sampleIndex + 1;
  int startIndex = sampleIndex;
  int endIndex = m_env ? static_cast<int>(m_env->nelements()) - 1 : sampleIndex;
  if ((m_pointsIn == ScatteringPointVicinity::ENVIRONMENTONLY) || !m_sample->hasValidShape())
    startIndex = firstEnvIndex;
  if (m_pointsIn == ScatteringPointVicinity::SAMPLEONLY)
    endIndex = sampleIndex;
  if (startIndex == endIndex) {
    return startIndex;
  } else {
    return rng.nextInt(startIndex, endIndex);
  }
}

/**
 * Generate a point in an object identified by an index
 * @param componentIndex Index of the sample/environment component where
 * the sample is -1
 * @param rng A reference to a PseudoRandomNumberGenerator where
 * nextValue should return a flat random number between 0.0 & 1.0
 * @return The generated point
 */

std::optional<Kernel::V3D>
MCInteractionVolume::generatePointInObjectByIndex(int componentIndex, Kernel::PseudoRandomNumberGenerator &rng) const {
  std::optional<Kernel::V3D> pointGenerated{std::nullopt};
  std::optional<Kernel::V3D> tmpPoint{std::nullopt};
  if (componentIndex == -1) {
    if (m_gaugeVolume != nullptr) {
      tmpPoint = m_gaugeVolume->generatePointInObject(rng, m_activeRegion, 1);
      if (tmpPoint) {
        if (m_sample->isValid(tmpPoint.value())) {
          pointGenerated = tmpPoint;
        }
      }
    } else {
      pointGenerated = m_sample->generatePointInObject(rng, m_activeRegion, 1);
    }
  } else {
    pointGenerated = m_env->getComponent(componentIndex).generatePointInObject(rng, m_activeRegion, 1);
  }
  return pointGenerated;
}

/**
 * Generate point randomly across one of the components of the environment
 * including the sample itself in the selection. The method first selects
 * a random component and then selects a random point within that component
 * using Object::generatePointObject
 * @param rng A reference to a PseudoRandomNumberGenerator where
 * nextValue should return a flat random number between 0.0 & 1.0
 * @return A struct containing the generated point and the index of the
 * component containing the scatter point
 */
ComponentScatterPoint MCInteractionVolume::generatePoint(Kernel::PseudoRandomNumberGenerator &rng) const {
  for (size_t i = 0; i < m_maxScatterAttempts; i++) {
    int componentIndex = getComponentIndex(rng);
    std::optional<Kernel::V3D> pointGenerated = generatePointInObjectByIndex(componentIndex, rng);
    if (pointGenerated) {
      return {componentIndex, *pointGenerated};
    }
  }
  throw std::runtime_error("MCInteractionVolume::generatePoint() - Unable to "
                           "generate point in object after " +
                           std::to_string(m_maxScatterAttempts) + " attempts");
}

/**
 * Calculate a before scatter and after scatter track based on a scatter point
 * in the volume given a start and end point.
 * @param rng A reference to a PseudoRandomNumberGenerator producing
 * random number between [0,1]
 * @param startPos Origin of the initial track
 * @param endPos Final position of neutron after scattering (assumed to be
 * outside of the "volume")
 * @param stats A statistics class to hold the statistics on the generated
 * tracks such as the scattering angle and count of scatter points generated in
 * each sample or environment part
 * @return A tuple containing a flag to indicate whether before/after tracks
 * were successfully generated and (if yes) the before/after tracks
 */
TrackPair MCInteractionVolume::calculateBeforeAfterTrack(Kernel::PseudoRandomNumberGenerator &rng,
                                                         const Kernel::V3D &startPos, const Kernel::V3D &endPos,
                                                         MCInteractionStatistics &stats) const {
  // Generate scatter point. If there is an environment present then
  // first select whether the scattering occurs on the sample or the
  // environment. The attenuation for the path leading to the scatter point
  // is calculated in reverse, i.e. defining the track from the scatter pt
  // backwards for simplicity with how the Track object works. This avoids
  // having to understand exactly which object the scattering occurred in.
  ComponentScatterPoint scatterPos;

  scatterPos = generatePoint(rng);
  stats.UpdateScatterPointCounts(scatterPos.componentIndex, false);

  const auto toStart = normalize(startPos - scatterPos.scatterPoint);
  auto beforeScatter = std::make_shared<Track>(scatterPos.scatterPoint, toStart);
  int nlinks = m_sample->interceptSurface(*beforeScatter);
  if (m_env) {
    nlinks += m_env->interceptSurfaces(*beforeScatter);
  }
  // This should not happen but numerical precision means that it can
  // occasionally occur with tracks that are very close to the surface
  if (nlinks == 0) {
    return {false, nullptr, nullptr};
  }
  stats.UpdateScatterPointCounts(scatterPos.componentIndex, true);

  // Now track to final destination
  const V3D scatteredDirec = normalize(endPos - scatterPos.scatterPoint);
  auto afterScatter = std::make_shared<Track>(scatterPos.scatterPoint, scatteredDirec);
  m_sample->interceptSurface(*afterScatter);
  if (m_env) {
    m_env->interceptSurfaces(*afterScatter);
  }
  stats.UpdateScatterAngleStats(toStart, scatteredDirec);
  return {true, beforeScatter, afterScatter};
}

} // namespace Algorithms
} // namespace Mantid
