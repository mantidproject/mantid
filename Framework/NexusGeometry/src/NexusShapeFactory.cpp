#include "MantidGeometry/Objects/IObject.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidGeometry/Objects/Rules.h"
#include "MantidGeometry/Surfaces/Cylinder.h"
#include "MantidGeometry/Surfaces/Surface.h"
#include "MantidGeometry/Surfaces/Plane.h"
#include "MantidNexusGeometry/NexusShapeFactory.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/V3D.h"
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
  return shape;
}
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
  surfaceShapes[0] = cylinder;
  surfaceShapes[1] = planeTop;
  surfaceShapes[2] = planeBottom;

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
  // std::string algebra = "(-0 -1 2)";
  std::string algebra = "(-0 -1 2)";
  return createShape(surfaceShapes, algebra, boundingBoxSimplified);
}

std::unique_ptr<const Geometry::IObject>
createMesh(std::vector<uint16_t> &&triangularFaces,
           std::vector<Mantid::Kernel::V3D> &&vertices) {

  return Mantid::Kernel::make_unique<Geometry::MeshObject>(
      std::move(triangularFaces), std::move(vertices), Kernel::Material{});
}
}
}
}
