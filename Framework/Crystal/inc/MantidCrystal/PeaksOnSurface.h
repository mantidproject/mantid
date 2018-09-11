#ifndef MANTID_CRYSTAL_PEAKSONSURFACE_H_
#define MANTID_CRYSTAL_PEAKSONSURFACE_H_

#include "MantidCrystal/PeaksIntersection.h"

namespace Mantid {
namespace Crystal {

/** PeaksOnSurface : Check peak workspace interaction with a single surface. Any
  peaks whos extents intersect the plane are identified.

  Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport PeaksOnSurface : public PeaksIntersection {
public:
  PeaksOnSurface();

  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Find peaks intersecting a single surface region.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"PeaksInRegion"};
  }
  const std::string category() const override;

private:
  void init() override;
  void exec() override;

  // Overriden base class methods.
  void validateExtentsInput() const override;
  int numberOfFaces() const override;
  VecVecV3D createFaces() const override;
  bool
  pointOutsideAnyExtents(const Mantid::Kernel::V3D &testPoint) const override;
  bool
  pointInsideAllExtents(const Mantid::Kernel::V3D &testPoint,
                        const Mantid::Kernel::V3D &peakCenter) const override;
  void checkTouchPoint(const Mantid::Kernel::V3D &touchPoint,
                       const Mantid::Kernel::V3D &normal,
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
bool DLLExport lineIntersectsSphere(const Mantid::Kernel::V3D &line,
                                    const Mantid::Kernel::V3D &lineStart,
                                    const Mantid::Kernel::V3D &peakCenter,
                                    const double peakRadius);

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_PEAKSONSURFACE_H_ */