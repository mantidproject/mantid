// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
*/
namespace NexusShapeFactory {

/// Creates a cylindrical shape
DLLExport std::unique_ptr<const Geometry::IObject>
createCylinder(const Eigen::Matrix<double, 3, 3> &pointsDef);

/// Creates a triangular mesh shape based on ready triangulated polygons
DLLExport std::unique_ptr<const Geometry::IObject>
createMesh(std::vector<uint32_t> &&triangularFaces,
           std::vector<Mantid::Kernel::V3D> &&vertices);

/// Creates a triangular mesh shape based on OFF (Object File Format) polygon
/// inputs https://en.wikipedia.org/wiki/OFF_(file_format)
DLLExport std::unique_ptr<const Geometry::IObject>
createFromOFFMesh(const std::vector<uint32_t> &faceIndices,
                  const std::vector<uint32_t> &windingOrder,
                  const std::vector<float> &nexusVertices);

DLLExport std::unique_ptr<const Geometry::IObject>
createFromOFFMesh(const std::vector<uint32_t> &faceIndices,
                  const std::vector<uint32_t> &windingOrder,
                  const std::vector<Eigen::Vector3d> &nexusVertices);
} // namespace NexusShapeFactory
} // namespace NexusGeometry
} // namespace Mantid
#endif // MANTIDNEXUSGEOMETRY_SHAPEFACTORY_H
