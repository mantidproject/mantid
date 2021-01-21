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

/** PeaksOnSurface : Check peak workspace interaction with a single surface. Any
  peaks whos extents intersect the plane are identified.
*/
class MANTID_CRYSTAL_DLL PeaksOnSurface : public PeaksIntersection {
public:
  PeaksOnSurface();

  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override { return "Find peaks intersecting a single surface region."; }

  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"PeaksInRegion"}; }
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

  Mantid::Kernel::V3D m_vertex1; // lower left
  Mantid::Kernel::V3D m_vertex2; // upper left
  Mantid::Kernel::V3D m_vertex3; // upper right
  Mantid::Kernel::V3D m_vertex4; // lower right

  // Lines used in bounary calculations.
  Mantid::Kernel::V3D m_line1;
  Mantid::Kernel::V3D m_line2;
  Mantid::Kernel::V3D m_line3;
  Mantid::Kernel::V3D m_line4;
};

/// Non-member helper function
bool MANTID_CRYSTAL_DLL lineIntersectsSphere(const Mantid::Kernel::V3D &line, const Mantid::Kernel::V3D &lineStart,
                                             const Mantid::Kernel::V3D &peakCenter, const double peakRadius);

} // namespace Crystal
} // namespace Mantid
