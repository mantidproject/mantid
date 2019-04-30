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

namespace Mantid {
using Geometry::Track;
using Kernel::V3D;

namespace Algorithms {

namespace {

/**
 * Compute the attenuation factor for the given coefficients
 * @param rho Number density of the sample in \f$\\A^{-3}\f$
 * @param sigma Cross-section in barns
 * @param length Path length in metres
 * @return The dimensionless attenuated fraction
 */
double attenuation(double rho, double sigma, double length) {
  using std::exp;
  return exp(-100 * rho * sigma * length);
}
} // namespace

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
    if (m_env->nelements() == 0) {
      throw std::invalid_argument(
          "MCInteractionVolume() - Sample enviroment has zero components.");
    }
  } catch (std::runtime_error &) {
    // swallow this as no defined environment from getEnvironment
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
    const Kernel::V3D &endPos, double lambdaBefore, double lambdaAfter) const {
  // Generate scatter point. If there is an environment present then
  // first select whether the scattering occurs on the sample or the
  // environment. The attenuation for the path leading to the scatter point
  // is calculated in reverse, i.e. defining the track from the scatter pt
  // backwards for simplicity with how the Track object works. This avoids
  // having to understand exactly which object the scattering occurred in.
  V3D scatterPos;
  if (m_env && (rng.nextValue() > 0.5)) {
    scatterPos =
        m_env->generatePoint(rng, m_activeRegion, m_maxScatterAttempts);
  } else {
    scatterPos = m_sample->generatePointInObject(rng, m_activeRegion,
                                                 m_maxScatterAttempts);
  }
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
      const auto &segMat = segObj.material();
      factor *= attenuation(segMat.numberDensity(),
                            segMat.totalScatterXSection(lambda) +
                                segMat.absorbXSection(lambda),
                            length);
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

} // namespace Algorithms
} // namespace Mantid
