// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidNexusGeometry/NexusShapeFactory.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidGeometry/Objects/MeshObject2D.h"
#include "MantidGeometry/Objects/Rules.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidGeometry/Rendering/ShapeInfo.h"
#include "MantidGeometry/Surfaces/Cylinder.h"
#include "MantidGeometry/Surfaces/Plane.h"
#include "MantidGeometry/Surfaces/Surface.h"
#include "MantidKernel/EigenConversionHelpers.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/V3D.h"

#include <iterator>
#include <memory>

namespace Mantid::NexusGeometry::NexusShapeFactory {
using namespace Eigen;

namespace {

/// Finalise shape
std::unique_ptr<const Geometry::IObject>
createCylinderShape(const std::map<int, std::shared_ptr<Geometry::Surface>> &surfaces, const std::string &algebra,
                    const std::vector<double> &boundingBox, Geometry::detail::ShapeInfo &&shapeInfo) {
  auto shape = std::make_unique<Geometry::CSGObject>();
  shape->setObject(21, algebra);
  shape->populate(surfaces);
  // Boundingbox x,y,z:max; x,y,z:min
  shape->defineBoundingBox(boundingBox[0], boundingBox[2], boundingBox[4], boundingBox[1], boundingBox[3],
                           boundingBox[5]);
  shape->getGeometryHandler()->setShapeInfo(std::move(shapeInfo));
  // Some older compilers (Apple clang 7) don't support copy construction
  // std::unique_ptr<const T>(const std::ptr<T>&)
  return std::unique_ptr<const Geometry::IObject>(shape.release());
}

void createTrianglesFromPolygon(const std::vector<uint32_t> &windingOrder, std::vector<uint32_t> &triangularFaces,
                                int &startOfFace, const int &endOfFace, int &windingOrderReached) {
  int polygonOrder = endOfFace - startOfFace;
  auto first = windingOrder.begin() + windingOrderReached;

  triangularFaces.reserve(triangularFaces.size() + 3 * polygonOrder);
  for (int polygonVertex = 1; polygonVertex < polygonOrder - 1; ++polygonVertex) {
    triangularFaces.emplace_back(*first);
    triangularFaces.emplace_back(*(first + polygonVertex));
    triangularFaces.emplace_back(*(first + polygonVertex + 1));
  }
  windingOrderReached += polygonOrder;
  startOfFace = endOfFace; // start of the next face
}

std::vector<uint32_t> createTriangularFaces(const std::vector<uint32_t> &faceIndices,
                                            const std::vector<uint32_t> &windingOrder) {

  // Elements 0 to 2 are the indices of the vertices vector corresponding to the
  // vertices of the first triangle.
  // Elements 3 to 5 are for the second triangle, and so on.
  // The order of the vertices is the winding order of the triangle, determining
  // the face normal by right-hand rule
  std::vector<uint32_t> triangularFaces;

  auto startOfFace = static_cast<int>(faceIndices[0]);
  int endOfFace = 0;
  int windingOrderReached = 0;
  for (auto it = faceIndices.begin() + 1; it != faceIndices.end(); ++it) {
    endOfFace = static_cast<int>(*it);
    createTrianglesFromPolygon(windingOrder, triangularFaces, startOfFace, endOfFace, windingOrderReached);
  }

  // and the last face
  endOfFace = startOfFace + static_cast<int>(windingOrder.size()) - windingOrderReached;
  createTrianglesFromPolygon(windingOrder, triangularFaces, startOfFace, endOfFace, windingOrderReached);

  return triangularFaces;
}
} // namespace

DLLExport std::unique_ptr<const Geometry::IObject> createCylinder(const std::vector<uint32_t> &cylinderPoints,
                                                                  const std::vector<Eigen::Vector3d> &vertices) {
  // Read points into matrix, sorted by cPoints ordering
  Eigen::Matrix<double, 3, 3> vSorted;
  for (int i = 0; i < 3; ++i) {
    vSorted.col(cylinderPoints[i]) = vertices[i];
  }
  return createCylinder(vSorted);
}

/** Refer to NX_Cylinder definition here
 * http://download.nexusformat.org/doc/html/classes/base_classes/NXcylindrical_geometry.html?highlight=nxcylindrical_geometry
 * @param pointsDef Eigen matrix which contains which contains vertices A
 * (pointsDef.col(0)), B (pointsDef.col(1)), C(pointDef.col(2)) which define our
 * nexus cylinder.
 */
std::unique_ptr<const Geometry::IObject> createCylinder(const Eigen::Matrix<double, 3, 3> &pointsDef) {
  // Calculate cylinder parameters
  auto centre = (pointsDef.col(2) + pointsDef.col(0)) * 0.5;
  auto axisVector = pointsDef.col(2) - pointsDef.col(0);
  auto height = axisVector.norm();
  auto axisUnitVector = axisVector.normalized();
  auto radiusVector = pointsDef.col(1) - pointsDef.col(0);
  double radius = radiusVector.norm();
  Kernel::V3D normVec = Kernel::V3D(axisUnitVector(0), axisUnitVector(1), axisUnitVector(2));

  auto cylinder = std::make_shared<Geometry::Cylinder>();
  auto vCenter = Kernel::V3D(centre(0), centre(1), centre(2));
  cylinder->setCentre(vCenter);
  cylinder->setNorm(normVec);
  cylinder->setRadius(radius);

  // Top plane
  auto planeTop = std::make_shared<Geometry::Plane>();
  planeTop->setPlane(Kernel::V3D(pointsDef(0, 2), pointsDef(1, 2), pointsDef(2, 2)), normVec);

  // Bottom plane
  auto planeBottom = std::make_shared<Geometry::Plane>();
  planeBottom->setPlane(Kernel::V3D(pointsDef(0, 0), pointsDef(1, 0), pointsDef(2, 0)), normVec);

  std::map<int, std::shared_ptr<Geometry::Surface>> surfaceShapes;
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
    boundingBoxSimplified.emplace_back(max);
    boundingBoxSimplified.emplace_back(min);
  }

  std::string algebra = "(-1 -2 3)";
  Geometry::detail::ShapeInfo shapeInfo;
  shapeInfo.setCylinder(vCenter, normVec, radius, height);
  return createCylinderShape(surfaceShapes, algebra, boundingBoxSimplified, std::move(shapeInfo));
}

std::unique_ptr<const Geometry::IObject> createFromOFFMesh(const std::vector<uint32_t> &faceIndices,
                                                           const std::vector<uint32_t> &windingOrder,
                                                           const std::vector<double> &nexusVertices) {
  std::vector<uint32_t> triangularFaces = createTriangularFaces(faceIndices, windingOrder);

  std::vector<Mantid::Kernel::V3D> vertices;
  vertices.reserve(nexusVertices.size() / 3);

  for (size_t vertexNumber = 0; vertexNumber < nexusVertices.size(); vertexNumber += 3) {
    vertices.emplace_back(nexusVertices[vertexNumber], nexusVertices[vertexNumber + 1],
                          nexusVertices[vertexNumber + 2]);
  }

  return NexusShapeFactory::createMesh(std::move(triangularFaces), std::move(vertices));
}

std::vector<Mantid::Kernel::V3D> toVectorV3D(const std::vector<Eigen::Vector3d> &nexusVertices) {
  std::vector<Mantid::Kernel::V3D> vertices;
  vertices.reserve(nexusVertices.size());

  std::transform(nexusVertices.cbegin(), nexusVertices.cend(), std::back_inserter(vertices),
                 [](const Eigen::Vector3d &vert) { return Kernel::toV3D(vert); });

  return vertices;
}

std::unique_ptr<const Geometry::IObject> createFromOFFMesh(const std::vector<uint32_t> &faceIndices,
                                                           const std::vector<uint32_t> &windingOrder,
                                                           const std::vector<Eigen::Vector3d> &nexusVertices) {
  std::vector<uint32_t> triangularFaces = createTriangularFaces(faceIndices, windingOrder);

  return NexusShapeFactory::createMesh(std::move(triangularFaces), toVectorV3D(nexusVertices));
}

std::unique_ptr<const Geometry::IObject> createMesh(std::vector<uint32_t> &&triangularFaces,
                                                    std::vector<Mantid::Kernel::V3D> &&vertices) {

  if (Geometry::MeshObject2D::pointsCoplanar(vertices))
    return std::make_unique<Geometry::MeshObject2D>(std::move(triangularFaces), std::move(vertices),
                                                    Kernel::Material{});
  else
    return std::make_unique<Geometry::MeshObject>(std::move(triangularFaces), std::move(vertices), Kernel::Material{});
}
} // namespace Mantid::NexusGeometry::NexusShapeFactory
