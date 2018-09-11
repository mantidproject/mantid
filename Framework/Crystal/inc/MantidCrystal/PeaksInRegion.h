#ifndef MANTID_CRYSTAL_PEAKSINREGION_H_
#define MANTID_CRYSTAL_PEAKSINREGION_H_

#include "MantidCrystal/PeaksIntersection.h"

namespace Mantid {
namespace Crystal {

/** PeaksInRegion : Find peaks that are either inside a box region, or that have
  a radius of sufficent size, that they intersect the box.

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
class DLLExport PeaksInRegion : public PeaksIntersection {
public:
  const std::string name() const override;
  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Find peaks intersecting a box region.";
  }

  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"PeaksOnSurface"};
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
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_PEAKSINREGION_H_ */
