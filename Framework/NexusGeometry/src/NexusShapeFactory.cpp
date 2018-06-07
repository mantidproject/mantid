#include "MantidNexusGeometry/NexusShapeFactory.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidGeometry/Objects/Rules.h"
#include "MantidGeometry/Surfaces/Cylinder.h"
#include "MantidGeometry/Surfaces/Plane.h"
#include "MantidGeometry/Surfaces/Surface.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/EigenConversionHelpers.h"
#include "MantidKernel/make_unique.h"

#include <boost/make_shared.hpp>

namespace Mantid {
namespace NexusGeometry {
namespace NexusShapeFactory {
using namespace Eigen;

namespace {

/// Finalise shape
std::unique_ptr<const Geometry::IObject>
createShape(const std::map<int, boost::shared_ptr<Geometry::Surface>> &surfaces,
            const std::string &algebra, std::vector<double> &boundingBox) {
  auto shape = Mantid::Kernel::make_unique<Geometry::CSGObject>();
  shape->setObject(21, algebra);
  shape->populate(surfaces);
  // Boundingbox x,y,z:max; x,y,z:min
  shape->defineBoundingBox(boundingBox[0], boundingBox[2], boundingBox[4],
                           boundingBox[1], boundingBox[3], boundingBox[5]);
  return Mantid::Kernel::make_unique<const Geometry::CSGObject>(*shape);
}

void createTrianglesFromPolygon(const std::vector<uint16_t> &windingOrder,
                                std::vector<uint16_t> &triangularFaces,
                                int &startOfFace, int &endOfFace) {
  int polygonOrder = endOfFace - startOfFace;
  auto first = windingOrder.begin() + startOfFace;

  for (int polygonVertex = 1; polygonVertex < polygonOrder - 1;
       ++polygonVertex) {
    triangularFaces.push_back(*first);
    triangularFaces.push_back(*(first + polygonVertex));
    triangularFaces.push_back(*(first + polygonVertex + 1));
  }
  startOfFace = endOfFace; // start of the next face
}

std::vector<uint16_t>
createTriangularFaces(const std::vector<uint16_t> &faceIndices,
                      const std::vector<uint16_t> &windingOrder) {

  // Elements 0 to 2 are the indices of the vertices vector corresponding to the
  // vertices of the first triangle.
  // Elements 3 to 5 are for the second triangle, and so on.
  // The order of the vertices is the winding order of the triangle, determining
  // the face normal by right-hand rule
  std::vector<uint16_t> triangularFaces;

  int startOfFace = 0;
  int endOfFace = 0;
  for (auto it = faceIndices.begin() + 1; it != faceIndices.end(); ++it) {
    endOfFace = *it;
    createTrianglesFromPolygon(windingOrder, triangularFaces, startOfFace,
                               endOfFace);
  }

  // and the last face
  endOfFace = static_cast<int>(windingOrder.size());
  createTrianglesFromPolygon(windingOrder, triangularFaces, startOfFace,
                             endOfFace);

  return triangularFaces;
}

} // namespace

bool pointsCoplanar(const std::vector<Mantid::Kernel::V3D> &vertices) {
  if (vertices.size() < 3)
    return false; // Not a plane with < 3 points

  auto v0 = vertices[1] - vertices[0];

  Mantid::Kernel::V3D normal;
  // Look for normal amongst first 3 non coplanar points
  auto v1 = vertices[2] - vertices[1];
  for (size_t i = 1; i < vertices.size() - 1; ++i) {
    v1 = vertices[i + 1] - vertices[i];
    normal = v0.cross_prod(v1);
    if (normal.norm2() != 0) {
      break;
    }
  }
  if (normal.norm2() == 0) {
    // If all points are colinear. Not a plane.
    return false;
  }

  bool in_plane = true;
  const auto nx = normal[0];
  const auto ny = normal[1];
  const auto nz = normal[2];
  auto denom = normal.norm2();
  auto k = nx * v0.X() + ny * v0.Y() + nz * v0.Z();

  for (size_t i = 0; i < vertices.size(); ++i) {
    auto d = (nx * vertices[i].X() + ny * vertices[i].Y() +
              nz * vertices[i].Z() - k) /
             denom;
    if (d != 0) {
      in_plane = false;
      break;
    }
  }
  return in_plane;
}
std::unique_ptr<const Geometry::IObject>
createCylinder(const Eigen::Matrix<double, 3, 3> &pointsDef) {
  // Calculate cylinder parameters
  auto centre = (pointsDef.col(2) + pointsDef.col(0)) / 2;
  auto axisVector = pointsDef.col(2) - pointsDef.col(0);
  auto axisUnitVector = axisVector.normalized();
  auto radiusVector = pointsDef.col(1) - pointsDef.col(0);
  double radius = radiusVector.norm();
  Kernel::V3D normVec =
      Kernel::V3D(axisUnitVector(0), axisUnitVector(1), axisUnitVector(2));

  auto cylinder = boost::make_shared<Geometry::Cylinder>();
  cylinder->setCentre(Kernel::V3D(centre(0), centre(1), centre(2)));
  cylinder->setNorm(normVec);
  cylinder->setRadius(radius);

  // Top plane
  auto planeTop = boost::make_shared<Geometry::Plane>();
  planeTop->setPlane(
      Kernel::V3D(pointsDef(0, 2), pointsDef(1, 2), pointsDef(2, 2)), normVec);

  // Bottom plane
  auto planeBottom = boost::make_shared<Geometry::Plane>();
  planeBottom->setPlane(
      Kernel::V3D(pointsDef(0, 0), pointsDef(1, 0), pointsDef(2, 0)), normVec);

  std::map<int, boost::shared_ptr<Geometry::Surface>> surfaceShapes;
  surfaceShapes[1] = cylinder;
  surfaceShapes[2] = planeTop;
  surfaceShapes[3] = planeBottom;

  // Bounding box calculation (Simple version)
  std::vector<double> boundingBoxSimplified;
  for (int i = 0; i < 3; ++i) {
    double max = pointsDef.row(i).maxCoeff();
    double min = pointsDef.row(i).minCoeff();

    if (max == centre[i]) {
      max += radius;
    }
    if (min == centre[i]) {
      min -= radius;
    }

    // xmax, xmin, ymax, ymin, zmax, zmin
    boundingBoxSimplified.push_back(max);
    boundingBoxSimplified.push_back(min);
  }

  std::string algebra = "(-1 -2 3)";
  return createShape(surfaceShapes, algebra, boundingBoxSimplified);
}

std::unique_ptr<const Geometry::IObject>
createFromOFFMesh(const std::vector<uint16_t> &faceIndices,
                  const std::vector<uint16_t> &windingOrder,
                  const std::vector<float> &nexusVertices) {
  std::vector<uint16_t> triangularFaces =
      createTriangularFaces(faceIndices, windingOrder);

  std::vector<Mantid::Kernel::V3D> vertices(nexusVertices.size() / 3);

  for (size_t vertexNumber = 0; vertexNumber < nexusVertices.size();
       vertexNumber += 3) {
    auto &vertex = vertices[vertexNumber / 3];
    vertex = Mantid::Kernel::V3D(nexusVertices[vertexNumber],
                                 nexusVertices[vertexNumber + 1],
                                 nexusVertices[vertexNumber + 2]);
  }

  return NexusShapeFactory::createMesh(std::move(triangularFaces),
                                       std::move(vertices));
}

std::unique_ptr<const Geometry::IObject>
createFromOFFMesh(const std::vector<uint16_t> &faceIndices,
                  const std::vector<uint16_t> &windingOrder,
                  const std::vector<Eigen::Vector3d> &nexusVertices) {
  std::vector<uint16_t> triangularFaces =
      createTriangularFaces(faceIndices, windingOrder);

  std::vector<Mantid::Kernel::V3D> vertices(nexusVertices.size());

  std::transform(
      nexusVertices.cbegin(), nexusVertices.cend(), vertices.begin(),
      [](const Eigen::Vector3d &vert) { return Kernel::toV3D(vert); });

  return NexusShapeFactory::createMesh(std::move(triangularFaces),
                                       std::move(vertices));
}

std::unique_ptr<const Geometry::IObject>
createMesh(std::vector<uint16_t> &&triangularFaces,
           std::vector<Mantid::Kernel::V3D> &&vertices) {

  if (NexusShapeFactory::pointsCoplanar(vertices))
    // TODO. Make Mesh2D
    return Mantid::Kernel::make_unique<Geometry::MeshObject>(
        std::move(triangularFaces), std::move(vertices), Kernel::Material{});
  else
    return Mantid::Kernel::make_unique<Geometry::MeshObject>(
        std::move(triangularFaces), std::move(vertices), Kernel::Material{});
}
} // namespace NexusShapeFactory
} // namespace NexusGeometry
} // namespace Mantid
