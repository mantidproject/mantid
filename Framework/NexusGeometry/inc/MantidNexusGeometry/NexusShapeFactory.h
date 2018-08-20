#ifndef MANTIDNEXUSGEOMETRY_SHAPEFACTORY_H
#define MANTIDNEXUSGEOMETRY_SHAPEFACTORY_H

#include "MantidNexusGeometry/DllConfig.h"

#include "Eigen/Core"
#include "Eigen/Geometry"
#include <map>
#include <memory>
#include <vector>

namespace Mantid {
namespace Kernel {
class V3D;
}
namespace Geometry {
class IObject;
}
namespace NexusGeometry {

/** NexusShapeFactory : Namespace containing free factory functions for creating
  IObjects (Shapes) from Nexus Geometry related formats

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
namespace NexusShapeFactory {

/// Creates a cylindrical shape
DLLExport std::unique_ptr<const Geometry::IObject>
createCylinder(const Eigen::Matrix<double, 3, 3> &pointsDef);

/// Creates a triangular mesh shape based on ready triangulated polygons
DLLExport std::unique_ptr<const Geometry::IObject>
createMesh(std::vector<uint16_t> &&triangularFaces,
           std::vector<Mantid::Kernel::V3D> &&vertices);

/// Creates a triangular mesh shape based on OFF polygon inputs
DLLExport std::unique_ptr<const Geometry::IObject>
createFromOFFMesh(const std::vector<uint16_t> &faceIndices,
                  const std::vector<uint16_t> &windingOrder,
                  const std::vector<float> &nexusVertices);

DLLExport std::unique_ptr<const Geometry::IObject>
createFromOFFMesh(const std::vector<uint16_t> &faceIndices,
                  const std::vector<uint16_t> &windingOrder,
                  const std::vector<Eigen::Vector3d> &nexusVertices);
} // namespace NexusShapeFactory
} // namespace NexusGeometry
} // namespace Mantid
#endif // MANTIDNEXUSGEOMETRY_SHAPEFACTORY_H
