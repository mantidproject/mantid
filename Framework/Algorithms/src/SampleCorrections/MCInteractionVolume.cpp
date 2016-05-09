#include "MantidAlgorithms/SampleCorrections/MCInteractionVolume.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SampleEnvironment.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Objects/Track.h"
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
}

/**
 * Construct the volume with only a sample object
 * @param sample A reference to a sample object that defines a valid shape
 * & material
 */
MCInteractionVolume::MCInteractionVolume(const API::Sample &sample)
    : m_sample(sample.getShape()), m_env(nullptr) {
  if (!m_sample.hasValidShape()) {
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
 * Calculate the attenuation correction factor for given track through the
 * volume
 * @param rng A reference to a PseudoRandomNumberGenerator
 * @param startPos Origin of the initial track
 * @param direc Direction of travel of the neutron
 * @param endPos Final position of neutron after scattering (assumed to be
 * outside of the "volume")
 * @param lambdaBefore Wavelength, in \f$\\A^-1\f$, before scattering
 * @param lambdaAfter Wavelength, in \f$\\A^-1\f$, after scattering
 * @return The fraction of the beam that has been attenuated
 */
double MCInteractionVolume::calculateAbsorption(
    Kernel::PseudoRandomNumberGenerator &rng, const Kernel::V3D &startPos,
    const Kernel::V3D &direc, const Kernel::V3D &endPos, double lambdaBefore,
    double lambdaAfter) const {
  // Create track with start position and direction and "fire" it through
  // the sample to produce a number of intersections. Choose a random
  // intersection and within this section pick a random "depth". This point
  // is the scatter point.
  // Form a second track originating at the scatter point and ending at endPos
  // to give a second set of intersections.
  // The total attenuation factor is the product of the attenuation factor
  // for each intersection

  // Generate scatter point
  Track path1(startPos, direc);
  int nsegments = m_sample.interceptSurface(path1);
  if (m_env) {
    nsegments += m_env->interceptSurfaces(path1);
  }
  if (nsegments == 0) {
    // The track passed through nothing and so was not attenuated at all.
    return 1.0;
  }
  int scatterSegmentNo(1);
  if (nsegments != 1) {
    scatterSegmentNo = rng.nextInt(1, nsegments);
  }

  double atten(1.0);
  V3D scatterPos;
  auto segItr(path1.cbegin());
  for (int i = 0; i < scatterSegmentNo; ++i, ++segItr) {
    double length = segItr->distInsideObject;
    if (i == scatterSegmentNo - 1) {
      length *= rng.nextValue();
      scatterPos = segItr->entryPoint + direc * length;
    }
    const auto &segObj = *(segItr->object);
    const auto segMat = segObj.material();
    atten *= attenuation(segMat.numberDensity(),
                         segMat.totalScatterXSection(lambdaBefore) +
                             segMat.absorbXSection(lambdaBefore),
                         length);
  }

  // Now track to final destination
  V3D scatteredDirec = endPos - scatterPos;
  scatteredDirec.normalize();
  Track path2(scatterPos, scatteredDirec);
  m_sample.interceptSurface(path2);
  if (m_env) {
    m_env->interceptSurfaces(path2);
  }

  for (const auto &segment : path2) {
    double length = segment.distInsideObject;
    const auto &segObj = *(segment.object);
    const auto segMat = segObj.material();
    atten *= attenuation(segMat.numberDensity(),
                         segMat.totalScatterXSection(lambdaAfter) +
                             segMat.absorbXSection(lambdaAfter),
                         length);
  }

  return atten;
}

} // namespace Algorithms
} // namespace Mantid
