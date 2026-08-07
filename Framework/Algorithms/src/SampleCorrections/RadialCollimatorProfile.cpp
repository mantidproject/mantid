// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/SampleCorrections/RadialCollimatorProfile.h"

#include <cmath>
#include <stdexcept>

namespace Mantid {
using Kernel::V3D;

namespace {
/// Ratio between the full width at half maximum and the standard deviation of a Gaussian.
constexpr double FWHM_TO_SIGMA = 2.354820045030949;
} // namespace

namespace Algorithms {

RadialCollimatorProfile::RadialCollimatorProfile(const double gaugeWidthFWHM, const Kernel::V3D &upDirection)
    : m_fwhm(gaugeWidthFWHM), m_sigma(gaugeWidthFWHM / FWHM_TO_SIGMA), m_upDirection(upDirection) {
  if (gaugeWidthFWHM <= 0.0) {
    throw std::invalid_argument("Collimator gauge width must be positive");
  }
  if (m_upDirection.norm2() == 0.0) {
    throw std::invalid_argument("Collimator up direction cannot be a zero vector");
  }
  m_upDirection.normalize();
}

double RadialCollimatorProfile::intensityAt(const Kernel::V3D &scatterPoint, const Kernel::V3D &samplePos,
                                            const Kernel::V3D &detectorPos) const {
  auto viewingAxis = detectorPos - samplePos;
  if (viewingAxis.norm2() == 0.0) {
    // Degenerate geometry - no direction to restrict, so impose no restriction.
    return 1.0;
  }
  viewingAxis.normalize();

  // The collimator restricts the direction transverse to the viewing axis, in the horizontal plane.
  auto restricted = viewingAxis.cross_prod(m_upDirection);
  if (restricted.norm2() == 0.0) {
    // The detector sits directly above or below the sample, so there is no horizontal transverse
    // direction to restrict.
    return 1.0;
  }
  restricted.normalize();

  const double offset = (scatterPoint - samplePos).scalar_prod(restricted);
  return std::exp(-0.5 * offset * offset / (m_sigma * m_sigma));
}

} // namespace Algorithms
} // namespace Mantid
