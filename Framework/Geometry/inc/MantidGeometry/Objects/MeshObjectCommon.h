// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidKernel/V3D.h"
#include <vector>

namespace Mantid {
namespace Geometry {
class BoundingBox;
/** MeshObjectCommon : Performs functions common to 3D and 2D closed meshes
 */
namespace MeshObjectCommon {

MANTID_GEOMETRY_DLL std::vector<double> getVertices(const std::vector<Kernel::V3D> &vertices);

MANTID_GEOMETRY_DLL bool isOnTriangle(const Kernel::V3D &point, const Kernel::V3D &v1, const Kernel::V3D &v2,
                                      const Kernel::V3D &v3);
MANTID_GEOMETRY_DLL bool rayIntersectsTriangle(const Kernel::V3D &start, const Kernel::V3D &direction,
                                               const Kernel::V3D &v1, const Kernel::V3D &v2, const Kernel::V3D &v3,
                                               Kernel::V3D &intersection, TrackDirection &entryExit);

MANTID_GEOMETRY_DLL void checkVertexLimit(size_t nVertices);
MANTID_GEOMETRY_DLL const BoundingBox &getBoundingBox(const std::vector<Kernel::V3D> &vertices, BoundingBox &cacheBB);
MANTID_GEOMETRY_DLL void getBoundingBox(const std::vector<Kernel::V3D> &vertices, BoundingBox &cacheBB, double &xmax,
                                        double &ymax, double &zmax, double &xmin, double &ymin, double &zmin);
MANTID_GEOMETRY_DLL double getTriangleSolidAngle(const Kernel::V3D &a, const Kernel::V3D &b, const Kernel::V3D &c,
                                                 const Kernel::V3D &observer);
} // namespace MeshObjectCommon

} // namespace Geometry
} // namespace Mantid
