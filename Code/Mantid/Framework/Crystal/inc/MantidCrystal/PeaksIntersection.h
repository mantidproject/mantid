#ifndef MANTID_CRYSTAL_PEAKSINTERSECTION_H_
#define MANTID_CRYSTAL_PEAKSINTERSECTION_H_

#include "MantidKernel/System.h"
#include "MantidKernel/V3D.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid {
namespace Crystal {

typedef std::vector<Mantid::Kernel::V3D> VecV3D;
typedef std::vector<VecV3D> VecVecV3D;

/** PeaksIntersection : Abstract base algorithm class for algorithms that
  identify peaks interacting with one or more surfaces
  i.e. a flat surface or a box made out of flat surfaces.

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
class DLLExport PeaksIntersection : public API::Algorithm {
public:
  PeaksIntersection();
  virtual ~PeaksIntersection();

  static std::string detectorSpaceFrame();
  static std::string qLabFrame();
  static std::string qSampleFrame();
  static std::string hklFrame();
  /// Number of surface faces that make up this object.
  virtual int numberOfFaces() const = 0;

protected:
  /// Initalize the common properties.
  void initBaseProperties();

  /// Run the algorithm.
  void executePeaksIntersection(const bool checkExtents = true);

  /// Get the peak radius.
  double getPeakRadius() const;

private:
  /// Validate the input extents.
  virtual void validateExtentsInput() const = 0;
  /// Create all faces.
  virtual VecVecV3D createFaces() const = 0;
  /// Check that a point is outside any of the extents
  virtual bool
  pointOutsideAnyExtents(const Mantid::Kernel::V3D &testPoint) const = 0;
  /// Check that a point is inside ALL of the extents
  virtual bool
  pointInsideAllExtents(const Mantid::Kernel::V3D &testPoints,
                        const Mantid::Kernel::V3D &peakCenter) const = 0;

  /// Verfifies that the normals have been set up correctly such that the touch
  /// point falls onto the plane. Use for debugging.
  virtual void checkTouchPoint(const Mantid::Kernel::V3D &touchPoint,
                               const Mantid::Kernel::V3D &normal,
                               const Mantid::Kernel::V3D &faceVertex) const = 0;

  // The peak radius.
  double m_peakRadius;
};

} // namespace Crystal
} // namespace Mantid

#endif /* MANTID_CRYSTAL_PEAKSINTERSECTION_H_ */