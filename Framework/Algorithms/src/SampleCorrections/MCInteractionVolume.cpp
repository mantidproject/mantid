// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
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
 * @param maxScatterAttempts The maximum number of tries to generate a random
 * point within the object. [Default=5000]
 */
MCInteractionVolume::MCInteractionVolume(
    const API::Sample &sample, const Geometry::BoundingBox &activeRegion,
    const size_t maxScatterAttempts)
    : m_sample(sample.getShape().clone()), m_env(nullptr),
      m_activeRegion(activeRegion), m_maxScatterAttempts(maxScatterAttempts) {
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
    m_envScatterPoints.resize(m_env->nelements());
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
  int componentIndex;
  // the sample has componentIndex -1, env components are number 0 upwards
  if (m_env) {
    componentIndex = rng.nextInt(0, static_cast<int>(m_env->nelements())) - 1;
  } else {
    componentIndex = -1;
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
 */
void MCInteractionVolume::UpdateScatterPointCounts(int componentIndex) {
  if (componentIndex == -1) {
    m_sampleScatterPoints++;
  } else {
    m_envScatterPoints[componentIndex]++;
  }
}

/**
 * Generate point randomly across one of the components of the environment
 * including the sample itself in the selection. The method first selects
 * a random component and then selects a random point within that component
 * using Object::generatePointObject
 * @param rng A reference to a PseudoRandomNumberGenerator where
 * nextValue should return a flat random number between 0.0 & 1.0
 * @return The generated point
 */
Kernel::V3D
MCInteractionVolume::generatePoint(Kernel::PseudoRandomNumberGenerator &rng) {
  for (size_t i = 0; i < m_maxScatterAttempts; i++) {
    int componentIndex = getComponentIndex(rng);
    boost::optional<Kernel::V3D> pointGenerated =
        generatePointInObjectByIndex(componentIndex, rng);
    if (pointGenerated) {
      UpdateScatterPointCounts(componentIndex);
      return *pointGenerated;
    }
  }
  throw std::runtime_error("MCInteractionVolume::generatePoint() - Unable to "
                           "generate point in object after " +
                           std::to_string(m_maxScatterAttempts) + " attempts");
}

/**
 * Calculate the attenuation correction factor the volume given a start and
 * end point.
 * @param rng A reference to a PseudoRandomNumberGenerator producing
 * random number between [0,1]
 * @param startPos Origin of the initial track
 * @param endPos Final position of neutron after scattering (assumed to be
 * outside of the "volume")
 * @param lambdaBefore Wavelength, in \f$\\A^-1\f$, before scattering
 * @param lambdaAfter Wavelength, in \f$\\A^-1\f$, after scattering
 * @return The fraction of the beam that has been attenuated. A negative number
 * indicates the track was not valid.
 */
double MCInteractionVolume::calculateAbsorption(
    Kernel::PseudoRandomNumberGenerator &rng, const Kernel::V3D &startPos,
    const Kernel::V3D &endPos, double lambdaBefore, double lambdaAfter) {
  // Generate scatter point. If there is an environment present then
  // first select whether the scattering occurs on the sample or the
  // environment. The attenuation for the path leading to the scatter point
  // is calculated in reverse, i.e. defining the track from the scatter pt
  // backwards for simplicity with how the Track object works. This avoids
  // having to understand exactly which object the scattering occurred in.

  V3D scatterPos = generatePoint(rng);

  const auto toStart = normalize(startPos - scatterPos);
  Track beforeScatter(scatterPos, toStart);
  int nlinks = m_sample->interceptSurface(beforeScatter);
  if (m_env) {
    nlinks += m_env->interceptSurfaces(beforeScatter);
  }
  // This should not happen but numerical precision means that it can
  // occasionally occur with tracks that are very close to the surface
  if (nlinks == 0) {
    return -1.0;
  }

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

  // Now track to final destination
  const V3D scatteredDirec = normalize(endPos - scatterPos);
  Track afterScatter(scatterPos, scatteredDirec);
  m_sample->interceptSurface(afterScatter);
  if (m_env) {
    m_env->interceptSurfaces(afterScatter);
  }
  return calculateAttenuation(beforeScatter, lambdaBefore) *
         calculateAttenuation(afterScatter, lambdaAfter);
}

/**
 * Generate a string summarising which parts of the environment
 * the simulated scatter points occurred in
 * @return The generated string
 */
std::string MCInteractionVolume::generateScatterPointStats() const {
  std::stringstream scatterPointSummary;
  scatterPointSummary << std::fixed;
  scatterPointSummary << std::setprecision(2);

  scatterPointSummary << "Scatter point counts:" << std::endl;

  int totalScatterPoints =
      std::accumulate(m_envScatterPoints.begin(), m_envScatterPoints.end(),
                      m_sampleScatterPoints);

  scatterPointSummary << "Total scatter points: " << totalScatterPoints
                      << std::endl;

  double percentage =
      static_cast<double>(m_sampleScatterPoints) / totalScatterPoints * 100;
  scatterPointSummary << "Sample: " << m_sampleScatterPoints << " ("
                      << percentage << "%)" << std::endl;

  for (std::vector<int>::size_type i = 0; i < m_envScatterPoints.size(); i++) {
    percentage =
        static_cast<double>(m_envScatterPoints[i]) / totalScatterPoints * 100;
    scatterPointSummary << "Environment part " << i << " ("
                        << m_env->getComponent(i).id()
                        << "): " << m_envScatterPoints[i] << " (" << percentage
                        << "%)" << std::endl;
  }
  return scatterPointSummary.str();
}

} // namespace Algorithms
} // namespace Mantid
