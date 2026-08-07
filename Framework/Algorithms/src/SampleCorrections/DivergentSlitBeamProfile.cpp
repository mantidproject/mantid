// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/SampleCorrections/DivergentSlitBeamProfile.h"
#include "MantidAPI/Sample.h"
#include "MantidAlgorithms/SampleCorrections/RectangularBeamProfile.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidKernel/PseudoRandomNumberGenerator.h"
#include "MantidKernel/V3D.h"
#include <cmath>

namespace Mantid {
using Kernel::V3D;
namespace Algorithms {

/**
 * Construct a beam profile.
 * @param frame Defines the direction of the beam, up and horizontal
 * @param center V3D defining the central point of the rectangle
 * @param width Width of beam
 * @param height Height of beam
 * @param horDiv Horizontal Component of Beam Divergence
 * @param verDiv Vertical Component of Beam Divergence
 * @param slitD Slit Distance from the Instrument origin
 */
DivergentSlitBeamProfile::DivergentSlitBeamProfile(const Geometry::ReferenceFrame &frame, const Kernel::V3D &center,
                                                   double width, double height, double horDiv, double verDiv,
                                                   double slitD)
    : RectangularBeamProfile(frame, center, width, height), m_horDiv(horDiv), m_verDiv(verDiv), m_slitD(slitD) {
  m_min[m_upIdx] = center[m_upIdx] - 0.5 * height;
  m_min[m_horIdx] = center[m_horIdx] - 0.5 * width;
  m_min[m_beamIdx] = center[m_beamIdx];
  m_beamDir[m_beamIdx] = 1.0;
}

namespace {
/**
 * Fraction of the incident beam that reaches a point offset from an aperture centre along a given axis
 * Eq. 14 of Creek, Santisteban & Edwards (2005)
 *
 * The paper normalises to unit integral (a 1/s prefactor, giving a probability density).
 * Here we normalise instead so a fully illuminated point returns 1.0, matching the existing
 * value when divergence is not accounted for.
 */
double apertureTransmission(const double axCoord, const double axSlitCentre, const double slitWidth,
                            const double sigma) {
  const double halfSlitWidth = 0.5 * slitWidth;
  const double offset = axCoord - axSlitCentre;
  if (sigma <= 0.0) {
    // Zero divergence, just return the top hat function of the slit (1 inside, 0 outside)
    return std::abs(offset) <= halfSlitWidth ? 1.0 : 0.0;
  }
  constexpr double SQRT_2 = 1.4142135623730951;
  const double invNorm = 1.0 / (sigma * SQRT_2);
  return 0.5 * (std::erf((offset + halfSlitWidth) * invNorm) - std::erf((offset - halfSlitWidth) * invNorm));
}
} // namespace

/**
 * Calculate the weighted intensity for the given point from the incident beam resolution function.
 * Eq. 13 and 14 of
 * Creek, S. & Santisteban, Javier & Edwards, Lyndon. (2005).
 * Modelling pseudo-strain effects induced in strain measurement using time-of-flight neutron diffraction.
 * @param scatteringPoint Point to calculate intensity at, lab frame
 */
double DivergentSlitBeamProfile::intensityAt(const Kernel::V3D &scatteringPoint) const {
  // Eq. 13: the Gaussian width grows linearly with distance travelled from the defining slit.
  // m_slitD is the slit's distance upstream of the instrument origin, so a point at beam
  // coordinate z lies (z + m_slitD) downstream of the slit.
  const double distFromSlit = scatteringPoint[m_beamIdx] + m_slitD;
  if (distFromSlit <= 0.0) {
    return 0.0; // upstream of the slit, so not illuminated
  }
  const double horSigma = distFromSlit * std::tan(m_horDiv);
  const double verSigma = distFromSlit * std::tan(m_verDiv);

  return apertureTransmission(scatteringPoint[m_horIdx], m_beamCenter[m_horIdx], m_width, horSigma) *
         apertureTransmission(scatteringPoint[m_upIdx], m_beamCenter[m_upIdx], m_height, verSigma);
}

} // namespace Algorithms
} // namespace Mantid
