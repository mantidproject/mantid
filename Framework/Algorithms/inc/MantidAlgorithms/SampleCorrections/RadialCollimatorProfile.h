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
  Acceptance of a radial collimator as a spatial profile, i.e. the detector resolution function P_d(r)
  of Creek, Santisteban & Edwards (2005) eq. 9. Designed specifically for ENGIN-X collimators but may be
  more broadly applicable.

  The ENGIN-X radial collimator restricts the extent of the gauge volume in the direction transverse to the
  line joining the focal point to the detector, in the horizontal plane. For a bank at 2-theta = 90
  degrees that direction is the incident beam. For the collimators the acceptance magnitude is essentially detector
  independent - the direction it restricts is not, since it rotates with the detector.

  Note that the collimator's angular divergence does not enter here. On ENGIN-X the spatial width and the
  divergence are decoupled by design: every collimator shares the same divergence and they differ only in
  the distance of the blades from the focal point, which is what sets the gauge width
  (Santisteban et al., J. Appl. Cryst. 39 (2006) 812, section 2.3).

  The profile is taken to be Gaussian, with a FWHM equal to the collimator's calibrated gauge width. This
  is the profile measured directly on ENGIN-X by scanning a thin nylon thread through the beam, which was
  fitted by Gaussians of 2.05 mm and 3.95 mm FWHM for the nominal 2 mm and 4 mm collimators (ibid., Fig. 6b).

  The profile is taken to be flat along the viewing axis and vertically. That is only valid while some other
  term of the spatial resolution function - the incident beam profile or an explicit gauge volume - bounds
  those directions; in reality P_d falls away from the focal point along the viewing axis as well
  (Creek et al., Figs. 4.1-4.6).
*/
class MANTID_ALGORITHMS_DLL RadialCollimatorProfile {
public:
  /// @param gaugeWidthFWHM Calibrated full width at half maximum of the collimator profile, in metres
  /// @param upDirection The instrument's vertical, used to find the restricted horizontal direction
  RadialCollimatorProfile(const double gaugeWidthFWHM, const Kernel::V3D &upDirection);

  /// Relative acceptance, (between 0 and 1) of a neutron scattered at scatterPoint toward a detector at
  /// detectorPos. All positions are in the lab frame.
  ///
  /// @param focalPoint The collimator's focal point, about which the profile is centred. This is fixed in
  /// the laboratory - the sample is translated through it - so it must not be made to track the sample.
  double intensityAt(const Kernel::V3D &scatterPoint, const Kernel::V3D &focalPoint,
                     const Kernel::V3D &detectorPos) const;

  double gaugeWidthFWHM() const { return m_fwhm; }

private:
  const double m_fwhm;
  const double m_sigma;
  const Kernel::V3D m_upDirection;
};

} // namespace Algorithms
} // namespace Mantid
