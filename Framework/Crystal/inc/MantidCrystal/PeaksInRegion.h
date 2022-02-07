// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidCrystal/PeaksIntersection.h"

namespace Mantid {
namespace Crystal {

/** PeaksInRegion : Find peaks that are either inside a box region, or that have
  a radius of sufficent size, that they intersect the box.
*/
class MANTID_CRYSTAL_DLL PeaksInRegion : public PeaksIntersection {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Find peaks intersecting a box region."; }

  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"PeaksOnSurface"}; }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;

  // Overriden base class methods.
  void validateExtentsInput() const override;
  int numberOfFaces() const override;
  VecVecV3D createFaces() const override;
  bool pointOutsideAnyExtents(const Mantid::Kernel::V3D &testPoint) const override;
  bool pointInsideAllExtents(const Mantid::Kernel::V3D &testPoint,
                             const Mantid::Kernel::V3D &peakCenter) const override;
  void checkTouchPoint(const Mantid::Kernel::V3D &touchPoint, const Mantid::Kernel::V3D &normal,
                       const Mantid::Kernel::V3D &faceVertex) const override;

  /// Extents.
  std::vector<double> m_extents;
};

} // namespace Crystal
} // namespace Mantid
