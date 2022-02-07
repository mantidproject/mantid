// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAlgorithms/DllConfig.h"
#include "MantidAlgorithms/SampleCorrections/IBeamProfile.h"
#include "MantidKernel/V3D.h"

#include <array>

namespace Mantid {
namespace Geometry {
class ReferenceFrame;
}
namespace Algorithms {

/**
  Defines a flat, circular beam profile that has a radius and center
  point. The profile is assumed infinitely thin.
*/
class MANTID_ALGORITHMS_DLL CircularBeamProfile final : public IBeamProfile {
public:
  CircularBeamProfile(const Geometry::ReferenceFrame &frame, const Kernel::V3D &center, double radius);

  IBeamProfile::Ray generatePoint(Kernel::PseudoRandomNumberGenerator &rng) const override;
  IBeamProfile::Ray generatePoint(Kernel::PseudoRandomNumberGenerator &rng,
                                  const Geometry::BoundingBox &bounds) const override;
  Geometry::BoundingBox defineActiveRegion(const Geometry::BoundingBox &sampleBox) const override;

private:
  const unsigned short m_upIdx;
  const unsigned short m_beamIdx;
  const unsigned short m_horIdx;
  const double m_radius;
  std::array<double, 3> m_min;
  Kernel::V3D m_center;
  Kernel::V3D m_beamDir;
};

} // namespace Algorithms
} // namespace Mantid
