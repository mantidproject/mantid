#ifndef MANTID_TESTMESHOBJECT__
#define MANTID_TESTMESHOBJECT__

#include "MantidGeometry/Objects/MeshObject.h"

#include "MantidGeometry/Math/Algebra.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidGeometry/Rendering/CacheGeometryHandler.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/make_unique.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/MersenneTwister.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

#include <cxxtest/TestSuite.h>
#include <cmath>
#include <ostream>
#include <vector>
#include <algorithm>
#include <ctime>

#include "boost/shared_ptr.hpp"
#include "boost/make_shared.hpp"

#include <gmock/gmock.h>
#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/Document.h>

using namespace Mantid;
using namespace Geometry;
using Mantid::Kernel::V3D;

namespace {
// -----------------------------------------------------------------------------
// Mock Random Number Generator
// -----------------------------------------------------------------------------
class MockRNG final : public Mantid::Kernel::PseudoRandomNumberGenerator {
public:
  GCC_DIAG_OFF_SUGGEST_OVERRIDE
  MOCK_METHOD0(nextValue, double());
  MOCK_METHOD2(nextValue, double(double, double));
  MOCK_METHOD2(nextInt, int(int, int));
  MOCK_METHOD0(restart, void());
  MOCK_METHOD0(save, void());
  MOCK_METHOD0(restore, void());
  MOCK_METHOD1(setSeed, void(size_t));
  MOCK_METHOD2(setRange, void(const double, const double));
  MOCK_CONST_METHOD0(min, double());
  MOCK_CONST_METHOD0(max, double());
  GCC_DIAG_ON_SUGGEST_OVERRIDE
};
}

class MeshObjectTest : public CxxTest::TestSuite {

public:
  void testInitialize() {
    MeshObject obj;

    std::vector<V3D> vertices;
    vertices.push_back(V3D(0, 0, 0));
    vertices.push_back(V3D(1, 0, 0));
    vertices.push_back(V3D(0, 1, 0));
    vertices.push_back(V3D(0, 0, 1));

    std::vector<int> triangles;
    // face
    triangles.push_back(1);
    triangles.push_back(2);
    triangles.push_back(3);
    // face
    triangles.push_back(2);
    triangles.push_back(1);
    triangles.push_back(0);
    // face
    triangles.push_back(3);
    triangles.push_back(0);
    triangles.push_back(1);
    // face
    triangles.push_back(0);
    triangles.push_back(3);
    triangles.push_back(2);

    // Test initialize works first time
    TS_ASSERT_THROWS_NOTHING(obj.initialize(triangles,vertices));
    // but connot be done again
    TS_ASSERT_THROWS_ANYTHING(obj.initialize(triangles, vertices));

  }

  void testClone() {
    MeshObject blank;

    // Test unitialized object cannot be cloned
    TS_ASSERT_THROWS_ANYTHING(blank.clone());

    // and an initialized object can be cloned
    IObject_sptr geom_obj = createOctahedron();
    TS_ASSERT_THROWS_NOTHING(geom_obj->clone());
  }

  void testDefaultObjectHasEmptyMaterial() {
    MeshObject obj;

    TSM_ASSERT_DELTA("Expected a zero number density", 0.0,
                     obj.material().numberDensity(), 1e-12);
  }

  void testObjectSetMaterialReplacesExisting() {
    using Mantid::Kernel::Material;
    MeshObject obj;

    TSM_ASSERT_DELTA("Expected a zero number density", 0.0,
                     obj.material().numberDensity(), 1e-12);
    obj.setMaterial(
        Material("arm", PhysicalConstants::getNeutronAtom(13), 45.0));
    TSM_ASSERT_DELTA("Expected a number density of 45", 45.0,
                     obj.material().numberDensity(), 1e-12);
  }

  void testCopyConstructorGivesObjectWithSameAttributes() {
    auto original_ptr = createCube(1.0);
    auto &original = dynamic_cast<MeshObject &>(*original_ptr);
    original.setID("sp-1");
    TS_ASSERT(boost::dynamic_pointer_cast<CacheGeometryHandler>(
      original.getGeometryHandler()));

    MeshObject copy(original);
    // The copy should be a primitive object with a CacheGeometryHandler

    TS_ASSERT_EQUALS("sp-1", copy.id());
    TS_ASSERT(boost::dynamic_pointer_cast<CacheGeometryHandler>(
      copy.getGeometryHandler()));
    TS_ASSERT_EQUALS(copy.getName(), original.getName());
    TS_ASSERT_EQUALS(copy.numberOfVertices(), original.numberOfVertices());
    TS_ASSERT_EQUALS(copy.numberOfTriangles(), original.numberOfTriangles());
  }

  void testAssignmentOperatorGivesObjectWithSameAttributes() {
    auto original_ptr = createCube(1.0);
    auto &original = dynamic_cast<MeshObject &>(*original_ptr);
    original.setID("sp-1");
    int objType(-1);
    double radius(-1.0), height(-1.0);
    std::vector<V3D> pts;
    original.GetObjectGeom(objType, pts, radius, height);
    TS_ASSERT(boost::dynamic_pointer_cast<CacheGeometryHandler>(
      original.getGeometryHandler())); 

    MeshObject lhs;  // initialize
    lhs = original; // assign
                    // The copy should be a primitive object with a GluGeometryHandler
    objType = -1;
    lhs.GetObjectGeom(objType, pts, radius, height);

    TS_ASSERT_EQUALS("sp-1", lhs.id());
    TS_ASSERT(boost::dynamic_pointer_cast<CacheGeometryHandler>(
      lhs.getGeometryHandler())); 
  }

  void testHasValidShape() {
    auto empty_obj = new MeshObject();
    auto geom_obj = createCube(1.0);
    TS_ASSERT(!empty_obj->hasValidShape() );
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
    IObject_sptr geom_obj = createCube(4.0);
    Track track(V3D(-10, 1, 1), V3D(1, 0, 0));

    // format = startPoint, endPoint, total distance so far
    expectedResults.push_back(
      Link(V3D(0, 1, 1), V3D(4, 1, 1), 14.0, *geom_obj));
    checkTrackIntercept(geom_obj, track, expectedResults);
  }

  void testInterceptCubeXY() {
    std::vector<Link> expectedResults;
    IObject_sptr geom_obj = createCube(4.0);
    Track track(V3D(-8, -6, 1), V3D(0.8, 0.6, 0));

    // format = startPoint, endPoint, total distance so far
    expectedResults.push_back(
      Link(V3D(0, 0, 1), V3D(4, 3, 1), 15.0, *geom_obj));
    checkTrackIntercept(geom_obj, track, expectedResults);
  }

  void testInterceptCubeMiss() {
    std::vector<Link>
      expectedResults; // left empty as there are no expected results
    IObject_sptr geom_obj = createCube(4.0);
    Track track(V3D(-10, 0, 0), V3D(1, 1, 0));

    checkTrackIntercept(geom_obj, track, expectedResults);
  }

  void testInterceptOctahedronX() {
    std::vector<Link> expectedResults;
    IObject_sptr geom_obj = createOctahedron();
    Track track(V3D(-10, 0.2, 0.2), V3D(1, 0, 0));

    // format = startPoint, endPoint, total distance so far
    expectedResults.push_back(
      Link(V3D(-0.6, 0.2, 0.2), V3D(0.6, 0.2, 0.2), 10.6, *geom_obj));
    checkTrackIntercept(geom_obj, track, expectedResults);
  }

  void testInterceptOctahedronXthroughEdge() {
    std::vector<Link> expectedResults;
    IObject_sptr geom_obj = createOctahedron();
    Track track(V3D(-10, 0.2, 0.0), V3D(1, 0, 0));

    // format = startPoint, endPoint, total distance so far
    expectedResults.push_back(
      Link(V3D(-0.8, 0.2, 0.0), V3D(0.8, 0.2, 0.0), 10.8, *geom_obj));
    checkTrackIntercept(geom_obj, track, expectedResults);
  }

  void testInterceptOctahedronXthroughVertex() {
    std::vector<Link> expectedResults;
    IObject_sptr geom_obj = createOctahedron();
    Track track(V3D(-10, 0.0, 0.0), V3D(1, 0, 0));

    // format = startPoint, endPoint, total distance so far
    expectedResults.push_back(
      Link(V3D(-1.0, 0.0, 0.0), V3D(1.0, 0.0, 0.0), 11.0, *geom_obj));
    checkTrackIntercept(geom_obj, track, expectedResults);
  }

  void testInterceptLShapeTwoPass() {
    std::vector<Link> expectedResults;
    IObject_sptr geom_obj = createLShape();
    Track track(V3D(0, 2.5, 0.5), V3D(0.707, -0.707, 0));

    // format = startPoint, endPoint, total distance so far
    expectedResults.push_back(
      Link(V3D(0.5, 2, 0.5), V3D(1, 1.5, 0.5), 1.4142135, *geom_obj));
    expectedResults.push_back(
      Link(V3D(1.5, 1, 0.5), V3D(2, 0.5, 0.5), 2.828427, *geom_obj));
    checkTrackIntercept(geom_obj, track, expectedResults);
  }

  void testInterceptLShapeMiss() {
    std::vector<Link>
      expectedResults; // left empty as there are no expected results
    IObject_sptr geom_obj = createLShape();
    // Passes through convex hull of L-Shape
    Track track(V3D(1.1, 1.1, -1), V3D(0, 0, 1));

    checkTrackIntercept(geom_obj, track, expectedResults);
  }

  void testTrackTwoIsolatedCubes()
    /**
    Test a track going through two objects
    */
  {
    IObject_sptr object1 = createCube(2.0, V3D(0.0, 0.0, 0.0));

    IObject_sptr object2 = createCube(2.0, V3D(5.5, 0.0, 0.0));

    Track TL(Kernel::V3D(-5, 0, 0), Kernel::V3D(1, 0, 0));

    // CARE: This CANNOT be called twice
    TS_ASSERT(object1->interceptSurface(TL) != 0);
    TS_ASSERT(object2->interceptSurface(TL) != 0);

    std::vector<Link> expectedResults;
    expectedResults.push_back(Link(V3D(-1, 0, 0), V3D(1, 0, 0), 6, *object1));
    expectedResults.push_back(
      Link(V3D(4.5, 0, 0), V3D(6.5, 0, 0), 11.5, *object2));
    checkTrackIntercept(TL, expectedResults);
  }

  void testTrackTwoTouchingCubes()
    /**
    Test a track going through two objects
    */
  {
    IObject_sptr object1 = createCube(2.0, V3D(0.0, 0.0, 0.0));

    IObject_sptr object2 = createCube(4.0, V3D(3.0, 0.0, 0.0));

    Track TL(Kernel::V3D(-5, 0, 0), Kernel::V3D(1, 0, 0));

    // CARE: This CANNOT be called twice
    TS_ASSERT(object1->interceptSurface(TL) != 0);
    TS_ASSERT(object2->interceptSurface(TL) != 0);

    std::vector<Link> expectedResults;
    expectedResults.push_back(Link(V3D(-1, 0, 0), V3D(1, 0, 0), 6, *object1));
    expectedResults.push_back(
      Link(V3D(1, 0, 0), V3D(5, 0, 0), 10.0, *object2));
    checkTrackIntercept(TL, expectedResults);
  }

  void checkTrackIntercept(Track &track,
    const std::vector<Link> &expectedResults) {
    int index = 0;
    for (Track::LType::const_iterator it = track.cbegin(); it != track.cend();
      ++it) {
      if (index < expectedResults.size()) {
        TS_ASSERT_DELTA(it->distFromStart, expectedResults[index].distFromStart,
          1e-6);
        TS_ASSERT_DELTA(it->distInsideObject,
          expectedResults[index].distInsideObject, 1e-6);
        TS_ASSERT_EQUALS(it->componentID, expectedResults[index].componentID);
        TS_ASSERT_EQUALS(it->entryPoint, expectedResults[index].entryPoint);
        TS_ASSERT_EQUALS(it->exitPoint, expectedResults[index].exitPoint);
      }
      ++index;
    }
    TS_ASSERT_EQUALS(index, static_cast<int>(expectedResults.size()));
  }


  void checkTrackIntercept(IObject_sptr obj, Track &track,
    const std::vector<Link> &expectedResults) {
    int unitCount = obj->interceptSurface(track);
    TS_ASSERT_EQUALS(unitCount, expectedResults.size());
    checkTrackIntercept(track, expectedResults);
  }

  void testIsOnSideCube() {
    IObject_sptr geom_obj = createCube(1.0);
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
    IObject_sptr geom_obj = createCube(1.0);
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
    IObject_sptr geom_obj = createOctahedron();
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
    IObject_sptr geom_obj = createOctahedron();
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
    IObject_sptr geom_obj = createLShape();
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
    IObject_sptr geom_obj = createLShape();
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
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0.0, 0.5, 0.5), V3D(-1, 0, 0)),-1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(1.0, 0.5, 0.5), V3D(1, 0, 0)), -1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(1.0, 0.5, 0.5), V3D(-1, 0, 0)), 1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0.5, 0.0, 0.5), V3D(0, 1, 0)), 1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0.5, 0.0, 0.5), V3D(0, -1, 0)),-1);
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
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(-0.2, -0.3, -0.5), V3D(-1, -1, -1)),-1);
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
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0.5, 0.0, 0.5), V3D(0, -1, 0)),-1);

    // glancing blow on edge
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0.0, 0.0, 0.5), V3D(1, -1, 0)), 0);
    // glancing blow on edge from inside
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(1.0, 1.0, 0.5), V3D(1, -1, 0)), 0);

    // not on the normal
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(1.0, 1.5, 0.5), V3D(0.5, 0.5, 0)), -1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(1.0, 1.5, 0.5), V3D(-0.5, 0.5, 0)), 1);
  }

  void testFindPointInCube()
  {
    auto geom_obj = createCube(1.0);
    Kernel::V3D pt;
    TS_ASSERT_EQUALS(geom_obj->getPointInObject(pt), 1);
    TS_ASSERT_LESS_THAN(0.0, pt.X());
    TS_ASSERT_LESS_THAN(pt.X(),1.0);
    TS_ASSERT_LESS_THAN(0.0, pt.Y());
    TS_ASSERT_LESS_THAN(pt.Y(), 1.0);    
    TS_ASSERT_LESS_THAN(0.0, pt.Z());
    TS_ASSERT_LESS_THAN(pt.Z(), 1.0);
  }

  void testFindPointInOctahedron()
  {
    auto geom_obj = createOctahedron();
    Kernel::V3D pt;
    TS_ASSERT_LESS_THAN(abs(pt.X()) + abs(pt.Y()) + abs(pt.Z()), 1.0)
  }

  void testFindPointInLShape()
  {
    auto geom_obj = createLShape();
    Kernel::V3D pt;
    TS_ASSERT_EQUALS(geom_obj->getPointInObject(pt), 1);
    TS_ASSERT_LESS_THAN(0.0, pt.X());
    TS_ASSERT_LESS_THAN(pt.X(), 2.0);
    TS_ASSERT_LESS_THAN(0.0, pt.Y());
    TS_ASSERT_LESS_THAN(pt.Y(), 2.0);
    TS_ASSERT_LESS_THAN(0.0, pt.Z());
    TS_ASSERT_LESS_THAN(pt.Z(), 1.0);
    TS_ASSERT(pt.X() < 1.0 || pt.Y() < 1.0)
  }

  void testGeneratePointInside() {
    using namespace ::testing;

    // Generate "random" sequence
    MockRNG rng;
    Sequence rand;
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(0.45));
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(0.55));
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(0.65));

    //  Random sequence set up so as to give point (0.90, 1.10, 0.75)
    auto geom_obj = createLShape();
    size_t maxAttempts(1);
    V3D point;
    TS_ASSERT_THROWS_NOTHING(
      point = geom_obj->generatePointInObject(rng, maxAttempts));

    const double tolerance(1e-10);
    TS_ASSERT_DELTA(0.90, point.X(), tolerance);
    TS_ASSERT_DELTA(1.10, point.Y(), tolerance);
    TS_ASSERT_DELTA(0.70, point.Z(), tolerance);
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
    TS_ASSERT_THROWS(geom_obj->generatePointInObject(rng, maxAttempts),
      std::runtime_error);
  }

  void testVolumeOfCube() {
    double size = 3.7;
    auto geom_obj = createCube(size);
    TS_ASSERT_DELTA(geom_obj->volume(), size*size*size, 1e-6)
  }

  void testVolumeOfOctahedron() {
    auto geom_obj = createOctahedron();
    TS_ASSERT_DELTA(geom_obj->volume(), 4.0/3.0, 1e-6)
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
    auto geom_obj = createCube(1.0);
    double satol = 1e-3; // tolerance for solid angle
                         // solid angle at distance 0.5 should be 4pi/6 by symmetry

    TS_ASSERT_DELTA(geom_obj->solidAngle(V3D(1.5, 0.5, 0.5)),
      M_PI * 2.0 / 3.0, satol);
    TS_ASSERT_DELTA(geom_obj->solidAngle(V3D(-0.5, 0.5, 0.5)),
      M_PI * 2.0 / 3.0, satol);
    TS_ASSERT_DELTA(geom_obj->solidAngle(V3D(0.5, 1.5, 0.5)),
      M_PI * 2.0 / 3.0, satol);
    TS_ASSERT_DELTA(geom_obj->solidAngle(V3D(0.5, -0.5, 0.5)),
      M_PI * 2.0 / 3.0, satol);
    TS_ASSERT_DELTA(geom_obj->solidAngle(V3D(0.5, 0.5, 1.5)),
      M_PI * 2.0 / 3.0, satol);
    TS_ASSERT_DELTA(geom_obj->solidAngle(V3D(0.5, 0.5, -0.5)),
      M_PI * 2.0 / 3.0, satol);
  }

private:

  boost::shared_ptr<MeshObject> createCube(const double size, const V3D &centre) {
    /**
    * Create cube of side length size with specified centre,
    * parellel to axes and non-negative vertex coordinates.
    */
    double min = 0.0 - 0.5*size;
    double max = 0.5*size;
    std::vector<V3D> vertices;
    vertices.push_back(centre + V3D(max, max, max));
    vertices.push_back(centre + V3D(min, max, max));
    vertices.push_back(centre + V3D(max, min, max));
    vertices.push_back(centre + V3D(min, min, max));
    vertices.push_back(centre + V3D(max, max, min));
    vertices.push_back(centre + V3D(min, max, min));
    vertices.push_back(centre + V3D(max, min, min));
    vertices.push_back(centre + V3D(min, min, min));

    std::vector<int> triangles;
    // top face of cube - z max
    triangles.push_back(0);
    triangles.push_back(1);
    triangles.push_back(2);
    triangles.push_back(2);
    triangles.push_back(1);
    triangles.push_back(3);
    // right face of cube - x max
    triangles.push_back(0);
    triangles.push_back(2);
    triangles.push_back(4);
    triangles.push_back(4);
    triangles.push_back(2);
    triangles.push_back(6);
    // back face of cube - y max
    triangles.push_back(0);
    triangles.push_back(4);
    triangles.push_back(1);
    triangles.push_back(1);
    triangles.push_back(4);
    triangles.push_back(5);
    // bottom face of cube - z min
    triangles.push_back(7);
    triangles.push_back(5);
    triangles.push_back(6);
    triangles.push_back(6);
    triangles.push_back(5);
    triangles.push_back(4);
    // left face of cube - x min
    triangles.push_back(7);
    triangles.push_back(3);
    triangles.push_back(5);
    triangles.push_back(5);
    triangles.push_back(3);
    triangles.push_back(1);
    // front fact of cube - y min
    triangles.push_back(7);
    triangles.push_back(6);
    triangles.push_back(3);
    triangles.push_back(3);
    triangles.push_back(6);
    triangles.push_back(2);

    boost::shared_ptr<MeshObject> retVal =
      boost::shared_ptr<MeshObject>(new MeshObject);
    retVal->initialize(triangles, vertices);
    return retVal;
  }

  boost::shared_ptr<MeshObject> createCube(const double size) {
    /**
    * Create cube of side length size with vertex at origin,
    * parellel to axes and non-negative vertex coordinates.
    */
    return createCube( size, V3D(0.5*size, 0.5*size, 0.5*size));
  }

  boost::shared_ptr<MeshObject> createOctahedron() {
    /**
    * Create octahedron with vertices on the axes at -1 & +1.
    */
    std::vector<V3D> vertices;
    vertices.push_back(V3D(1, 0, 0));
    vertices.push_back(V3D(0, 1, 0));
    vertices.push_back(V3D(0, 0, 1));
    vertices.push_back(V3D(-1, 0, 0));
    vertices.push_back(V3D(0, -1, 0));
    vertices.push_back(V3D(0, 0, -1));

    std::vector<int> triangles;
    // +++ face
    triangles.push_back(0);
    triangles.push_back(1);
    triangles.push_back(2);
    //++- face
    triangles.push_back(0);
    triangles.push_back(5);
    triangles.push_back(1);
    // +-- face
    triangles.push_back(0);
    triangles.push_back(4);
    triangles.push_back(5);
    // +-+ face
    triangles.push_back(0);
    triangles.push_back(2);
    triangles.push_back(4);
    // --- face
    triangles.push_back(3);
    triangles.push_back(5);
    triangles.push_back(4);
    // --+ face
    triangles.push_back(3);
    triangles.push_back(4);
    triangles.push_back(2);
    // -++ face
    triangles.push_back(3);
    triangles.push_back(2);
    triangles.push_back(1);
    // -+- face
    triangles.push_back(3);
    triangles.push_back(1);
    triangles.push_back(5);


    boost::shared_ptr<MeshObject> retVal =
      boost::shared_ptr<MeshObject>(new MeshObject);
    retVal->initialize(triangles, vertices);
    return retVal;
  }

  boost::shared_ptr<MeshObject> createLShape() {
    /**
    * Create an L shape with vertices at
    * (0,0,Z) (2,0,Z) (2,1,Z) (1,1,Z) (1,2,Z) & (0,2,Z),
    *  where Z = 0 or 1.
    */
    std::vector<V3D> vertices;
    vertices.push_back(V3D(0, 0, 0));
    vertices.push_back(V3D(2, 0, 0));
    vertices.push_back(V3D(2, 1, 0));
    vertices.push_back(V3D(1, 1, 0));
    vertices.push_back(V3D(1, 2, 0));
    vertices.push_back(V3D(0, 2, 0));
    vertices.push_back(V3D(0, 0, 1));
    vertices.push_back(V3D(2, 0, 1));
    vertices.push_back(V3D(2, 1, 1));
    vertices.push_back(V3D(1, 1, 1));
    vertices.push_back(V3D(1, 2, 1));
    vertices.push_back(V3D(0, 2, 1));

    std::vector<int> triangles;
    // z min
    triangles.push_back(0);
    triangles.push_back(1);
    triangles.push_back(5);
    triangles.push_back(5);
    triangles.push_back(3);
    triangles.push_back(4);
    triangles.push_back(3);
    triangles.push_back(1);
    triangles.push_back(2);
    // z max
    triangles.push_back(6);
    triangles.push_back(7);
    triangles.push_back(11);
    triangles.push_back(11);
    triangles.push_back(9);
    triangles.push_back(10);
    triangles.push_back(9);
    triangles.push_back(7);
    triangles.push_back(8);
    // y min
    triangles.push_back(0);
    triangles.push_back(1);
    triangles.push_back(6);
    triangles.push_back(6);
    triangles.push_back(1);
    triangles.push_back(7);
    // x max
    triangles.push_back(1);
    triangles.push_back(2);
    triangles.push_back(7);
    triangles.push_back(7);
    triangles.push_back(2);
    triangles.push_back(8);
    // y mid
    triangles.push_back(2);
    triangles.push_back(3);
    triangles.push_back(8);
    triangles.push_back(8);
    triangles.push_back(3);
    triangles.push_back(9);
    // x mid
    triangles.push_back(3);
    triangles.push_back(4);
    triangles.push_back(9);
    triangles.push_back(9);
    triangles.push_back(4);
    triangles.push_back(10);
    // y max
    triangles.push_back(4);
    triangles.push_back(5);
    triangles.push_back(10);
    triangles.push_back(10);
    triangles.push_back(5);
    triangles.push_back(11);
    // x min
    triangles.push_back(5);
    triangles.push_back(0);
    triangles.push_back(11);
    triangles.push_back(11);
    triangles.push_back(0);
    triangles.push_back(6);

    boost::shared_ptr<MeshObject> retVal =
      boost::shared_ptr<MeshObject>(new MeshObject);
    retVal->initialize(triangles, vertices);
    return retVal;
  }

};


// -----------------------------------------------------------------------------
// Performance tests
// -----------------------------------------------------------------------------
class MeshObjectTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MeshObjectTestPerformance *createSuite() {
    return new MeshObjectTestPerformance();
  }
  static void destroySuite(MeshObjectTestPerformance *suite) { delete suite; }

  MeshObjectTestPerformance()
    : rng(200000), octahedron(createOctahedron()),
    lShape(createLShape()) {}

  void test_generatePointInside_Convex_Solid() {
    const size_t maxAttempts(500);
    V3D dummy;
    for (size_t i = 0; i < npoints; ++i) {
      dummy = octahedron->generatePointInObject(rng, maxAttempts);
    }
  }

  void test_Point_Inside_NonConvex_Solid() {
    const size_t maxAttempts(500);
    V3D dummy;
    for (size_t i = 0; i < npoints; ++i) {
      dummy = lShape->generatePointInObject(rng, maxAttempts);
    }
  }

  boost::shared_ptr<MeshObject> createOctahedron() {
    /**
    * Create octahedron with vertices on the axes at -1 & +1.
    */
    boost::shared_ptr<MeshObject> retVal =
      boost::shared_ptr<MeshObject>(new MeshObject);
    return retVal;
  }

  boost::shared_ptr<MeshObject> createLShape() {
    /**
    * Create an L shape with vertices at
    * (0,0,Z) (2,0,Z) (2,1,Z) (1,1,Z) (1,2,Z) & (0,2,Z),
    *  where Z = 0 or 1.
    */
    boost::shared_ptr<MeshObject> retVal =
      boost::shared_ptr<MeshObject>(new MeshObject);
    return retVal;
  }


private:
  const size_t npoints = 20000;
  Mantid::Kernel::MersenneTwister rng;
  IObject_sptr octahedron;
  IObject_sptr lShape;
};

#endif // MANTID_TESTMESHOBJECT__
