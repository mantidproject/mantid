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
#include "MantidKernel/Material.h"
#include "MantidKernel/PseudoRandomNumberGenerator.h"
#include <iomanip>

namespace Mantid {
using Geometry::Track;
using Kernel::V3D;

namespace Algorithms {

/**
 * Construct the volume encompassing the sample + any environment kit. The
 * beam profile defines a bounding region for the sampling of the scattering
 * position.
 * @param sample A reference to a sample object that defines a valid shape
 * & material
 * @param activeRegion Restrict scattering point sampling to this region
 * @param logger Logger to write debug information on scatter pt stats
 * @param maxScatterAttempts The maximum number of tries to generate a random
 * point within the object. [Default=5000]
 * @param pointsIn Where to generate the scattering point in
 */
MCInteractionVolume::MCInteractionVolume(
    const API::Sample &sample, const Geometry::BoundingBox &activeRegion,
    Kernel::Logger &logger, const size_t maxScatterAttempts,
    const MCInteractionVolume::ScatteringPointVicinity pointsIn)
    : m_sample(sample.getShape().clone()), m_env(nullptr),
      m_activeRegion(activeRegion), m_maxScatterAttempts(maxScatterAttempts),
      m_logger(logger), m_pointsIn(pointsIn) {
  if (!m_sample->hasValidShape()) {
    throw std::invalid_argument(
        "MCInteractionVolume() - Sample shape does not have a valid shape.");
  }
  try {
    m_env = &sample.getEnvironment();
    assert(m_env);
    if (m_env->nelements() == 0) {
      throw std::invalid_argument(
          "MCInteractionVolume() - Sample enviroment has zero components.");
    }
  } catch (std::runtime_error &) {
    // swallow this as no defined environment from getEnvironment
  }

  if (m_env) {
    m_envScatterPoints.resize(m_env->nelements(), {0, 0});
  }
}

/**
 * Returns the axis-aligned bounding box for the volume
 * @return A reference to the bounding box
 */
const Geometry::BoundingBox &MCInteractionVolume::getBoundingBox() const {
  return m_sample->getBoundingBox();
}

/**
 * Randomly select a component across the sample/environment
 * @param rng A reference to a PseudoRandomNumberGenerator where
 * nextValue should return a flat random number between 0.0 & 1.0
 * @return The randomly selected component index
 */
int MCInteractionVolume::getComponentIndex(
    Kernel::PseudoRandomNumberGenerator &rng) {
  // the sample has componentIndex -1, env components are number 0 upwards
  int componentIndex = -1;
  if (m_pointsIn != ScatteringPointVicinity::SAMPLEONLY && m_env) {
    const int randomStart =
        (m_pointsIn == ScatteringPointVicinity::ENVIRONMENTONLY) ? 1 : 0;
    componentIndex =
        rng.nextInt(randomStart, static_cast<int>(m_env->nelements())) - 1;
  }
  return componentIndex;
}

/**
 * Generate a point in an object identified by an index
 * @param componentIndex Index of the sample/environment component where
 * the sample is -1
 * @param rng A reference to a PseudoRandomNumberGenerator where
 * nextValue should return a flat random number between 0.0 & 1.0
 * @return The generated point
 */

boost::optional<Kernel::V3D> MCInteractionVolume::generatePointInObjectByIndex(
    int componentIndex, Kernel::PseudoRandomNumberGenerator &rng) {
  boost::optional<Kernel::V3D> pointGenerated{boost::none};
  if (componentIndex == -1) {
    pointGenerated = m_sample->generatePointInObject(rng, m_activeRegion, 1);
  } else {
    pointGenerated = m_env->getComponent(componentIndex)
                         .generatePointInObject(rng, m_activeRegion, 1);
  }
  return pointGenerated;
}

/**
 * Update the scatter point counts
 * @param componentIndex Index of the sample/environment component where
 * the sample is -1
 * @param pointUsed Whether the generated point is actually used in the
 * simulation. If no valid tracks before and after scatter are generated
 * the point will be discarded
 */
void MCInteractionVolume::UpdateScatterPointCounts(int componentIndex,
                                                   bool pointUsed) {
  ScatterPointStat *StatToUpdate;
  if (componentIndex == -1) {
    StatToUpdate = &m_sampleScatterPoints;
  } else {
    StatToUpdate = &m_envScatterPoints[componentIndex];
  }
  if (pointUsed) {
    StatToUpdate->usedPointCount++;
  } else {
    StatToUpdate->generatedPointCount++;
  }
}

/**
 * Update the scattering angle statistics
 * @param toStart Vector from scatter point to point on beam profile where
 * @param scatteredDirec Vector from scatter point to detector
 */
void MCInteractionVolume::UpdateScatterAngleStats(V3D toStart,
                                                  V3D scatteredDirec) {
  double scatterAngleDegrees = scatteredDirec.angle(-toStart) * 180. / M_PI;
  double delta = scatterAngleDegrees - m_scatterAngleMean;
  int totalScatterPoints = m_sampleScatterPoints.usedPointCount;
  for (auto stat : m_envScatterPoints) {
    totalScatterPoints += stat.usedPointCount;
  }
  m_scatterAngleMean += delta / totalScatterPoints;
  m_scatterAngleM2 += delta * (scatterAngleDegrees - m_scatterAngleMean);
  m_scatterAngleSD = sqrt(m_scatterAngleM2 / totalScatterPoints);
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
ComponentScatterPoint
MCInteractionVolume::generatePoint(Kernel::PseudoRandomNumberGenerator &rng) {
  for (size_t i = 0; i < m_maxScatterAttempts; i++) {
    int componentIndex = getComponentIndex(rng);
    boost::optional<Kernel::V3D> pointGenerated =
        generatePointInObjectByIndex(componentIndex, rng);
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
 * @param beforeScatter Out parameter to return generated before scatter track
 * @param afterScatter Out parameter to return generated after scatter track
 * @return Whether before/after tracks were successfully generated
 */
bool MCInteractionVolume::calculateBeforeAfterTrack(
    Kernel::PseudoRandomNumberGenerator &rng, const Kernel::V3D &startPos,
    const Kernel::V3D &endPos, Geometry::Track &beforeScatter,
    Geometry::Track &afterScatter) {
  // Generate scatter point. If there is an environment present then
  // first select whether the scattering occurs on the sample or the
  // environment. The attenuation for the path leading to the scatter point
  // is calculated in reverse, i.e. defining the track from the scatter pt
  // backwards for simplicity with how the Track object works. This avoids
  // having to understand exactly which object the scattering occurred in.
  ComponentScatterPoint scatterPos;

  scatterPos = generatePoint(rng);
  UpdateScatterPointCounts(scatterPos.componentIndex, false);

  const auto toStart = normalize(startPos - scatterPos.scatterPoint);
  beforeScatter = Track(scatterPos.scatterPoint, toStart);
  int nlinks = m_sample->interceptSurface(beforeScatter);
  if (m_env) {
    nlinks += m_env->interceptSurfaces(beforeScatter);
  }
  // This should not happen but numerical precision means that it can
  // occasionally occur with tracks that are very close to the surface
  if (nlinks == 0) {
    return false;
  }
  UpdateScatterPointCounts(scatterPos.componentIndex, true);

  // Now track to final destination
  const V3D scatteredDirec = normalize(endPos - scatterPos.scatterPoint);
  afterScatter = Track(scatterPos.scatterPoint, scatteredDirec);
  m_sample->interceptSurface(afterScatter);
  if (m_env) {
    m_env->interceptSurfaces(afterScatter);
  }
  UpdateScatterAngleStats(toStart, scatteredDirec);
  return true;
}

/**
 * Calculate the attenuation correction factor the volume given a before
 * and after track.
 * @param beforeScatter Before scatter track
 * @param afterScatter After scatter track
 * @param lambdaBefore Lambda before scattering
 * @param lambdaAfter Lambda after scattering
 * @return Absorption factor
 */
double MCInteractionVolume::calculateAbsorption(const Track &beforeScatter,
                                                const Track &afterScatter,
                                                double lambdaBefore,
                                                double lambdaAfter) const {

  // Function to calculate total attenuation for a track
  auto calculateAttenuation = [](const Track &path, double lambda) {
    double factor(1.0);
    for (const auto &segment : path) {
      const double length = segment.distInsideObject;
      const auto &segObj = *(segment.object);
      factor *= segObj.material().attenuation(length, lambda);
    }
    return factor;
  };

  return calculateAttenuation(beforeScatter, lambdaBefore) *
         calculateAttenuation(afterScatter, lambdaAfter);
}

/**
 * Log a debug string summarising which parts of the environment
 * the simulated scatter points occurred in
 */
void MCInteractionVolume::generateScatterPointStats() {
  if (m_logger.is(Kernel::Logger::Priority::PRIO_DEBUG)) {
    std::stringstream scatterPointSummary;
    scatterPointSummary << std::fixed;
    scatterPointSummary << std::setprecision(2);

    scatterPointSummary << "Scatter point counts:" << std::endl;

    int totalScatterPointsGenerated = m_sampleScatterPoints.generatedPointCount;
    int totalScatterPointsUsed = m_sampleScatterPoints.usedPointCount;
    for (auto stat : m_envScatterPoints) {
      totalScatterPointsGenerated += stat.generatedPointCount;
      totalScatterPointsUsed += stat.usedPointCount;
    }

    scatterPointSummary << "Total scatter points generated: "
                        << totalScatterPointsGenerated << std::endl;
    scatterPointSummary << "Total scatter points used: "
                        << totalScatterPointsUsed << std::endl;

    if (m_env && (m_env->nelements() > 0)) {
      double percentage =
          static_cast<double>(m_sampleScatterPoints.usedPointCount) /
          totalScatterPointsUsed * 100;
      scatterPointSummary << "Sample: " << m_sampleScatterPoints.usedPointCount
                          << " (" << percentage << "%)" << std::endl;

      for (std::vector<int>::size_type i = 0; i < m_envScatterPoints.size();
           i++) {
        percentage = static_cast<double>(m_envScatterPoints[i].usedPointCount) /
                     totalScatterPointsUsed * 100;
        scatterPointSummary
            << "Environment part " << i << " (" << m_env->getComponent(i).id()
            << "): " << m_envScatterPoints[i].usedPointCount << " ("
            << percentage << "%)" << std::endl;
      }
    }
    scatterPointSummary << "Scattering angle mean (degrees)="
                        << m_scatterAngleMean << std::endl;
    scatterPointSummary << "Scattering angle sd (degrees)=" << m_scatterAngleSD
                        << std::endl;
    m_logger.debug(scatterPointSummary.str());

    // reset the counters
    m_sampleScatterPoints = {0, 0};
    std::fill(m_envScatterPoints.begin(), m_envScatterPoints.end(),
              ScatterPointStat{0, 0});
    m_scatterAngleMean = 0;
    m_scatterAngleM2 = 0;
  }
}

} // namespace Algorithms
} // namespace Mantid
