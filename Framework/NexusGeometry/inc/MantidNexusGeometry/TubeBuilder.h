#ifndef MANTIDNEXUSGEOMETRY_TUBEBUILDER_H
#define MANTIDNEXUSGEOMETRY_TUBEBUILDER_H
#include "MantidNexusGeometry/DllConfig.h"
#include <Eigen/Core>
#include <Eigen/Geometry>
#include <boost/shared_ptr.hpp>
#include <vector>

namespace Mantid {
namespace Geometry {
class IObject;
}

namespace NexusGeometry {
namespace detail {

/** TubeBuilder : Builder for wrapping the creation of a tube as a collection of
Colinear detectors with cylindrical shape.

Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_NEXUSGEOMETRY_DLL TubeBuilder {
public:
  TubeBuilder(const Mantid::Geometry::IObject &pixelShape,
              Eigen::Vector3d firstDetectorPosition, int firstDetectorId);
  ~TubeBuilder();

  const Eigen::Vector3d &tubePosition() const;
  const std::vector<Eigen::Vector3d> &detPositions() const;
  const std::vector<int> &detIDs() const;
  boost::shared_ptr<const Mantid::Geometry::IObject> shape() const;
  double tubeHeight() const;
  double tubeRadius() const;
  bool addDetectorIfCoLinear(const Eigen::Vector3d &pos, int detID);
  size_t size() const;

private:
  Eigen::Vector3d m_axis;
  double m_pixelHeight;
  double m_tubeHeight;
  Eigen::Vector3d m_halfHeightVec; // stored for faster calculations
  Eigen::Vector3d m_baseVec;       // stored for faster calculations
  double m_pixelRadius;
  std::vector<Eigen::Vector3d> m_positions;
  std::vector<int> m_detIDs;
  Eigen::Vector3d
      m_p1; ///< First point which defines the line in space the tube lies along
  Eigen::Vector3d m_p2; ///< Second point which defines the line in space the
                        ///< tube lies along

private:
  bool checkCoLinear(const Eigen::Vector3d &pos) const;
};
} // namespace detail
} // namespace NexusGeometry
} // namespace Mantid

#endif // MANTIDNEXUSGEOMETRY_TUBEBUILDER_H
