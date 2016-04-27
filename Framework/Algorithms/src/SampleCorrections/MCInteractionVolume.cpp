#include "MantidAlgorithms/SampleCorrections/MCInteractionVolume.h"
#include "MantidAPI/Sample.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidKernel/PseudoRandomNumberGenerator.h"

namespace Mantid {
using Geometry::Track;
using Kernel::V3D;

namespace Algorithms {

/**
 * Construct the volume with only a sample object
 * @param sample A reference to a sample object that defines a valid shape
 * & material
 */
MCInteractionVolume::MCInteractionVolume(const API::Sample &sample)
    : m_sample(sample.getShape()) {
  if (!m_sample.hasValidShape()) {
    throw std::invalid_argument(
        "MCInteractionVolume() - Sample shape does not have a valid shape.");
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
 * @param lambdaBefore Wavelength, in \f$\A^-1\f$, before scattering
 * @param lambdaAfter Wavelength, in \f$\A^-1\f$, after scattering
 * @return The fraction of the beam that has been attenuated
 */
double MCInteractionVolume::calculateAbsorption(
    Kernel::PseudoRandomNumberGenerator &rng, const Kernel::V3D &startPos,
    const Kernel::V3D &direc, const Kernel::V3D &endPos, double lambdaBefore,
    double lambdaAfter) {
  // Create track with start position and direction and "fire" it through
  // the sample to produce a number of intersections. Choose a random
  // intersection and within this section pick a random "depth". This point
  // is the scatter point.
  // Form a second track originating at the scatter point and ending at endPos
  // to give a second set of intersections.
  // The total attenuation factor is the product of the attenuation factor
  // for each intersection
  Track path1(startPos, direc);
  const int nsegments = m_sample.interceptSurface(path1);
  if (nsegments == 0) {
    throw std::logic_error(
        "MCInteractionVolume::calculateAbsorption() - Zero "
        "intersections found for input track and sample object.");
  }
  int scatterSegmentNo(1);
  if (nsegments != 1) {
    scatterSegmentNo = rng.nextInt(1, nsegments);
  }

  // Generate scatter point
  double l1(0.0);
  V3D scatterPos;
  int segmentCount(0);
  for (const auto &segment : path1) {
    segmentCount += 1;
    if (segmentCount != scatterSegmentNo) {
      l1 += segment.distInsideObject;
    } else {
      const double depth = rng.nextValue() * segment.distInsideObject;
      scatterPos = segment.entryPoint + direc * depth;
      l1 += depth;
      break;
    }
  }

  // Now track to final destination
  V3D scatteredDirec = endPos - scatterPos;
  scatteredDirec.normalize();
  Track path2(scatterPos, scatteredDirec);
  m_sample.interceptSurface(path2);
  double l2(0.0);
  for (const auto &segment : path2) {
    l2 += segment.distInsideObject;
  }

  const auto &material = m_sample.material();
  const double rho = material.numberDensity() * 100;
  const double sigmaTot1 = material.totalScatterXSection(lambdaBefore) +
                           material.absorbXSection(lambdaBefore);
  const double sigmaTot2 = material.totalScatterXSection(lambdaAfter) +
                           material.absorbXSection(lambdaAfter);
  return exp(-rho * (sigmaTot1 * l1 + sigmaTot2 * l2));
}

} // namespace Algorithms
} // namespace Mantid
