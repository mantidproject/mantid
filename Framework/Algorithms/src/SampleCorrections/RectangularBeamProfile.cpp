#include "MantidAlgorithms/SampleCorrections/RectangularBeamProfile.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/PseudoRandomNumberGenerator.h"
#include "MantidKernel/V3D.h"

namespace Mantid {

namespace Algorithms {

/**
 * Construct a beam profile.
 * @param frame Defines the direction of the beam, up and horizontal
 * @param center V3D defining the central point of the rectangle
 * @param width Width of beam
 * @param height Height of beam
 */
RectangularBeamProfile::RectangularBeamProfile(
    const Geometry::ReferenceFrame &frame, const Kernel::V3D &center,
    double width, double height)
    : IBeamProfile(), m_upIdx(frame.pointingUp()),
      m_beamIdx(frame.pointingAlongBeam()),
      m_horIdx(frame.pointingHorizontal()), m_width(width), m_height(height),
      m_min(), m_beamDir() {
  m_min[m_upIdx] = center[m_upIdx] - 0.5 * height;
  m_min[m_horIdx] = center[m_horIdx] - 0.5 * width;
  m_min[m_beamIdx] = center[m_beamIdx];
  m_beamDir[m_beamIdx] = 1.0;
}

/**
 * Generate a random point within the beam profile using the supplied random
 * number source
 * @param rng A reference to a random number generator
 * @return An IBeamProfile::Ray describing the start and direction
 */
IBeamProfile::Ray RectangularBeamProfile::generatePoint(
    Kernel::PseudoRandomNumberGenerator &rng) const {
  using Kernel::V3D;
  V3D pt;
  pt[m_upIdx] = m_min[m_upIdx] + rng.nextValue() * m_height;
  pt[m_horIdx] = m_min[m_horIdx] + rng.nextValue() * m_width;
  pt[m_beamIdx] = m_min[m_beamIdx];
  return {pt, m_beamDir};
}

} // namespace Algorithms
} // namespace Mantid
