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
/// Offsets beyond this many standard deviations are treated as completely rejected. The Gaussian is
/// ~4e-6 of its peak here, so this changes no centroid meaningfully, but it lets callers skip
/// elements the collimator cannot see rather than accumulating negligible weights from them.
constexpr double CUTOFF_SIGMAS = 5.0;

double validatedSigma(const double gaugeWidthFWHM) {
  if (gaugeWidthFWHM <= 0.0) {
    throw std::invalid_argument("Collimator gauge width must be positive");
  }
  return gaugeWidthFWHM / FWHM_TO_SIGMA;
}

V3D validatedUpDirection(V3D upDirection) {
  if (upDirection.norm2() == 0.0) {
    throw std::invalid_argument("Collimator up direction cannot be a zero vector");
  }
  upDirection.normalize();
  return upDirection;
}
} // namespace

namespace Algorithms {

RadialCollimatorProfile::RadialCollimatorProfile(const double gaugeWidthFWHM, const Kernel::V3D &upDirection)
    : m_fwhm(gaugeWidthFWHM), m_sigma(validatedSigma(gaugeWidthFWHM)),
      m_upDirection(validatedUpDirection(upDirection)) {}

double RadialCollimatorProfile::intensityAt(const Kernel::V3D &scatterPoint, const Kernel::V3D &focalPoint,
                                            const Kernel::V3D &detectorPos) const {
  auto viewingAxis = detectorPos - focalPoint;
  if (viewingAxis.norm2() == 0.0) {
    // Degenerate geometry - no direction to restrict, so impose no restriction.
    return 1.0;
  }
  viewingAxis.normalize();

  // The collimator restricts the direction transverse to the viewing axis, in the horizontal plane.
  // The blades are vertical, so the restricted direction is horizontal regardless of how high in the
  // bank the detector sits - the cross product with the vertical delivers exactly that.
  auto restricted = viewingAxis.cross_prod(m_upDirection);
  if (restricted.norm2() == 0.0) {
    // The detector sits directly above or below the focal point, so there is no horizontal transverse
    // direction to restrict.
    return 1.0;
  }
  restricted.normalize();

  const double offset = (scatterPoint - focalPoint).scalar_prod(restricted);
  if (std::abs(offset) > CUTOFF_SIGMAS * m_sigma) {
    return 0.0;
  }
  return std::exp(-0.5 * offset * offset / (m_sigma * m_sigma));
}

} // namespace Algorithms
} // namespace Mantid
