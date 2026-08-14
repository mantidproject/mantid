// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAlgorithms/DllConfig.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
namespace Algorithms {

/**
  Acceptance of a radial collimator as a spatial profile. Designed specifically for ENGIN-X collimators
  but may be more broadly applicable.

  The ENGIN-X radial collimator restricts the extent of the gauge volume in the direction transverse to the
  line joining the sample to the detector, in the horizontal plane. For a bank at 2-theta = 90
  degrees that direction is the incident beam. For the collimators the acceptance magnitude is essentially detector
  independent - the direction it restricts is not, since it rotates with the detector.

  The profile is taken to be Gaussian, with a FWHM equal to the collimator's calibrated gauge width.
*/
class MANTID_ALGORITHMS_DLL RadialCollimatorProfile {
public:
  /// @param gaugeWidthFWHM Calibrated full width at half maximum of the collimator profile, in metres
  /// @param upDirection The instrument's vertical, used to find the restricted horizontal direction
  RadialCollimatorProfile(const double gaugeWidthFWHM, const Kernel::V3D &upDirection);

  /// Relative acceptance, (between 0 and 1) of a neutron scattered at scatterPoint toward a detector at
  /// detectorPos. All positions are in the lab frame.
  double intensityAt(const Kernel::V3D &scatterPoint, const Kernel::V3D &samplePos,
                     const Kernel::V3D &detectorPos) const;

  double gaugeWidthFWHM() const { return m_fwhm; }

private:
  double m_fwhm;
  double m_sigma;
  Kernel::V3D m_upDirection;
};

} // namespace Algorithms
} // namespace Mantid
