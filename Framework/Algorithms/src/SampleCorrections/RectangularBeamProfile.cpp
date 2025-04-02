// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/SampleCorrections/RectangularBeamProfile.h"
#include "MantidAPI/Sample.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidKernel/PseudoRandomNumberGenerator.h"
#include "MantidKernel/V3D.h"

namespace Mantid {
using Kernel::V3D;
namespace Algorithms {

/**
 * Construct a beam profile.
 * @param frame Defines the direction of the beam, up and horizontal
 * @param center V3D defining the central point of the rectangle
 * @param width Width of beam
 * @param height Height of beam
 */
RectangularBeamProfile::RectangularBeamProfile(const Geometry::ReferenceFrame &frame, const Kernel::V3D &center,
                                               double width, double height)
    : IBeamProfile(center), m_upIdx(frame.pointingUp()), m_beamIdx(frame.pointingAlongBeam()),
      m_horIdx(frame.pointingHorizontal()), m_width(width), m_height(height), m_min(), m_beamDir() {
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
IBeamProfile::Ray RectangularBeamProfile::generatePoint(Kernel::PseudoRandomNumberGenerator &rng) const {
  V3D pt;
  pt[m_upIdx] = m_min[m_upIdx] + rng.nextValue() * m_height;
  pt[m_horIdx] = m_min[m_horIdx] + rng.nextValue() * m_width;
  pt[m_beamIdx] = m_min[m_beamIdx];
  return {pt, m_beamDir};
}

/**
 * Generate a random point on the profile that is within the given bounding
 * area. If the point is outside the area then it is pulled to the boundary of
 * the bounding area.
 * @param rng A reference to a random number generator
 * @param bounds A reference to the bounding area that defines the maximum
 * allowed region for the generated point.
 * @return An IBeamProfile::Ray describing the start and direction
 */
IBeamProfile::Ray RectangularBeamProfile::generatePoint(Kernel::PseudoRandomNumberGenerator &rng,
                                                        const Geometry::BoundingBox &bounds) const {
  auto rngRay = generatePoint(rng);
  auto &rngPt = rngRay.startPos;
  const V3D minBound(bounds.minPoint()), maxBound(bounds.maxPoint());
  if (rngPt[m_upIdx] > maxBound[m_upIdx])
    rngPt[m_upIdx] = maxBound[m_upIdx];
  else if (rngPt[m_upIdx] < minBound[m_upIdx])
    rngPt[m_upIdx] = minBound[m_upIdx];

  if (rngPt[m_horIdx] > maxBound[m_horIdx])
    rngPt[m_horIdx] = maxBound[m_horIdx];
  else if (rngPt[m_horIdx] < minBound[m_horIdx])
    rngPt[m_horIdx] = minBound[m_horIdx];
  return rngRay;
}

/**
 * Compute a region that defines how the beam illuminates the given sample/can
 * @param sampleBox A reference to the bounding box of the sample
 * @return A BoundingBox defining the active region
 */
Geometry::BoundingBox RectangularBeamProfile::defineActiveRegion(const Geometry::BoundingBox &sampleBox) const {
  // In the beam direction use the maximum sample extent other wise restrict
  // the active region to the width/height of beam
  const auto &sampleMin(sampleBox.minPoint());
  const auto &sampleMax(sampleBox.maxPoint());
  V3D minPt, maxPt;
  minPt[m_horIdx] = std::max(sampleMin[m_horIdx], m_min[m_horIdx]);
  maxPt[m_horIdx] = std::min(sampleMax[m_horIdx], m_min[m_horIdx] + m_width);
  minPt[m_upIdx] = std::max(sampleMin[m_upIdx], m_min[m_upIdx]);
  maxPt[m_upIdx] = std::min(sampleMax[m_upIdx], m_min[m_upIdx] + m_height);
  minPt[m_beamIdx] = sampleMin[m_beamIdx];
  maxPt[m_beamIdx] = sampleMax[m_beamIdx];
  printf("minPt: %f, %f, %f\n", minPt.X(), minPt.Y(), minPt.Z());
  printf("maxPt: %f, %f, %f\n", maxPt.X(), maxPt.Y(), maxPt.Z());
  printf("idx: %d, %d, %d\n", m_horIdx, m_upIdx, m_beamIdx);

  return Geometry::BoundingBox(maxPt.X(), maxPt.Y(), maxPt.Z(), minPt.X(), minPt.Y(), minPt.Z());
}

} // namespace Algorithms
} // namespace Mantid
