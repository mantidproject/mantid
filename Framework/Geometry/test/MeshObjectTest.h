// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidGeometry/Math/Algebra.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidGeometry/Objects/MeshObjectCommon.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/MersenneTwister.h"
#include "MockRNG.h"

#include <optional>

#include <cxxtest/TestSuite.h>

#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/Document.h>

using namespace Mantid;
using namespace Geometry;
using Mantid::Kernel::V3D;

namespace {
std::unique_ptr<MeshObject> createCube(const double size, const V3D &centre) {
  /**
   * Create cube of side length size with specified centre,
   * parellel to axes and non-negative vertex coordinates.
   */
  double min = 0.0 - 0.5 * size;
  double max = 0.5 * size;
  std::vector<V3D> vertices;
  vertices.emplace_back(centre + V3D(max, max, max));
  vertices.emplace_back(centre + V3D(min, max, max));
  vertices.emplace_back(centre + V3D(max, min, max));
  vertices.emplace_back(centre + V3D(min, min, max));
  vertices.emplace_back(centre + V3D(max, max, min));
  vertices.emplace_back(centre + V3D(min, max, min));
  vertices.emplace_back(centre + V3D(max, min, min));
  vertices.emplace_back(centre + V3D(min, min, min));

  std::vector<uint32_t> triangles;
  // top face of cube - z max
  triangles.insert(triangles.end(), {0, 1, 2});
  triangles.insert(triangles.end(), {2, 1, 3});
  // right face of cube - x max
  triangles.insert(triangles.end(), {0, 2, 4});
  triangles.insert(triangles.end(), {4, 2, 6});
  // back face of cube - y max
  triangles.insert(triangles.end(), {0, 4, 1});
  triangles.insert(triangles.end(), {1, 4, 5});
  // bottom face of cube - z min
  triangles.insert(triangles.end(), {7, 5, 6});
  triangles.insert(triangles.end(), {6, 5, 4});
  // left face of cube - x min
  triangles.insert(triangles.end(), {7, 3, 5});
  triangles.insert(triangles.end(), {5, 3, 1});
  // front fact of cube - y min
  triangles.insert(triangles.end(), {7, 6, 3});
  triangles.insert(triangles.end(), {3, 6, 2});

  // Use efficient constructor
  std::unique_ptr<MeshObject> retVal =
      std::make_unique<MeshObject>(std::move(triangles), std::move(vertices), Mantid::Kernel::Material());
  return retVal;
}

std::unique_ptr<MeshObject> createCube(const double size) {
  /**
   * Create cube of side length size with vertex at origin,
   * parellel to axes and non-negative vertex coordinates.
   */
  return createCube(size, V3D(0.5 * size, 0.5 * size, 0.5 * size));
}

std::unique_ptr<MeshObject> createOctahedron() {
  /**
   * Create octahedron with vertices on the axes at -1 & +1.
   */
  // The octahedron is made slightly bigger than this to
  // ensure interior points are not rounded to be outside
  // Opposite vertices have indices differing by 3.
  double u = 1.0000000001;
  std::vector<V3D> vertices;
  vertices.emplace_back(V3D(u, 0, 0));
  vertices.emplace_back(V3D(0, u, 0));
  vertices.emplace_back(V3D(0, 0, u));
  vertices.emplace_back(V3D(-u, 0, 0));
  vertices.emplace_back(V3D(0, -u, 0));
  vertices.emplace_back(V3D(0, 0, -u));

  std::vector<uint32_t> triangles;
  // +++ face
  triangles.insert(triangles.end(), {0, 1, 2});
  //++- face
  triangles.insert(triangles.end(), {0, 5, 1});
  // +-- face
  triangles.insert(triangles.end(), {0, 4, 5});
  // +-+ face
  triangles.insert(triangles.end(), {0, 2, 4});
  // --- face
  triangles.insert(triangles.end(), {3, 5, 4});
  // --+ face
  triangles.insert(triangles.end(), {3, 4, 2});
  // -++ face
  triangles.insert(triangles.end(), {3, 2, 1});
  // -+- face
  triangles.insert(triangles.end(), {3, 1, 5});

  // Use flexible constructor
  std::unique_ptr<MeshObject> retVal = std::make_unique<MeshObject>(triangles, vertices, Mantid::Kernel::Material());
  return retVal;
}

std::unique_ptr<MeshObject> createLShape() {
  /**
   * Create an L shape with vertices at
   * (0,0,Z) (2,0,Z) (2,1,Z) (1,1,Z) (1,2,Z) & (0,2,Z),
   *  where Z = 0 or 1.
   */
  std::vector<V3D> vertices;
  vertices.emplace_back(V3D(0, 0, 0));
  vertices.emplace_back(V3D(2, 0, 0));
  vertices.emplace_back(V3D(2, 1, 0));
  vertices.emplace_back(V3D(1, 1, 0));
  vertices.emplace_back(V3D(1, 2, 0));
  vertices.emplace_back(V3D(0, 2, 0));
  vertices.emplace_back(V3D(0, 0, 1));
  vertices.emplace_back(V3D(2, 0, 1));
  vertices.emplace_back(V3D(2, 1, 1));
  vertices.emplace_back(V3D(1, 1, 1));
  vertices.emplace_back(V3D(1, 2, 1));
  vertices.emplace_back(V3D(0, 2, 1));

  std::vector<uint32_t> triangles;
  // z min
  triangles.insert(triangles.end(), {0, 5, 1});
  triangles.insert(triangles.end(), {1, 3, 2});
  triangles.insert(triangles.end(), {3, 5, 4});
  // z max
  triangles.insert(triangles.end(), {6, 7, 11});
  triangles.insert(triangles.end(), {11, 9, 10});
  triangles.insert(triangles.end(), {9, 7, 8});
  // y min
  triangles.insert(triangles.end(), {0, 1, 6});
  triangles.insert(triangles.end(), {6, 1, 7});
  // x max
  triangles.insert(triangles.end(), {1, 2, 7});
  triangles.insert(triangles.end(), {7, 2, 8});
  // y mid
  triangles.insert(triangles.end(), {2, 3, 8});
  triangles.insert(triangles.end(), {8, 3, 9});
  // x mid
  triangles.insert(triangles.end(), {3, 4, 9});
  triangles.insert(triangles.end(), {9, 4, 10});
  // y max
  triangles.insert(triangles.end(), {4, 5, 10});
  triangles.insert(triangles.end(), {10, 5, 11});
  // x min
  triangles.insert(triangles.end(), {5, 0, 11});
  triangles.insert(triangles.end(), {11, 0, 6});

  // Use efficient constructor
  std::unique_ptr<MeshObject> retVal =
      std::make_unique<MeshObject>(std::move(triangles), std::move(vertices), Mantid::Kernel::Material());
  return retVal;
}
} // namespace

class MeshObjectTest : public CxxTest::TestSuite {

public:
  void testConstructor() {

    std::vector<V3D> vertices;
    vertices.emplace_back(V3D(0, 0, 0));
    vertices.emplace_back(V3D(1, 0, 0));
    vertices.emplace_back(V3D(0, 1, 0));
    vertices.emplace_back(V3D(0, 0, 1));

    std::vector<uint32_t> triangles;
    triangles.insert(triangles.end(), {1, 2, 3});
    triangles.insert(triangles.end(), {2, 1, 0});
    triangles.insert(triangles.end(), {3, 0, 1});
    triangles.insert(triangles.end(), {0, 3, 2});

    // Test flexible constructor
    TS_ASSERT_THROWS_NOTHING(MeshObject(triangles, vertices, Mantid::Kernel::Material()));
    // Test eficient constructor
    TS_ASSERT_THROWS_NOTHING(MeshObject(std::move(triangles), std::move(vertices), Mantid::Kernel::Material()));
  }

  void testClone() {
    auto geom_obj = createOctahedron();
    std::unique_ptr<IObject> cloned;
    TS_ASSERT_THROWS_NOTHING(cloned.reset(geom_obj->clone()));
    TS_ASSERT(cloned);
  }

  void testMaterial() {
    using Mantid::Kernel::Material;
    std::vector<V3D> vertices;
    vertices.emplace_back(V3D(0, 0, 0));
    vertices.emplace_back(V3D(1, 0, 0));
    vertices.emplace_back(V3D(0, 1, 0));
    vertices.emplace_back(V3D(0, 0, 1));

    std::vector<uint32_t> triangles;
    triangles.insert(triangles.end(), {1, 2, 3});
    triangles.insert(triangles.end(), {2, 1, 0});
    triangles.insert(triangles.end(), {3, 0, 1});
    triangles.insert(triangles.end(), {0, 3, 2});

    auto testMaterial = Material("arm", PhysicalConstants::getNeutronAtom(13), 45.0);

    // Test material through flexible constructor
    auto obj1 = std::make_unique<MeshObject>(triangles, vertices, testMaterial);
    TSM_ASSERT_DELTA("Expected a number density of 45", 45.0, obj1->material().numberDensity(), 1e-12);
    // Test material through efficient constructor
    auto obj2 = std::make_unique<MeshObject>(std::move(triangles), std::move(vertices), testMaterial);
    TSM_ASSERT_DELTA("Expected a number density of 45", 45.0, obj2->material().numberDensity(), 1e-12);
  }

  void testCloneWithMaterial() {
    using Mantid::Kernel::Material;
    auto testMaterial = Material("arm", PhysicalConstants::getNeutronAtom(13), 45.0);
    auto geom_obj = createOctahedron();
    std::unique_ptr<IObject> cloned_obj;
    TS_ASSERT_THROWS_NOTHING(cloned_obj.reset(geom_obj->cloneWithMaterial(testMaterial)));
    TSM_ASSERT_DELTA("Expected a number density of 45", 45.0, cloned_obj->material().numberDensity(), 1e-12);
  }

  void testHasValidShape() {
    auto geom_obj = createCube(1.0);
    TS_ASSERT(geom_obj->hasValidShape());
  }

  void testGetBoundingBoxForCube() {
    auto geom_obj = createCube(4.1);
    const double tolerance(1e-10);

    const BoundingBox &bbox = geom_obj->getBoundingBox();

    TS_ASSERT_DELTA(bbox.xMax(), 4.1, tolerance);
    TS_ASSERT_DELTA(bbox.yMax(), 4.1, tolerance);
    TS_ASSERT_DELTA(bbox.zMax(), 4.1, tolerance);
    TS_ASSERT_DELTA(bbox.xMin(), 0.0, tolerance);
    TS_ASSERT_DELTA(bbox.yMin(), 0.0, tolerance);
    TS_ASSERT_DELTA(bbox.zMin(), 0.0, tolerance);
  }

  void testGetBoundingBoxForOctahedron() {
    auto geom_obj = createOctahedron();
    const double tolerance(1e-10);

    const BoundingBox &bbox = geom_obj->getBoundingBox();

    TS_ASSERT_DELTA(bbox.xMax(), 1.0, tolerance);
    TS_ASSERT_DELTA(bbox.yMax(), 1.0, tolerance);
    TS_ASSERT_DELTA(bbox.zMax(), 1.0, tolerance);
    TS_ASSERT_DELTA(bbox.xMin(), -1.0, tolerance);
    TS_ASSERT_DELTA(bbox.yMin(), -1.0, tolerance);
    TS_ASSERT_DELTA(bbox.zMin(), -1.0, tolerance);
  }

  void testGetBoundingBoxForLShape() {
    auto geom_obj = createLShape();
    const double tolerance(1e-10);

    const BoundingBox &bbox = geom_obj->getBoundingBox();

    TS_ASSERT_DELTA(bbox.xMax(), 2.0, tolerance);
    TS_ASSERT_DELTA(bbox.yMax(), 2.0, tolerance);
    TS_ASSERT_DELTA(bbox.zMax(), 1.0, tolerance);
    TS_ASSERT_DELTA(bbox.xMin(), 0.0, tolerance);
    TS_ASSERT_DELTA(bbox.yMin(), 0.0, tolerance);
    TS_ASSERT_DELTA(bbox.zMin(), 0.0, tolerance);
  }

  void testInterceptCubeX() {
    std::vector<Link> expectedResults;
    auto geom_obj = createCube(4.0);
    Track track(V3D(-10, 1, 1), V3D(1, 0, 0));

    // format = startPoint, endPoint, total distance so far
    expectedResults.emplace_back(Link(V3D(0, 1, 1), V3D(4, 1, 1), 14.0, *geom_obj));
    checkTrackIntercept(std::move(geom_obj), track, expectedResults);
  }

  void testInterceptCubeXY() {
    std::vector<Link> expectedResults;
    auto geom_obj = createCube(4.0);
    Track track(V3D(-8, -6, 1), V3D(0.8, 0.6, 0));

    // format = startPoint, endPoint, total distance so far
    expectedResults.emplace_back(Link(V3D(0, 0, 1), V3D(4, 3, 1), 15.0, *geom_obj));
    checkTrackIntercept(std::move(geom_obj), track, expectedResults);
  }

  void testInterceptCubeMiss() {
    std::vector<Link> expectedResults; // left empty as there are no expected results
    auto geom_obj = createCube(4.0);
    V3D dir(1., 1., 0.);
    dir.normalize();
    Track track(V3D(-10, 0, 0), dir);

    checkTrackIntercept(std::move(geom_obj), track, expectedResults);
  }

  void testInterceptOctahedronX() {
    std::vector<Link> expectedResults;
    auto geom_obj = createOctahedron();
    Track track(V3D(-10, 0.2, 0.2), V3D(1, 0, 0));

    // format = startPoint, endPoint, total distance so far
    expectedResults.emplace_back(Link(V3D(-0.6, 0.2, 0.2), V3D(0.6, 0.2, 0.2), 10.6, *geom_obj));
    checkTrackIntercept(std::move(geom_obj), track, expectedResults);
  }

  void testInterceptOctahedronXthroughEdge() {
    std::vector<Link> expectedResults;
    auto geom_obj = createOctahedron();
    Track track(V3D(-10, 0.2, 0.0), V3D(1, 0, 0));

    // format = startPoint, endPoint, total distance so far
    expectedResults.emplace_back(Link(V3D(-0.8, 0.2, 0.0), V3D(0.8, 0.2, 0.0), 10.8, *geom_obj));
    checkTrackIntercept(std::move(geom_obj), track, expectedResults);
  }

  void testInterceptOctahedronXthroughVertex() {
    std::vector<Link> expectedResults;
    auto geom_obj = createOctahedron();
    Track track(V3D(-10, 0.0, 0.0), V3D(1, 0, 0));

    // format = startPoint, endPoint, total distance so far
    expectedResults.emplace_back(Link(V3D(-1.0, 0.0, 0.0), V3D(1.0, 0.0, 0.0), 11.0, *geom_obj));
    checkTrackIntercept(std::move(geom_obj), track, expectedResults);
  }

  void testInterceptLShapeTwoPass() {
    std::vector<Link> expectedResults;
    auto geom_obj = createLShape();
    V3D dir(1., -1., 0.);
    dir.normalize();
    Track track(V3D(0, 2.5, 0.5), dir);

    // format = startPoint, endPoint, total distance so far
    expectedResults.emplace_back(Link(V3D(0.5, 2, 0.5), V3D(1, 1.5, 0.5), 1.4142135, *geom_obj));
    expectedResults.emplace_back(Link(V3D(1.5, 1, 0.5), V3D(2, 0.5, 0.5), 2.828427, *geom_obj));
    checkTrackIntercept(std::move(geom_obj), track, expectedResults);
  }

  void testInterceptLShapeMiss() {
    std::vector<Link> expectedResults; // left empty as there are no expected results
    auto geom_obj = createLShape();
    // Passes through convex hull of L-Shape
    Track track(V3D(1.1, 1.1, -1), V3D(0, 0, 1));

    checkTrackIntercept(std::move(geom_obj), track, expectedResults);
  }

  void testDistanceWithIntersectionReturnsResult() {
    auto geom_obj = createCube(3);
    V3D dir(0., 1., 0.);
    dir.normalize();
    Track track(V3D(0, 0, 0), dir);

    TS_ASSERT_DELTA(3.0, geom_obj->distance(track), 1e-08)
  }

  void testDistanceWithoutIntersectionThrows() {
    auto geom_obj = createCube(3);
    V3D dir(-1., 0., 0.);
    dir.normalize();
    Track track(V3D(-10, 0, 0), dir);

    TS_ASSERT_THROWS(geom_obj->distance(track), const std::runtime_error &)
  }

  void testTrackTwoIsolatedCubes()
  /**
  Test a track going through two objects
  */
  {
    auto object1 = createCube(2.0, V3D(0.0, 0.0, 0.0));

    auto object2 = createCube(2.0, V3D(5.5, 0.0, 0.0));

    Track TL(Kernel::V3D(-5, 0, 0), Kernel::V3D(1, 0, 0));

    // CARE: This CANNOT be called twice
    TS_ASSERT(object1->interceptSurface(TL) != 0);
    TS_ASSERT(object2->interceptSurface(TL) != 0);

    std::vector<Link> expectedResults;
    expectedResults.emplace_back(Link(V3D(-1, 0, 0), V3D(1, 0, 0), 6, *object1));
    expectedResults.emplace_back(Link(V3D(4.5, 0, 0), V3D(6.5, 0, 0), 11.5, *object2));
    checkTrackIntercept(TL, expectedResults);
  }

  void testTrackTwoTouchingCubes()
  /**
  Test a track going through two objects
  */
  {
    auto object1 = createCube(2.0, V3D(0.0, 0.0, 0.0));

    auto object2 = createCube(4.0, V3D(3.0, 0.0, 0.0));

    Track TL(Kernel::V3D(-5, 0, 0), Kernel::V3D(1, 0, 0));

    // CARE: This CANNOT be called twice
    TS_ASSERT(object1->interceptSurface(TL) != 0);
    TS_ASSERT(object2->interceptSurface(TL) != 0);

    std::vector<Link> expectedResults;
    expectedResults.emplace_back(Link(V3D(-1, 0, 0), V3D(1, 0, 0), 6, *object1));
    expectedResults.emplace_back(Link(V3D(1, 0, 0), V3D(5, 0, 0), 10.0, *object2));
    checkTrackIntercept(TL, expectedResults);
  }

  void checkTrackIntercept(Track &track, const std::vector<Link> &expectedResults) {
    size_t index = 0;
    for (auto it = track.cbegin(); it != track.cend(); ++it) {
      if (index < expectedResults.size()) {
        TS_ASSERT_DELTA(it->distFromStart, expectedResults[index].distFromStart, 1e-6);
        TS_ASSERT_DELTA(it->distInsideObject, expectedResults[index].distInsideObject, 1e-6);
        TS_ASSERT_EQUALS(it->componentID, expectedResults[index].componentID);
        TS_ASSERT_EQUALS(it->entryPoint, expectedResults[index].entryPoint);
        TS_ASSERT_EQUALS(it->exitPoint, expectedResults[index].exitPoint);
      }
      ++index;
    }
    TS_ASSERT_EQUALS(index, expectedResults.size());
  }

  void checkTrackIntercept(IObject_uptr obj, Track &track, const std::vector<Link> &expectedResults) {
    int unitCount = obj->interceptSurface(track);
    TS_ASSERT_EQUALS(unitCount, expectedResults.size());
    checkTrackIntercept(track, expectedResults);
  }

  void testIsOnSideCube() {
    auto geom_obj = createCube(1.0);
    // inside
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.5, 0.5, 0.5)), false); // centre
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.5, 0.1, 0.5)), false);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.5, 0.9, 0.5)), false);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.5, 0.5, 0.1)), false);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.5, 0.5, 0.9)), false);
    // on the faces
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.0, 0.5, 0.5)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.5, 0.5, 0.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.5, 0.0, 0.5)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(1.0, 0.5, 0.5)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.5, 0.5, 1.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.5, 1.0, 0.5)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.0, 0.1, 0.9)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.1, 0.0, 0.9)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(1.0, 0.9, 0.1)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.1, 1.0, 0.9)), true);
    // on the edges
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.5, 0.0, 0.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.0, 0.5, 0.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.0, 0.0, 0.5)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.5, 0.0, 1.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(1.0, 0.5, 0.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.0, 1.0, 0.5)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.5, 1.0, 1.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(1.0, 0.5, 1.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(1.0, 1.0, 0.5)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.1, 1.0, 0.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(1.0, 0.9, 0.0)), true);
    // on the vertices
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.0, 0.0, 0.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.0, 0.0, 1.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.0, 1.0, 0.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.0, 1.0, 1.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(1.0, 0.0, 0.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(1.0, 0.0, 1.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(1.0, 1.0, 0.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(1.0, 1.0, 1.0)), true);
    // out side
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.5, 1.1, 0.5)), false);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.5, -0.1, 0.5)), false);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.5, 0.5, -0.1)), false);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.1, 0.0, 1.1)), false);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(1.3, 0.9, 0.0)), false);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(-3.3, 2.0, 0.9)), false);
  }

  void testIsValidCube() {
    auto geom_obj = createCube(1.0);
    // inside
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.5, 0.5, 0.5)), true); // centre
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.5, 0.1, 0.5)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.5, 0.9, 0.5)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.5, 0.5, 0.1)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.5, 0.5, 0.9)), true);
    // on the faces
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.0, 0.5, 0.5)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.5, 0.5, 0.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.5, 0.0, 0.5)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(1.0, 0.5, 0.5)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.5, 0.5, 1.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.5, 1.0, 0.5)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.0, 0.1, 0.9)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.1, 0.0, 0.9)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(1.0, 0.9, 0.1)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.1, 1.0, 0.9)), true);
    // on the edges
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.5, 0.0, 0.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.0, 0.5, 0.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.0, 0.0, 0.5)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.5, 0.0, 1.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(1.0, 0.5, 0.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.0, 1.0, 0.5)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.5, 1.0, 1.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(1.0, 0.5, 1.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(1.0, 1.0, 0.5)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.1, 1.0, 0.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(1.0, 0.9, 0.0)), true);
    // on the vertices
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.0, 0.0, 0.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.0, 0.0, 1.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.0, 1.0, 0.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.0, 1.0, 1.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(1.0, 0.0, 0.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(1.0, 0.0, 1.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(1.0, 1.0, 0.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(1.0, 1.0, 1.0)), true);
    // out side
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.5, 1.1, 0.5)), false);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.5, -0.1, 0.5)), false);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.5, 0.5, -0.1)), false);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.1, 0.0, 1.1)), false);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(1.3, 0.9, 0.0)), false);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(-3.3, 2.0, 0.9)), false);
  }

  void testIsOnSideOctahedron() {
    auto geom_obj = createOctahedron();
    // inside
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.0, 0.0, 0.0)), false); // centre
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.5, 0.2, 0.2)), false);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.2, 0.5, -0.2)), false);
    // on face
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.5, 0.3, 0.2)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.5, -0.3, 0.2)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.4, -0.4, -0.2)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(-0.4, 0.3, 0.3)), true);
    // on edge
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.0, 0.5, 0.5)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.0, -0.5, -0.5)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.7, 0.0, 0.3)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(-0.7, 0.0, -0.3)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.8, 0.2, 0.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(-0.8, 0.2, 0.0)), true);
    // on vertex
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(1.0, 0.0, 0.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(-1.0, 0.0, 0.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.0, 1.0, 0.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.0, -1.0, 0.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.0, 0.0, 1.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.0, 0.0, -1.0)), true);
    // out side
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.35, 0.35, 0.35)), false);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.35, -0.35, -0.35)), false);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(-0.35, 0.35, 0.35)), false);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(-0.35, 0.35, -0.35)), false);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(2.0, 2.0, 0.0)), false);
  }

  void testIsValidOctahedron() {
    auto geom_obj = createOctahedron();
    // inside
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.0, 0.0, 0.0)), true); // centre
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.5, 0.2, 0.2)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.2, 0.5, -0.2)), true);
    // on face
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.5, 0.3, 0.2)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.5, -0.3, 0.2)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.4, -0.4, -0.2)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(-0.4, 0.3, 0.3)), true);
    // on edge
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.0, 0.5, 0.5)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.0, -0.5, -0.5)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.7, 0.0, 0.3)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(-0.7, 0.0, -0.3)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.8, 0.2, 0.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(-0.8, 0.2, 0.0)), true);
    // on vertex
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(1.0, 0.0, 0.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(-1.0, 0.0, 0.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.0, 1.0, 0.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.0, -1.0, 0.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.0, 0.0, 1.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.0, 0.0, -1.0)), true);
    // out side
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.35, 0.35, 0.35)), false);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.35, -0.35, -0.35)), false);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(-0.35, 0.35, 0.35)), false);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(-0.35, 0.35, -0.35)), false);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(2.0, 2.0, 0.0)), false);
  }

  void testIsOnSideLShape() {
    auto geom_obj = createLShape();
    // inside
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.5, 0.5, 0.5)), false);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(1.5, 0.5, 0.5)), false);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.5, 1.5, 0.5)), false);
    // on front and back
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.5, 0.5, 0.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(1.5, 0.5, 0.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.5, 1.5, 0.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.5, 0.5, 1.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(1.5, 0.5, 1.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.5, 1.5, 1.0)), true);
    // on sides
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(1.0, 0.0, 0.5)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.0, 1.0, 0.5)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(2.0, 0.5, 0.5)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.5, 2.0, 0.5)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(1.5, 1.0, 0.5)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(1.0, 1.5, 0.5)), true);
    // out side
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.5, 0.5, 1.5)), false);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(2.0, 2.0, 0.5)), false);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(2.0, 2.0, 0.0)), false);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(1.1, 1.1, 0.5)), false);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(1.1, 1.1, 1.0)), false);
  }

  void testIsValidLShape() {
    auto geom_obj = createLShape();
    // inside
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.5, 0.5, 0.5)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(1.5, 0.5, 0.5)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.5, 1.5, 0.5)), true);
    // on front and back
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.5, 0.5, 0.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(1.5, 0.5, 0.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.5, 1.5, 0.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.5, 0.5, 1.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(1.5, 0.5, 1.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.5, 1.5, 1.0)), true);
    // on sides
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(1.0, 0.0, 0.5)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.0, 1.0, 0.5)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(2.0, 0.5, 0.5)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.5, 2.0, 0.5)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(1.5, 1.0, 0.5)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(1.0, 1.5, 0.5)), true);
    // out side
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.5, 0.5, 1.5)), false);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(2.0, 2.0, 0.5)), false);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(2.0, 2.0, 0.0)), false);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(1.1, 1.1, 0.5)), false);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(1.1, 1.1, 1.0)), false);
  }

  void testCalcValidTypeCube() {
    auto geom_obj = createCube(1.0);
    // entry or exit on the normal
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0.0, 0.5, 0.5), V3D(1, 0, 0)), 1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0.0, 0.5, 0.5), V3D(-1, 0, 0)), -1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(1.0, 0.5, 0.5), V3D(1, 0, 0)), -1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(1.0, 0.5, 0.5), V3D(-1, 0, 0)), 1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0.5, 0.0, 0.5), V3D(0, 1, 0)), 1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0.5, 0.0, 0.5), V3D(0, -1, 0)), -1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0.5, 1.0, 0.5), V3D(0, 1, 0)), -1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0.5, 1.0, 0.5), V3D(0, -1, 0)), 1);

    // glancing blow on edge
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0.0, 0.0, 0.5), V3D(1, -1, 0)), 0);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0.5, 0.0, 0.0), V3D(0, -1, 1)), 0);
    // entry of exit on edge
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0.0, 0.0, 0.5), V3D(1, 1, 0)), 1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0.0, 0.0, 0.5), V3D(-1, -1, 0)), -1);

    // not on the normal
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0.0, 0.5, 0.5), V3D(0.5, 0.5, 0)), 1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(1.0, 0.5, 0.5), V3D(0.5, 0.5, 0)), -1);
  }

  void testCalcValidOctahedron() {
    auto geom_obj = createOctahedron();
    // entry or exit on the normal
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0.2, 0.3, 0.5), V3D(1, 1, 1)), -1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0.2, 0.3, 0.5), V3D(-1, -1, -1)), 1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(-0.2, -0.3, -0.5), V3D(1, 1, 1)), 1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(-0.2, -0.3, -0.5), V3D(-1, -1, -1)), -1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0.5, 0.2, -0.3), V3D(1, 1, -1)), -1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0.5, 0.2, -0.3), V3D(-1, -1, 1)), 1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(-0.5, -0.2, 0.3), V3D(1, 1, -1)), 1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(-0.5, -0.2, 0.3), V3D(-1, -1, 1)), -1);

    // glancing blow on edge
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0.0, 0.5, 0.5), V3D(1, 0, 0)), 0);
    // entry or exit at edge
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0.0, -0.5, 0.5), V3D(0, 1, 0)), 1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0.0, 0.5, 0.5), V3D(0, 1, 0)), -1);

    // not on the normal
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0.2, 0.3, 0.5), V3D(0, 1, 0)), -1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0.2, -0.3, 0.5), V3D(0, 1, 0)), 1);
  }

  void testCalcValidTypeLShape() {
    auto geom_obj = createLShape();
    // entry or exit on the normal
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0.0, 1.5, 0.5), V3D(1, 0, 0)), 1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0.0, 1.5, 0.5), V3D(-1, 0, 0)), -1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(1.0, 1.5, 0.5), V3D(1, 0, 0)), -1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(1.0, 1.5, 0.5), V3D(-1, 0, 0)), 1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0.5, 2.0, 0.5), V3D(0, 1, 0)), -1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0.5, 2.0, 0.5), V3D(0, -1, 0)), 1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0.5, 0.0, 0.5), V3D(0, 1, 0)), 1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0.5, 0.0, 0.5), V3D(0, -1, 0)), -1);

    // glancing blow on edge
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0.0, 0.0, 0.5), V3D(1, -1, 0)), 0);
    // glancing blow on edge from inside
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(1.0, 1.0, 0.5), V3D(1, -1, 0)), 0);

    // not on the normal
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(1.0, 1.5, 0.5), V3D(0.5, 0.5, 0)), -1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(1.0, 1.5, 0.5), V3D(-0.5, 0.5, 0)), 1);
  }

  void testFindPointInCube() {
    auto geom_obj = createCube(1.0);
    Kernel::V3D pt;
    TS_ASSERT_EQUALS(geom_obj->getPointInObject(pt), 1);
    TS_ASSERT_LESS_THAN_EQUALS(0.0, pt.X());
    TS_ASSERT_LESS_THAN_EQUALS(pt.X(), 1.0);
    TS_ASSERT_LESS_THAN_EQUALS(0.0, pt.Y());
    TS_ASSERT_LESS_THAN_EQUALS(pt.Y(), 1.0);
    TS_ASSERT_LESS_THAN_EQUALS(0.0, pt.Z());
    TS_ASSERT_LESS_THAN_EQUALS(pt.Z(), 1.0);
  }

  void testFindPointInOctahedron() {
    auto geom_obj = createOctahedron();
    Kernel::V3D pt;
    TS_ASSERT_EQUALS(geom_obj->getPointInObject(pt), 1);
    TS_ASSERT_LESS_THAN_EQUALS(fabs(pt.X()) + fabs(pt.Y()) + fabs(pt.Z()), 1.0);
  }

  void testFindPointInLShape() {
    auto geom_obj = createLShape();
    Kernel::V3D pt;
    TS_ASSERT_EQUALS(geom_obj->getPointInObject(pt), 1);
    TS_ASSERT_LESS_THAN_EQUALS(0.0, pt.X());
    TS_ASSERT_LESS_THAN_EQUALS(pt.X(), 2.0);
    TS_ASSERT_LESS_THAN_EQUALS(0.0, pt.Y());
    TS_ASSERT_LESS_THAN_EQUALS(pt.Y(), 2.0);
    TS_ASSERT_LESS_THAN_EQUALS(0.0, pt.Z());
    TS_ASSERT_LESS_THAN_EQUALS(pt.Z(), 1.0);
    TS_ASSERT(pt.X() <= 1.0 || pt.Y() <= 1.0)
  }

  void testGeneratePointInside() {
    using namespace ::testing;

    // Generate "random" sequence
    MockRNG rng;
    Sequence rand;
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(0.45));
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(0.55));
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(0.65));

    //  Random sequence set up so as to give point (0.90, 1.10, 0.65)
    auto geom_obj = createLShape();
    size_t maxAttempts(1);
    std::optional<Kernel::V3D> point;
    TS_ASSERT_THROWS_NOTHING(point = geom_obj->generatePointInObject(rng, maxAttempts));

    const double tolerance(1e-10);
    TS_ASSERT_DELTA(0.90, point->X(), tolerance);
    TS_ASSERT_DELTA(1.10, point->Y(), tolerance);
    TS_ASSERT_DELTA(0.65, point->Z(), tolerance);
  }

  void testGeneratePointInsideRespectsMaxAttempts() {
    using namespace ::testing;

    // Generate "random" sequence
    MockRNG rng;
    Sequence rand;
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(0.1));
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(0.2));
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(0.3));

    //  Random sequence set up so as to give point (-0.8, -0.6, -0.4),
    //  which is outside the octahedron
    auto geom_obj = createOctahedron();
    size_t maxAttempts(1);
    std::optional<V3D> point;
    point = geom_obj->generatePointInObject(rng, maxAttempts);
    TS_ASSERT_EQUALS(!point, true);
  }

  void testVolumeOfCube() {
    double size = 3.7;
    auto geom_obj = createCube(size);
    TS_ASSERT_DELTA(geom_obj->volume(), size * size * size, 1e-6)
  }

  void testVolumeOfOctahedron() {
    auto geom_obj = createOctahedron();
    TS_ASSERT_DELTA(geom_obj->volume(), 4.0 / 3.0, 1e-3)
  }

  void testVolumeOfLShape() {
    auto geom_obj = createLShape();
    TS_ASSERT_DELTA(geom_obj->volume(), 3.0, 1e-6)
    // 3.5 is the volume of the convex hull
    // 4.0 is the volume of the bounding box
  }

  void testSolidAngleCube()
  /**
  Test solid angle calculation for a cube.
  */
  {
    auto geom_obj = createCube(1.0); // Cube centre at 0.5, 0.5, 0.5
    double satol = 1e-3;             // tolerance for solid angle
    // solid angle at distance 0.5 should be 4pi/6 by symmetry

    TS_ASSERT_DELTA(geom_obj->solidAngle(V3D(1.5, 0.5, 0.5)), M_PI * 2.0 / 3.0, satol);
    TS_ASSERT_DELTA(geom_obj->solidAngle(V3D(-0.5, 0.5, 0.5)), M_PI * 2.0 / 3.0, satol);
    TS_ASSERT_DELTA(geom_obj->solidAngle(V3D(0.5, 1.5, 0.5)), M_PI * 2.0 / 3.0, satol);
    TS_ASSERT_DELTA(geom_obj->solidAngle(V3D(0.5, -0.5, 0.5)), M_PI * 2.0 / 3.0, satol);
    TS_ASSERT_DELTA(geom_obj->solidAngle(V3D(0.5, 0.5, 1.5)), M_PI * 2.0 / 3.0, satol);
    TS_ASSERT_DELTA(geom_obj->solidAngle(V3D(0.5, 0.5, -0.5)), M_PI * 2.0 / 3.0, satol);
  }

  void testSolidAngleScaledCube()
  /**
  Test solid angle calculation for a cube that is scaled.
  */
  {
    auto geom_obj = createCube(2.0);
    auto scale = V3D(0.5, 0.5, 0.5);
    double satol = 1e-3; // tolerance for solid angle
    // solid angle at distance 0.5 should be 4pi/6 by symmetry

    TS_ASSERT_DELTA(geom_obj->solidAngle(V3D(1.5, 0.5, 0.5), scale), M_PI * 2.0 / 3.0, satol);
    TS_ASSERT_DELTA(geom_obj->solidAngle(V3D(-0.5, 0.5, 0.5), scale), M_PI * 2.0 / 3.0, satol);
    TS_ASSERT_DELTA(geom_obj->solidAngle(V3D(0.5, 1.5, 0.5), scale), M_PI * 2.0 / 3.0, satol);
    TS_ASSERT_DELTA(geom_obj->solidAngle(V3D(0.5, -0.5, 0.5), scale), M_PI * 2.0 / 3.0, satol);
    TS_ASSERT_DELTA(geom_obj->solidAngle(V3D(0.5, 0.5, 1.5), scale), M_PI * 2.0 / 3.0, satol);
    TS_ASSERT_DELTA(geom_obj->solidAngle(V3D(0.5, 0.5, -0.5), scale), M_PI * 2.0 / 3.0, satol);
  }

  void testOutputForRendering()
  /* Here we test the output functions used in rendering */
  {
    auto geom_obj = createOctahedron();
    TS_ASSERT_EQUALS(geom_obj->numberOfTriangles(), 8);
    TS_ASSERT_EQUALS(geom_obj->numberOfVertices(), 6);
    TS_ASSERT_THROWS_NOTHING(geom_obj->getTriangles());
    TS_ASSERT_THROWS_NOTHING(geom_obj->getVertices());
  }

  void testRotation()
  /* Test Rotating a mesh */
  {
    auto lShape = createLShape();
    const double valueList[] = {0, -1, 0, 1, 0, 0, 0, 0, 1};
    const std::vector<double> rotationMatrix = std::vector<double>(std::begin(valueList), std::end(valueList));
    const Kernel::Matrix<double> rotation = Kernel::Matrix<double>(rotationMatrix);

    const double checkList[] = {0, 0, 0, 0, 2, 0, -1, 2, 0, -1, 1, 0, -2, 1, 0, -2, 0, 0,
                                0, 0, 1, 0, 2, 1, -1, 2, 1, -1, 1, 1, -2, 1, 1, -2, 0, 1};
    auto checkVector = std::vector<double>(std::begin(checkList), std::end(checkList));

    TS_ASSERT_THROWS_NOTHING(lShape->rotate(rotation));
    auto rotated = lShape->getVertices();
    TS_ASSERT_DELTA(rotated, checkVector, 1e-8);
  }
  void testTranslation()
  /* Test Translating a mesh */
  {
    auto octahedron = createOctahedron();
    V3D translation = V3D(1, 2, 3);
    const double checkList[] = {2, 2, 3, 1, 3, 3, 1, 2, 4, 0, 2, 3, 1, 1, 3, 1, 2, 2};
    auto checkVector = std::vector<double>(std::begin(checkList), std::end(checkList));
    TS_ASSERT_THROWS_NOTHING(octahedron->translate(translation));
    auto moved = octahedron->getVertices();
    TS_ASSERT_DELTA(moved, checkVector, 1e-8);
  }
};

// -----------------------------------------------------------------------------
// Performance tests
// -----------------------------------------------------------------------------
class MeshObjectTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MeshObjectTestPerformance *createSuite() { return new MeshObjectTestPerformance(); }
  static void destroySuite(MeshObjectTestPerformance *suite) { delete suite; }

  MeshObjectTestPerformance()
      : rng(200000), octahedron(createOctahedron()), lShape(createLShape()), smallCube(createCube(0.2)) {
    testPoints = create_test_points();
    testRays = create_test_rays();
    translation = create_translation_vector();
    rotation = create_rotation_matrix();
  }

  void test_rotate(const Kernel::Matrix<double> &) {
    const size_t number(10000);
    for (size_t i = 0; i < number; ++i) {
      octahedron->rotate(rotation);
    }
  }

  void test_translate(Kernel::V3D) {
    const size_t number(10000);
    for (size_t i = 0; i < number; ++i) {
      octahedron->translate(translation);
    }
  }

  void test_isOnSide() {
    const size_t number(10000);
    for (size_t i = 0; i < number; ++i) {
      octahedron->isOnSide(testPoints[i % testPoints.size()]);
    }
  }

  void test_isValid() {
    const size_t number(10000);
    for (size_t i = 0; i < number; ++i) {
      octahedron->isValid(testPoints[i % testPoints.size()]);
    }
  }

  void test_calcValidType() {
    const size_t number(10000);
    for (size_t i = 0; i < number; ++i) {
      size_t j = i % testRays.size();
      octahedron->calcValidType(testRays[j].startPoint(), testRays[j].direction());
    }
  }

  void test_interceptSurface() {
    const size_t number(10000);
    Track testRay;
    for (size_t i = 0; i < number; ++i) {
      octahedron->interceptSurface(testRays[i % testRays.size()]);
    }
  }

  void test_solid_angle() {
    const size_t number(10000);
    for (size_t i = 0; i < number; ++i) {
      smallCube->solidAngle(testPoints[i % testPoints.size()]);
    }
  }

  void test_solid_angle_scaled() {
    const size_t number(10000);
    for (size_t i = 0; i < number; ++i) {
      smallCube->solidAngle(testPoints[i % testPoints.size()], V3D(0.5, 1.33, 1.5));
    }
  }

  void test_volume() {
    const size_t numberOfRuns(10000);
    for (size_t i = 0; i < numberOfRuns; ++i) {
      octahedron->volume();
      lShape->volume();
    }
  }

  void test_getPointInObject() {
    const size_t numberOfRuns(1000);
    V3D pDummy;
    for (size_t i = 0; i < numberOfRuns; ++i) {
      octahedron->getPointInObject(pDummy);
      lShape->getPointInObject(pDummy);
      smallCube->getPointInObject(pDummy);
    }
  }

  void test_generatePointInside_Convex_Solid() {
    const size_t npoints(6000);
    const size_t maxAttempts(500);
    std::optional<V3D> point;
    for (size_t i = 0; i < npoints; ++i) {
      point = octahedron->generatePointInObject(rng, maxAttempts);
    }
  }

  void test_generatePointInside_NonConvex_Solid() {
    const size_t npoints(6000);
    const size_t maxAttempts(500);
    std::optional<V3D> point;
    for (size_t i = 0; i < npoints; ++i) {
      point = lShape->generatePointInObject(rng, maxAttempts);
    }
  }

  void test_output_for_rendering() {
    const size_t numberOfRuns(30000);
    for (size_t i = 0; i < numberOfRuns; ++i) {
      octahedron->getVertices();
      octahedron->getTriangles();
      lShape->getVertices();
      lShape->getTriangles();
    }
  }

  V3D create_test_point(size_t index, size_t dimension) {
    // Create a test point with coordinates within [-1.0, 0.0]
    // for applying to octahedron
    V3D output;
    double interval = 1.0 / static_cast<double>(dimension - 1);
    output.setX((static_cast<double>(index % dimension)) * interval);
    index /= dimension;
    output.setY((static_cast<double>(index % dimension)) * interval);
    index /= dimension;
    output.setZ((static_cast<double>(index % dimension)) * interval);
    return output;
  }

  std::vector<V3D> create_test_points() {
    // Create a vector of test points
    size_t dim = 5;
    size_t num = dim * dim * dim;
    std::vector<V3D> output;
    output.reserve(num);
    for (size_t i = 0; i < num; ++i) {
      output.emplace_back(create_test_point(i, dim));
    }
    return output;
  }

  Track create_test_ray(size_t index, size_t startDim, size_t dirDim) {
    // create a test ray
    const V3D shift(0.01, -1.0 / 77, -1.0 / 117);
    V3D startPoint = create_test_point(index, startDim);
    V3D direction = V3D(0.0, 0.0, 0.0) - create_test_point(index, dirDim);
    direction += shift; // shift to avoid divide by zero error
    direction.normalize();
    return Track(startPoint, direction);
  }

  std::vector<Track> create_test_rays() {
    size_t sDim = 3;
    size_t dDim = 2;
    size_t num = sDim * sDim * sDim * dDim * dDim * dDim;
    std::vector<Track> output;
    output.reserve(num);
    for (size_t i = 0; i < num; ++i) {
      output.emplace_back(create_test_ray(i, sDim, dDim));
    }
    return output;
  }

  V3D create_translation_vector() {
    V3D translate = Kernel::V3D(5, 5, 15);
    return translate;
  }

  Kernel::Matrix<double> create_rotation_matrix() {
    double valueList[] = {0, -1, 0, 1, 0, 0, 0, 0, 1};
    const std::vector<double> rotationMatrix = std::vector<double>(std::begin(valueList), std::end(valueList));
    Kernel::Matrix<double> rotation = Kernel::Matrix<double>(rotationMatrix);
    return rotation;
  }

private:
  Mantid::Kernel::MersenneTwister rng;
  std::unique_ptr<MeshObject> octahedron;
  std::unique_ptr<MeshObject> lShape;
  std::unique_ptr<MeshObject> smallCube;
  std::vector<V3D> testPoints;
  std::vector<Track> testRays;
  V3D translation;
  Kernel::Matrix<double> rotation;
};
