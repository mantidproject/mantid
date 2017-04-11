#ifndef MANTID_TESTOBJECT__
#define MANTID_TESTOBJECT__

#include "MantidGeometry/Objects/Object.h"

#include "MantidGeometry/Surfaces/Cylinder.h"
#include "MantidGeometry/Surfaces/Sphere.h"
#include "MantidGeometry/Surfaces/Plane.h"
#include "MantidGeometry/Math/Algebra.h"
#include "MantidGeometry/Surfaces/SurfaceFactory.h"
#include "MantidGeometry/Objects/Rules.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidGeometry/Rendering/GluGeometryHandler.h"
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

class ObjectTest : public CxxTest::TestSuite {

public:
  void testDefaultObjectHasEmptyMaterial() {
    Object obj;

    TSM_ASSERT_DELTA("Expected a zero number density", 0.0,
                     obj.material().numberDensity(), 1e-12);
  }

  void testObjectSetMaterialReplacesExisting() {
    using Mantid::Kernel::Material;
    Object obj;

    TSM_ASSERT_DELTA("Expected a zero number density", 0.0,
                     obj.material().numberDensity(), 1e-12);
    obj.setMaterial(
        Material("arm", PhysicalConstants::getNeutronAtom(13), 45.0));
    TSM_ASSERT_DELTA("Expected a number density of 45", 45.0,
                     obj.material().numberDensity(), 1e-12);
  }

  void testCopyConstructorGivesObjectWithSameAttributes() {
    Object_sptr original = ComponentCreationHelper::createSphere(1.0);
    original->setID("sp-1");
    int objType(-1);
    double radius(-1.0), height(-1.0);
    std::vector<V3D> pts;
    original->GetObjectGeom(objType, pts, radius, height);
    TS_ASSERT_EQUALS(3, objType);
    TS_ASSERT(boost::dynamic_pointer_cast<GluGeometryHandler>(
        original->getGeometryHandler()));

    Object copy(*original);
    // The copy should be a primitive object with a GluGeometryHandler
    objType = -1;
    copy.GetObjectGeom(objType, pts, radius, height);

    TS_ASSERT_EQUALS("sp-1", copy.id());
    TS_ASSERT_EQUALS(3, objType);
    TS_ASSERT(boost::dynamic_pointer_cast<GluGeometryHandler>(
        copy.getGeometryHandler()));
    TS_ASSERT_EQUALS(copy.getName(), original->getName());
    // Check the string representation is the same
    TS_ASSERT_EQUALS(copy.str(), original->str());
    TS_ASSERT_EQUALS(copy.getSurfaceIndex(), original->getSurfaceIndex());
  }

  void testAssignmentOperatorGivesObjectWithSameAttributes() {
    Object_sptr original = ComponentCreationHelper::createSphere(1.0);
    original->setID("sp-1");
    int objType(-1);
    double radius(-1.0), height(-1.0);
    std::vector<V3D> pts;
    original->GetObjectGeom(objType, pts, radius, height);
    TS_ASSERT_EQUALS(3, objType);
    TS_ASSERT(boost::dynamic_pointer_cast<GluGeometryHandler>(
        original->getGeometryHandler()));

    Object lhs;      // initialize
    lhs = *original; // assign
    // The copy should be a primitive object with a GluGeometryHandler
    objType = -1;
    lhs.GetObjectGeom(objType, pts, radius, height);

    TS_ASSERT_EQUALS("sp-1", lhs.id());
    TS_ASSERT_EQUALS(3, objType);
    TS_ASSERT(boost::dynamic_pointer_cast<GluGeometryHandler>(
        lhs.getGeometryHandler()));
  }

  void testCreateUnitCube() {
    Object_sptr geom_obj = createUnitCube();

    TS_ASSERT_EQUALS(geom_obj->str(), "68 1 -2 3 -4 5 -6");

    double xmin(0.0), xmax(0.0), ymin(0.0), ymax(0.0), zmin(0.0), zmax(0.0);
    geom_obj->getBoundingBox(xmax, ymax, zmax, xmin, ymin, zmin);
  }

  void testIsOnSideCappedCylinder() {
    Object_sptr geom_obj = createCappedCylinder();
    // inside
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0, 0, 0)), false); // origin
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0, 2.9, 0)), false);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0, -2.9, 0)), false);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0, 0, -2.9)), false);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0, 0, 2.9)), false);
    // on the side
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0, 3, 0)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0, -3, 0)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0, 0, -3)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0, 0, 3)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(1.2, 0, 0)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(-3.2, 0, 0)), true);

    // on the edges
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(1.2, 3, 0)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(1.2, -3, 0)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(1.2, 0, -3)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(1.2, 0, 3)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(-3.2, 3, 0)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(-3.2, -3, 0)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(-3.2, 0, -3)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(-3.2, 0, 3)), true);
    // out side
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0, 3.1, 0)), false);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0, -3.1, 0)), false);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0, 0, -3.1)), false);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0, 0, 3.1)), false);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(1.3, 0, 0)), false);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(-3.3, 0, 0)), false);
  }

  void testIsValidCappedCylinder() {
    Object_sptr geom_obj = createCappedCylinder();
    // inside
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0, 0, 0)), true); // origin
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0, 2.9, 0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0, -2.9, 0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0, 0, -2.9)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0, 0, 2.9)), true);
    // on the side
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0, 3, 0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0, -3, 0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0, 0, -3)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0, 0, 3)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(1.2, 0, 0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(-3.2, 0, 0)), true);

    // on the edges
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(1.2, 3, 0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(1.2, -3, 0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(1.2, 0, -3)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(1.2, 0, 3)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(-3.2, 3, 0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(-3.2, -3, 0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(-3.2, 0, -3)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(-3.2, 0, 3)), true);
    // out side
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0, 3.1, 0)), false);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0, -3.1, 0)), false);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0, 0, -3.1)), false);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0, 0, 3.1)), false);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(1.3, 0, 0)), false);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(-3.3, 0, 0)), false);
  }

  void testIsOnSideSphere() {
    Object_sptr geom_obj = ComponentCreationHelper::createSphere(4.1);
    // inside
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0, 0, 0)), false); // origin
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0, 4.0, 0)), false);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0, -4.0, 0)), false);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0, 0, -4.0)), false);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0, 0, 4.0)), false);
    // on the side
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0, 4.1, 0)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0, -4.1, 0)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0, 0, -4.1)), true);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0, 0, 4.1)), true);

    // out side
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0, 4.2, 0)), false);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0, -4.2, 0)), false);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0, 0, -4.2)), false);
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0, 0, 4.2)), false);
  }

  void testIsValidSphere() {
    Object_sptr geom_obj = ComponentCreationHelper::createSphere(4.1);
    // inside
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0, 0, 0)), true); // origin
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0, 4.0, 0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0, -4.0, 0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0, 0, -4.0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0, 0, 4.0)), true);
    // on the side
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0, 4.1, 0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0, -4.1, 0)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0, 0, -4.1)), true);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0, 0, 4.1)), true);

    // out side
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0, 4.2, 0)), false);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0, -4.2, 0)), false);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0, 0, -4.2)), false);
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0, 0, 4.2)), false);
  }

  void testCalcValidTypeSphere() {
    Object_sptr geom_obj = ComponentCreationHelper::createSphere(4.1);
    // entry on the normal
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(-4.1, 0, 0), V3D(1, 0, 0)), 1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(-4.1, 0, 0), V3D(-1, 0, 0)),
                     -1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(4.1, 0, 0), V3D(1, 0, 0)), -1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(4.1, 0, 0), V3D(-1, 0, 0)), 1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0, -4.1, 0), V3D(0, 1, 0)), 1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0, -4.1, 0), V3D(0, -1, 0)),
                     -1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0, 4.1, 0), V3D(0, 1, 0)), -1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0, 4.1, 0), V3D(0, -1, 0)), 1);

    // a glancing blow
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(-4.1, 0, 0), V3D(0, 1, 0)), 0);
    // not quite on the normal
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(-4.1, 0, 0), V3D(0.5, 0.5, 0)),
                     1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(4.1, 0, 0), V3D(0.5, 0.5, 0)),
                     -1);
  }

  void testGetBoundingBoxForSphere() {
    Object_sptr geom_obj = ComponentCreationHelper::createSphere(4.1);
    const double tolerance(1e-10);

    double xmax, ymax, zmax, xmin, ymin, zmin;
    xmax = ymax = zmax = 20;
    xmin = ymin = zmin = -20;
    geom_obj->getBoundingBox(xmax, ymax, zmax, xmin, ymin, zmin);
    TS_ASSERT_DELTA(xmax, 4.1, tolerance);
    TS_ASSERT_DELTA(ymax, 4.1, tolerance);
    TS_ASSERT_DELTA(zmax, 4.1, tolerance);
    TS_ASSERT_DELTA(xmin, -4.1, tolerance);
    TS_ASSERT_DELTA(ymin, -4.1, tolerance);
    TS_ASSERT_DELTA(zmin, -4.1, tolerance);

    const BoundingBox &bbox = geom_obj->getBoundingBox();

    TS_ASSERT_DELTA(bbox.xMax(), 4.1, tolerance);
    TS_ASSERT_DELTA(bbox.yMax(), 4.1, tolerance);
    TS_ASSERT_DELTA(bbox.zMax(), 4.1, tolerance);
    TS_ASSERT_DELTA(bbox.xMin(), -4.1, tolerance);
    TS_ASSERT_DELTA(bbox.yMin(), -4.1, tolerance);
    TS_ASSERT_DELTA(bbox.zMin(), -4.1, tolerance);
  }

  void testCalcValidTypeCappedCylinder() {
    Object_sptr geom_obj = createCappedCylinder();
    // entry on the normal
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(-3.2, 0, 0), V3D(1, 0, 0)), 1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(-3.2, 0, 0), V3D(-1, 0, 0)),
                     -1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(1.2, 0, 0), V3D(1, 0, 0)), -1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(1.2, 0, 0), V3D(-1, 0, 0)), 1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0, -3, 0), V3D(0, 1, 0)), 1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0, -3, 0), V3D(0, -1, 0)), -1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0, 3, 0), V3D(0, 1, 0)), -1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0, 3, 0), V3D(0, -1, 0)), 1);

    // a glancing blow
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(-3.2, 0, 0), V3D(0, 1, 0)), 0);
    // not quite on the normal
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(-3.2, 0, 0), V3D(0.5, 0.5, 0)),
                     1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(1.2, 0, 0), V3D(0.5, 0.5, 0)),
                     -1);
  }

  void testInterceptSurfaceSphereZ() {
    std::vector<Link> expectedResults;
    std::string S41 = "s 1 1 1 4"; // Sphere at (1,1,1) radius 4

    // First create some surfaces
    std::map<int, boost::shared_ptr<Surface>> SphSurMap;
    SphSurMap[41] = boost::make_shared<Sphere>();
    SphSurMap[41]->setSurface(S41);
    SphSurMap[41]->setName(41);

    // A sphere
    std::string ObjSphere = "-41";

    Object_sptr geom_obj = Object_sptr(new Object);
    geom_obj->setObject(41, ObjSphere);
    geom_obj->populate(SphSurMap);

    Track track(V3D(-1, 1.5, 1), V3D(1, 0, 0));

    // format = startPoint, endPoint, total distance so far
    // forward only intercepts means that start point should be track origin
    expectedResults.push_back(Link(V3D(-1, 1.5, 1),
                                   V3D(sqrt(16 - 0.25) + 1, 1.5, 1.0),
                                   sqrt(15.75) + 2, *geom_obj));

    checkTrackIntercept(geom_obj, track, expectedResults);
  }

  void testInterceptSurfaceSphereY() {
    std::vector<Link> expectedResults;
    Object_sptr geom_obj = ComponentCreationHelper::createSphere(4.1);
    Track track(V3D(0, -10, 0), V3D(0, 1, 0));

    // format = startPoint, endPoint, total distance so far
    expectedResults.push_back(
        Link(V3D(0, -4.1, 0), V3D(0, 4.1, 0), 14.1, *geom_obj));

    checkTrackIntercept(geom_obj, track, expectedResults);
  }

  void testInterceptSurfaceSphereX() {
    std::vector<Link> expectedResults;
    Object_sptr geom_obj = ComponentCreationHelper::createSphere(4.1);
    Track track(V3D(-10, 0, 0), V3D(1, 0, 0));

    // format = startPoint, endPoint, total distance so far
    expectedResults.push_back(
        Link(V3D(-4.1, 0, 0), V3D(4.1, 0, 0), 14.1, *geom_obj));
    checkTrackIntercept(geom_obj, track, expectedResults);
  }

  void testInterceptSurfaceCappedCylinderY() {
    std::vector<Link> expectedResults;
    Object_sptr geom_obj = createCappedCylinder();
    // format = startPoint, endPoint, total distance so far
    expectedResults.push_back(Link(V3D(0, -3, 0), V3D(0, 3, 0), 13, *geom_obj));

    Track track(V3D(0, -10, 0), V3D(0, 1, 0));
    checkTrackIntercept(geom_obj, track, expectedResults);
  }

  void testInterceptSurfaceCappedCylinderX() {
    std::vector<Link> expectedResults;
    Object_sptr geom_obj = createCappedCylinder();
    Track track(V3D(-10, 0, 0), V3D(1, 0, 0));

    // format = startPoint, endPoint, total distance so far
    expectedResults.push_back(
        Link(V3D(-3.2, 0, 0), V3D(1.2, 0, 0), 11.2, *geom_obj));
    checkTrackIntercept(geom_obj, track, expectedResults);
  }

  void testInterceptSurfaceCappedCylinderMiss() {
    std::vector<Link>
        expectedResults; // left empty as there are no expected results
    Object_sptr geom_obj = createCappedCylinder();
    Track track(V3D(-10, 0, 0), V3D(1, 1, 0));

    checkTrackIntercept(geom_obj, track, expectedResults);
  }

  void checkTrackIntercept(Track &track,
                           const std::vector<Link> &expectedResults) {
    int index = 0;
    for (Track::LType::const_iterator it = track.cbegin(); it != track.cend();
         ++it) {
      TS_ASSERT_DELTA(it->distFromStart, expectedResults[index].distFromStart,
                      1e-6);
      TS_ASSERT_DELTA(it->distInsideObject,
                      expectedResults[index].distInsideObject, 1e-6);
      TS_ASSERT_EQUALS(it->componentID, expectedResults[index].componentID);
      TS_ASSERT_EQUALS(it->entryPoint, expectedResults[index].entryPoint);
      TS_ASSERT_EQUALS(it->exitPoint, expectedResults[index].exitPoint);
      ++index;
    }
    TS_ASSERT_EQUALS(index, static_cast<int>(expectedResults.size()));
  }

  void checkTrackIntercept(Object_sptr obj, Track &track,
                           const std::vector<Link> &expectedResults) {
    int unitCount = obj->interceptSurface(track);
    TS_ASSERT_EQUALS(unitCount, expectedResults.size());
    checkTrackIntercept(track, expectedResults);
  }

  void testTrackTwoIsolatedCubes()
  /**
  Test a track going through an object
  */
  {
    std::string ObjA = "60001 -60002 60003 -60004 60005 -60006";
    std::string ObjB = "80001 -80002 60003 -60004 60005 -60006";

    createSurfaces(ObjA);
    Object object1 = Object();
    object1.setObject(3, ObjA);
    object1.populate(SMap);

    createSurfaces(ObjB);
    Object object2 = Object();
    object2.setObject(4, ObjB);
    object2.populate(SMap);

    Track TL(Kernel::V3D(-5, 0, 0), Kernel::V3D(1, 0, 0));

    // CARE: This CANNOT be called twice
    TS_ASSERT(object1.interceptSurface(TL) != 0);
    TS_ASSERT(object2.interceptSurface(TL) != 0);

    std::vector<Link> expectedResults;
    expectedResults.push_back(Link(V3D(-1, 0, 0), V3D(1, 0, 0), 6, object1));
    expectedResults.push_back(
        Link(V3D(4.5, 0, 0), V3D(6.5, 0, 0), 11.5, object2));
    checkTrackIntercept(TL, expectedResults);
  }

  void testTrackTwoTouchingCubes()
  /**
  Test a track going through an object
  */
  {
    std::string ObjA = "60001 -60002 60003 -60004 60005 -60006";
    std::string ObjB = "60002 -80002 60003 -60004 60005 -60006";

    createSurfaces(ObjA);
    Object object1 = Object();
    object1.setObject(3, ObjA);
    object1.populate(SMap);

    createSurfaces(ObjB);
    Object object2 = Object();
    object2.setObject(4, ObjB);
    object2.populate(SMap);

    Track TL(Kernel::V3D(-5, 0, 0), Kernel::V3D(1, 0, 0));

    // CARE: This CANNOT be called twice
    TS_ASSERT(object1.interceptSurface(TL) != 0);
    TS_ASSERT(object2.interceptSurface(TL) != 0);

    std::vector<Link> expectedResults;
    expectedResults.push_back(Link(V3D(-1, 0, 0), V3D(1, 0, 0), 6, object1));
    expectedResults.push_back(
        Link(V3D(1, 0, 0), V3D(6.5, 0, 0), 11.5, object2));

    checkTrackIntercept(TL, expectedResults);
  }

  void testTrackCubeWithInternalSphere()
  /**
  Test a track going through an object
  */
  {
    std::string ObjA = "60001 -60002 60003 -60004 60005 -60006 71";
    std::string ObjB = "-71";

    createSurfaces(ObjA);
    Object object1 = Object();
    object1.setObject(3, ObjA);
    object1.populate(SMap);

    createSurfaces(ObjB);
    Object object2 = Object();
    object2.setObject(4, ObjB);
    object2.populate(SMap);

    Track TL(Kernel::V3D(-5, 0, 0), Kernel::V3D(1, 0, 0));

    // CARE: This CANNOT be called twice
    TS_ASSERT(object1.interceptSurface(TL) != 0);
    TS_ASSERT(object2.interceptSurface(TL) != 0);

    std::vector<Link> expectedResults;
    expectedResults.push_back(
        Link(V3D(-1, 0, 0), V3D(-0.8, 0, 0), 4.2, object1));
    expectedResults.push_back(
        Link(V3D(-0.8, 0, 0), V3D(0.8, 0, 0), 5.8, object1));
    expectedResults.push_back(Link(V3D(0.8, 0, 0), V3D(1, 0, 0), 6, object2));
    checkTrackIntercept(TL, expectedResults);
  }

  void testTrack_CubePlusInternalEdgeTouchSpheres()
  /**
  Test a track going through an object
  */
  {
    std::string ObjA = "60001 -60002 60003 -60004 60005 -60006 72 73";
    std::string ObjB = "(-72 : -73)";

    createSurfaces(ObjA);
    Object object1 = Object();
    object1.setObject(3, ObjA);
    object1.populate(SMap);

    createSurfaces(ObjB);
    Object object2 = Object();
    object2.setObject(4, ObjB);
    object2.populate(SMap);

    Track TL(Kernel::V3D(-5, 0, 0), Kernel::V3D(1, 0, 0));

    // CARE: This CANNOT be called twice
    TS_ASSERT(object1.interceptSurface(TL) != 0);
    TS_ASSERT(object2.interceptSurface(TL) != 0);

    std::vector<Link> expectedResults;
    expectedResults.push_back(
        Link(V3D(-1, 0, 0), V3D(-0.4, 0, 0), 4.6, object1));
    expectedResults.push_back(
        Link(V3D(-0.4, 0, 0), V3D(0.2, 0, 0), 5.2, object1));
    expectedResults.push_back(Link(V3D(0.2, 0, 0), V3D(1, 0, 0), 6, object2));
    checkTrackIntercept(TL, expectedResults);
  }

  void testTrack_CubePlusInternalEdgeTouchSpheresMiss()
  /**
  Test a track missing an object
  */
  {
    std::string ObjA = "60001 -60002 60003 -60004 60005 -60006 72 73";
    std::string ObjB = "(-72 : -73)";

    createSurfaces(ObjA);
    Object object1 = Object();
    object1.setObject(3, ObjA);
    object1.populate(SMap);

    createSurfaces(ObjB);
    Object object2 = Object();
    object2.setObject(4, ObjB);
    object2.populate(SMap);

    Track TL(Kernel::V3D(-5, 0, 0), Kernel::V3D(0, 1, 0));

    // CARE: This CANNOT be called twice
    TS_ASSERT_EQUALS(object1.interceptSurface(TL), 0);
    TS_ASSERT_EQUALS(object2.interceptSurface(TL), 0);

    std::vector<Link> expectedResults; // left empty as this should miss
    checkTrackIntercept(TL, expectedResults);
  }

  void testComplementWithTwoPrimitives() {
    auto shell = ComponentCreationHelper::createHollowShell(0.5, 1.0);

    TS_ASSERT_EQUALS(2, shell->getSurfaceIndex().size());

    // Are the rules correct?
    const Rule *headRule = shell->topRule();
    TS_ASSERT_EQUALS("Intersection", headRule->className());
    const Rule *leaf1 = headRule->leaf(0);
    TS_ASSERT_EQUALS("SurfPoint", leaf1->className());
    auto surfPt1 = dynamic_cast<const SurfPoint *>(leaf1);
    TS_ASSERT(surfPt1);
    TS_ASSERT_EQUALS(2, surfPt1->getKeyN());
    auto outer = dynamic_cast<const Sphere *>(surfPt1->getKey());
    TS_ASSERT(outer);
    TS_ASSERT_DELTA(1.0, outer->getRadius(), 1e-10);

    const Rule *leaf2 = headRule->leaf(1);
    TS_ASSERT_EQUALS("CompGrp", leaf2->className());
    auto compRule = dynamic_cast<const CompGrp *>(leaf2);
    TS_ASSERT(compRule);
    TS_ASSERT_EQUALS("SurfPoint", compRule->leaf(0)->className());
    auto surfPt2 = dynamic_cast<const SurfPoint *>(compRule->leaf(0));
    TS_ASSERT_EQUALS(1, surfPt2->getKeyN());
    auto inner = dynamic_cast<const Sphere *>(surfPt2->getKey());
    TS_ASSERT(inner);
    TS_ASSERT_DELTA(0.5, inner->getRadius(), 1e-10);

    TS_ASSERT_EQUALS(false, shell->isValid(V3D(0, 0, 0)));

    Track p1(V3D(-2, 0, 0), V3D(1, 0, 0));
    int nsegments = shell->interceptSurface(p1);
    TS_ASSERT_EQUALS(2, nsegments);
    // total traversed distance -> 2*(r2-r1)
    double distanceInside(0.0);
    std::for_each(p1.cbegin(), p1.cend(),
                  [&distanceInside](const Link &segment) {
                    distanceInside += segment.distInsideObject;
                  });
    TS_ASSERT_DELTA(1.0, distanceInside, 1e-10);
  }

  void testFindPointInCube()
  /**
  Test find point in cube
  */
  {
    Object_sptr geom_obj = createUnitCube();
    // initial guess in object
    Kernel::V3D pt;
    TS_ASSERT_EQUALS(geom_obj->getPointInObject(pt), 1);
    TS_ASSERT_EQUALS(pt, V3D(0, 0, 0));
    // initial guess not in object, but on x-axis
    std::vector<std::string> planes{"px 10",  "px 11",   "py -0.5",
                                    "py 0.5", "pz -0.5", "pz 0.5"};
    Object_sptr B = createCuboid(planes);
    TS_ASSERT_EQUALS(B->getPointInObject(pt), 1);
    TS_ASSERT_EQUALS(pt, V3D(10, 0, 0));
    // on y axis
    planes = {"px -0.5", "px 0.5", "py -22", "py -21", "pz -0.5", "pz 0.5"};
    Object_sptr C = createCuboid(planes);
    TS_ASSERT_EQUALS(C->getPointInObject(pt), 1);
    TS_ASSERT_EQUALS(pt, V3D(0, -21, 0));
    // not on principle axis, now works using getBoundingBox
    planes = {"px 0.5", "px 1.5", "py -22", "py -21", "pz -0.5", "pz 0.5"};
    Object_sptr D = createCuboid(planes);
    TS_ASSERT_EQUALS(D->getPointInObject(pt), 1);
    TS_ASSERT_DELTA(pt.X(), 1.0, 1e-6);
    TS_ASSERT_DELTA(pt.Y(), -21.5, 1e-6);
    TS_ASSERT_DELTA(pt.Z(), 0.0, 1e-6);
    // Test non axis aligned (AA) case - getPointInObject works because the
    // object is on a principle axis
    // However, if not on a principle axis then the getBoundingBox fails to find
    // correct minima (maxima are OK)
    // This is related to use of the complement for -ve surfaces and might be
    // avoided by only using +ve surfaces
    // for defining non-AA objects. However, BoundingBox is poor for non-AA and
    // needs improvement if these are
    // common
    planes = {"p 1 0 0 -0.5", "p 1 0 0 0.5",
              "p 0 .70710678118 .70710678118 -1.1",
              "p 0 .70710678118 .70710678118 -0.1",
              "p 0 -.70710678118 .70710678118 -0.5",
              "p 0 -.70710678118 .70710678118 0.5"};
    Object_sptr E = createCuboid(planes);
    TS_ASSERT_EQUALS(E->getPointInObject(pt), 1);
    TS_ASSERT_DELTA(pt.X(), 0.0, 1e-6);
    TS_ASSERT_DELTA(pt.Y(), -0.1414213562373, 1e-6);
    TS_ASSERT_DELTA(pt.Z(), 0.0, 1e-6);
    // This test used to fail to find a point in object, as object not on a
    // principle axis and getBoundingBox did not give a useful result in this
    // case. Framework has now been updated to support this automatically.
    // Object is unit cube located at +-0.5 in x but centred on z=y=-1.606.. and
    // rotated 45deg to these two axes
    planes = {"p 1 0 0 -0.5", "p 1 0 0 0.5",
              "p 0  .70710678118 .70710678118 -2",
              "p 0  .70710678118 .70710678118 -1",
              "p 0 -.70710678118 .70710678118 -0.5",
              "p 0 -.70710678118 .70710678118 0.5"};
    Object_sptr F = createCuboid(planes);
    TS_ASSERT_EQUALS(F->getPointInObject(pt), 1); // This now succeeds
    // Test use of defineBoundingBox to explictly set the bounding box, when the
    // automatic method fails
    F->defineBoundingBox(0.5, -0.5 * M_SQRT1_2, -0.5 * M_SQRT1_2, -0.5,
                         -M_SQRT2 - 0.5 * M_SQRT1_2,
                         -M_SQRT2 - 0.5 * M_SQRT1_2);
    TS_ASSERT_EQUALS(F->getPointInObject(pt), 1);
    Object_sptr S = ComponentCreationHelper::createSphere(4.1);
    TS_ASSERT_EQUALS(S->getPointInObject(pt), 1);
    TS_ASSERT_EQUALS(pt, V3D(0.0, 0.0, 0));
  }

  void testGeneratePointInside() {
    using namespace ::testing;

    // Generate "random" sequence
    MockRNG rng;
    Sequence rand;
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(0.55));
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(0.65));
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(0.70));

    // inner radius=0.5, outer=1. Random sequence set up so as to give point
    // inside hole
    auto shell = ComponentCreationHelper::createHollowShell(0.5, 1.0);
    size_t maxAttempts(1);
    V3D point;
    TS_ASSERT_THROWS_NOTHING(
        point = shell->generatePointInObject(rng, maxAttempts));

    const double tolerance(1e-10);
    TS_ASSERT_DELTA(-1. + 2. * 0.55, point.X(), tolerance);
    TS_ASSERT_DELTA(-1. + 2. * 0.65, point.Y(), tolerance);
    TS_ASSERT_DELTA(-1. + 2. * 0.70, point.Z(), tolerance);
  }

  void testGeneratePointInsideRespectsMaxAttempts() {
    using namespace ::testing;

    // Generate "random" sequence
    MockRNG rng;
    Sequence rand;
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(0.1));
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(0.2));
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(0.3));

    // inner radius=0.5, outer=1. Random sequence set up so as to give point
    // inside hole
    auto shell = ComponentCreationHelper::createHollowShell(0.5, 1.0);
    size_t maxAttempts(1);
    TS_ASSERT_THROWS(shell->generatePointInObject(rng, maxAttempts),
                     std::runtime_error);
  }

  void testGeneratePointInsideRespectsActiveRegion() {
    using namespace ::testing;

    // Generate "random" sequence.
    MockRNG rng;
    Sequence rand;
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(0.01));
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(0.02));
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(0.03));

    // Radius=0.5
    auto ball = ComponentCreationHelper::createSphere(0.5);
    // Create a thin infinite rectangular region to restrict point generation
    BoundingBox activeRegion(0.1, 0.1, 0.1, -0.1, -0.1, -0.1);
    size_t maxAttempts(1);
    V3D point;
    TS_ASSERT_THROWS_NOTHING(
        point = ball->generatePointInObject(rng, activeRegion, maxAttempts));
    const double tolerance(1e-10);
    TS_ASSERT_DELTA(-0.1 + 0.01 * 0.2, point.X(), tolerance);
    TS_ASSERT_DELTA(-0.1 + 0.02 * 0.2, point.Y(), tolerance);
    TS_ASSERT_DELTA(-0.1 + 0.03 * 0.2, point.Z(), tolerance);
  }

  void testSolidAngleSphere()
  /**
  Test solid angle calculation for a sphere
  */
  {
    Object_sptr geom_obj = ComponentCreationHelper::createSphere(4.1);
    double satol = 2e-2; // tolerance for solid angle

    // Solid angle at distance 8.1 from centre of sphere radius 4.1 x/y/z
    // Expected solid angle calculated values from sa=2pi(1-cos(arcsin(R/r))
    // where R is sphere radius and r is distance of observer from sphere centre
    // Intercept for track in reverse direction now worked round
    TS_ASSERT_DELTA(geom_obj->rayTraceSolidAngle(V3D(8.1, 0, 0)), 0.864364,
                    satol);
    // internal point (should be 4pi)
    TS_ASSERT_DELTA(geom_obj->rayTraceSolidAngle(V3D(0, 0, 0)), 4 * M_PI,
                    satol);
    // surface point
    TS_ASSERT_DELTA(geom_obj->rayTraceSolidAngle(V3D(4.1, 0, 0)), 2 * M_PI,
                    satol);
  }

  void testSolidAngleCappedCylinder()
  /**
  Test solid angle calculation for a capped cylinder
  */
  {
    Object_sptr geom_obj = createSmallCappedCylinder();
    // Want to test triangulation so setup a geometry handler
    boost::shared_ptr<GluGeometryHandler> h =
        boost::shared_ptr<GluGeometryHandler>(
            new GluGeometryHandler(geom_obj.get()));
    h->setCylinder(V3D(-0.0015, 0.0, 0.0), V3D(1., 0.0, 0.0), 0.005, 0.003);
    geom_obj->setGeometryHandler(h);

    double satol(1e-8); // tolerance for solid angle

    // solid angle at point -0.5 from capped cyl -1.0 -0.997 in x, rad 0.005 -
    // approx WISH cylinder
    // We intentionally exclude the cylinder end caps so they this should
    // produce 0
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(-0.5, 0.0, 0.0)), 0.0,
                    satol);
    // Other end
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(-1.497, 0.0, 0.0)), 0.0,
                    satol);

    // Side values
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(0, 0, 0.1)), 0.00301186,
                    satol);
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(0, 0, -0.1)), 0.00301186,
                    satol);
    // Sweep in the axis of the cylinder angle to see if the solid angle
    // decreases (as we are excluding the end caps)
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(0.1, 0.0, 0.1)),
                    0.00100267, satol);

    // internal point (should be 4pi)
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(-0.999, 0.0, 0.0)),
                    4 * M_PI, satol);

    // surface points
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(-1.0, 0.0, 0.0)), 2 * M_PI,
                    satol);
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(-0.997, 0.0, 0.0)),
                    2 * M_PI, satol);
  }

  void testSolidAngleCubeTriangles()
  /**
  Test solid angle calculation for a cube using triangles
  - test for using Open Cascade surface triangulation for all solid angles.
  */
  {
    Object_sptr geom_obj = createUnitCube();
    double satol = 1e-3; // tolerance for solid angle

    // solid angle at distance 0.5 should be 4pi/6 by symmetry
    //
    // tests for Triangulated cube
    //
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(1.0, 0, 0)),
                    M_PI * 2.0 / 3.0, satol);
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(-1.0, 0, 0)),
                    M_PI * 2.0 / 3.0, satol);
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(0, 1.0, 0)),
                    M_PI * 2.0 / 3.0, satol);
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(0, -1.0, 0)),
                    M_PI * 2.0 / 3.0, satol);
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(0, 0, 1.0)),
                    M_PI * 2.0 / 3.0, satol);
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(0, 0, -1.0)),
                    M_PI * 2.0 / 3.0, satol);
  }

  /** Add a scale factor */
  void testSolidAngleCubeTriangles_WithScaleFactor() {
    Object_sptr geom_obj = createUnitCube();
    double satol = 1e-3; // tolerance for solid angle
    // solid angle at distance 0.5 should be 4pi/6 by symmetry
    double expected = M_PI * 2.0 / 3.0;
    V3D scaleFactor(2.0, 2.0, 2.0);
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(2.0, 0, 0), scaleFactor),
                    expected, satol);
  }

  void testExactVolumeCuboid() {
    using namespace Poco::XML;
    const double width = 1.23;
    const double height = 4.98;
    const double thickness = 8.14;
    AutoPtr<Document> shapeDescription = new Document;
    AutoPtr<Element> typeElement = shapeDescription->createElement("type");
    typeElement->setAttribute("name", "testCuboid");
    AutoPtr<Element> shapeElement = createCuboidTypeElement(
        "cuboid-shape", width, height, thickness, shapeDescription);
    typeElement->appendChild(shapeElement);
    AutoPtr<Element> algebraElement =
        shapeDescription->createElement("algebra");
    algebraElement->setAttribute("val", "cuboid-shape");
    typeElement->appendChild(algebraElement);
    ShapeFactory shapeFactory;
    auto cuboid = shapeFactory.createShape(typeElement);
    const double cuboidVolume = width * height * thickness;
    TS_ASSERT_DELTA(cuboid->volume(), cuboidVolume, 1e-6)
  }

  void testExactVolumeSphere() {
    using namespace Poco::XML;
    const double radius = 99.9;
    AutoPtr<Document> shapeDescription = new Document;
    AutoPtr<Element> typeElement = shapeDescription->createElement("type");
    typeElement->setAttribute("name", "testSphere");
    AutoPtr<Element> shapeElement =
        createSphereTypeElement("sphere-shape", radius, shapeDescription);
    typeElement->appendChild(shapeElement);
    AutoPtr<Element> algebraElement =
        shapeDescription->createElement("algebra");
    algebraElement->setAttribute("val", "sphere-shape");
    typeElement->appendChild(algebraElement);
    ShapeFactory shapeFactory;
    auto cuboid = shapeFactory.createShape(typeElement);
    const double sphereVolume = 4.0 / 3.0 * M_PI * radius * radius * radius;
    TS_ASSERT_DELTA(cuboid->volume(), sphereVolume, 1e-6)
  }

  void testExactVolumeCylinder() {
    using namespace Poco::XML;
    const double radius = 0.99;
    const double height = 88;
    AutoPtr<Document> shapeDescription = new Document;
    AutoPtr<Element> typeElement = shapeDescription->createElement("type");
    typeElement->setAttribute("name", "testCylinder");
    AutoPtr<Element> shapeElement = createCylinderTypeElement(
        "cylinder-shape", height, radius, shapeDescription);
    typeElement->appendChild(shapeElement);
    AutoPtr<Element> algebraElement =
        shapeDescription->createElement("algebra");
    algebraElement->setAttribute("val", "cylinder-shape");
    typeElement->appendChild(algebraElement);
    ShapeFactory shapeFactory;
    auto cuboid = shapeFactory.createShape(typeElement);
    const double cylinderVolume = height * M_PI * radius * radius;
    TS_ASSERT_DELTA(cuboid->volume(), cylinderVolume, 1e-6)
  }

  void testMonteCarloVolume() {
    // We use a cuboid with spherical void here.
    using namespace Poco::XML;
    const double width = 71.99;
    const double height = 11.87;
    const double thickness = 74.1;
    AutoPtr<Document> shapeDescription = new Document;
    AutoPtr<Element> typeElement = shapeDescription->createElement("type");
    typeElement->setAttribute("name", "testShape");
    AutoPtr<Element> shapeElement = createCuboidTypeElement(
        "solid-cuboid", width, height, thickness, shapeDescription);
    typeElement->appendChild(shapeElement);
    const double radius = 0.47 * std::min(std::min(width, height), thickness);
    shapeElement =
        createSphereTypeElement("void-sphere", radius, shapeDescription);
    typeElement->appendChild(shapeElement);
    AutoPtr<Element> algebraElement =
        shapeDescription->createElement("algebra");
    algebraElement->setAttribute("val", "solid-cuboid (# void-sphere)");
    typeElement->appendChild(algebraElement);
    ShapeFactory shapeFactory;
    auto cuboid = shapeFactory.createShape(typeElement);
    const double cuboidVolume = width * height * thickness;
    const double sphereVolume = 4.0 / 3.0 * M_PI * radius * radius * radius;
    const double correctVolume = cuboidVolume - sphereVolume;
    TS_ASSERT_DELTA(cuboid->volume(), correctVolume, 1e-3 * correctVolume)
  }

  void testVolumeThrowsWhenBoundingBoxIsInvalid() {
    Object shape("This text gives an invalid Object.");
    TS_ASSERT_THROWS(shape.volume(), std::runtime_error);
  }

  void testGetBoundingBoxForCylinder()
  /**
  Test bounding box for a object capped cylinder
  */
  {
    Object_sptr geom_obj = createCappedCylinder();
    double xmax, ymax, zmax, xmin, ymin, zmin;
    xmax = ymax = zmax = 100;
    xmin = ymin = zmin = -100;
    geom_obj->getBoundingBox(xmax, ymax, zmax, xmin, ymin, zmin);
    TS_ASSERT_DELTA(xmax, 1.2, 0.0001);
    TS_ASSERT_DELTA(ymax, 3.0, 0.0001);
    TS_ASSERT_DELTA(zmax, 3.0, 0.0001);
    TS_ASSERT_DELTA(xmin, -3.2, 0.0001);
    TS_ASSERT_DELTA(ymin, -3.0, 0.0001);
    TS_ASSERT_DELTA(zmin, -3.0, 0.0001);
  }

  void testGetBoundingBoxForCuboid() {
    Object_sptr cuboid = createUnitCube();
    double xmax, ymax, zmax, xmin, ymin, zmin;
    xmax = ymax = zmax = 100;
    xmin = ymin = zmin = -100;

    cuboid->getBoundingBox(xmax, ymax, zmax, xmin, ymin, zmin);

    TS_ASSERT_DELTA(xmax, 0.5, 0.0001);
    TS_ASSERT_DELTA(ymax, 0.5, 0.0001);
    TS_ASSERT_DELTA(zmax, 0.5, 0.0001);
    TS_ASSERT_DELTA(xmin, -0.5, 0.0001);
    TS_ASSERT_DELTA(ymin, -0.5, 0.0001);
    TS_ASSERT_DELTA(zmin, -0.5, 0.0001);
  }

  void testGetBoundingBoxForHexahedron() {
    // For information on how the hexahedron is constructed
    // See
    // http://docs.mantidproject.org/nightly/concepts/HowToDefineGeometricShape.html#hexahedron
    Hexahedron hex;
    hex.lbb = V3D(0, 0, -2);
    hex.lfb = V3D(1, 0, 0);
    hex.rfb = V3D(1, 1, 0);
    hex.rbb = V3D(0, 1, 0);
    hex.lbt = V3D(0, 0, 2);
    hex.lft = V3D(0.5, 0, 2);
    hex.rft = V3D(0.5, 0.5, 2);
    hex.rbt = V3D(0, 0.5, 2);

    Object_sptr hexahedron = createHexahedron(hex);

    auto bb = hexahedron->getBoundingBox();

    TS_ASSERT_DELTA(bb.xMax(), 1, 0.0001);
    TS_ASSERT_DELTA(bb.yMax(), 1, 0.0001);
    TS_ASSERT_DELTA(bb.zMax(), 2, 0.0001);
    TS_ASSERT_DELTA(bb.xMin(), 0, 0.0001);
    TS_ASSERT_DELTA(bb.yMin(), 0, 0.0001);
    TS_ASSERT_DELTA(bb.zMin(), -2, 0.0001);
  }

  void testdefineBoundingBox()
  /**
  Test use of defineBoundingBox
  */
  {
    Object_sptr geom_obj = createCappedCylinder();
    double xmax, ymax, zmax, xmin, ymin, zmin;
    xmax = 1.2;
    ymax = 3.0;
    zmax = 3.0;
    xmin = -3.2;
    ymin = -3.0;
    zmin = -3.0;

    TS_ASSERT_THROWS_NOTHING(
        geom_obj->defineBoundingBox(xmax, ymax, zmax, xmin, ymin, zmin));

    const BoundingBox &boundBox = geom_obj->getBoundingBox();

    TS_ASSERT_EQUALS(boundBox.xMax(), 1.2);
    TS_ASSERT_EQUALS(boundBox.yMax(), 3.0);
    TS_ASSERT_EQUALS(boundBox.zMax(), 3.0);
    TS_ASSERT_EQUALS(boundBox.xMin(), -3.2);
    TS_ASSERT_EQUALS(boundBox.yMin(), -3.0);
    TS_ASSERT_EQUALS(boundBox.zMin(), -3.0);

    // Inconsistent bounding box
    xmax = 1.2;
    xmin = 3.0;
    TS_ASSERT_THROWS(
        geom_obj->defineBoundingBox(xmax, ymax, zmax, xmin, ymin, zmin),
        std::invalid_argument);
  }
  void testSurfaceTriangulation()
  /**
  Test triangle solid angle calc
  */
  {
    Object_sptr geom_obj = createCappedCylinder();
    double xmax, ymax, zmax, xmin, ymin, zmin;
    xmax = 20;
    ymax = 20.0;
    zmax = 20.0;
    xmin = -20.0;
    ymin = -20.0;
    zmin = -20.0;
    geom_obj->getBoundingBox(xmax, ymax, zmax, xmin, ymin, zmin);
    double saTri, saRay;
    V3D observer(4.2, 0, 0);

    double satol = 1e-3; // typical result tolerance

    //    if(timeTest)
    //    {
    //      // block to test time of solid angle methods
    //      // change false to true to include
    //      int iter=4000;
    //      int starttime=clock();
    //      for (int i=0;i<iter;i++)
    //        saTri=geom_obj->triangleSolidAngle(observer);
    //      int endtime=clock();
    //      std::cout << std::endl << "Cyl tri time=" <<
    //      (endtime-starttime)/(static_cast<double>(CLOCKS_PER_SEC*iter)) <<
    //      '\n';
    //      iter=50;
    //      starttime=clock();
    //      for (int i=0;i<iter;i++)
    //        saRay=geom_obj->rayTraceSolidAngle(observer);
    //      endtime=clock();
    //      std::cout << "Cyl ray time=" <<
    //      (endtime-starttime)/(static_cast<double>(CLOCKS_PER_SEC*iter)) <<
    //      '\n';
    //    }

    saTri = geom_obj->triangleSolidAngle(observer);
    saRay = geom_obj->rayTraceSolidAngle(observer);
    TS_ASSERT_DELTA(saTri, 1.840302, 0.001);
    TS_ASSERT_DELTA(saRay, 1.840302, 0.01);

    observer = V3D(-7.2, 0, 0);
    saTri = geom_obj->triangleSolidAngle(observer);
    saRay = geom_obj->rayTraceSolidAngle(observer);

    TS_ASSERT_DELTA(saTri, 1.25663708, 0.001);
    TS_ASSERT_DELTA(saRay, 1.25663708, 0.001);

    // No analytic value for side on SA, using hi-res value
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(0, 0, 7)), 0.7531,
                    0.753 * satol);
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(0, 7, 0)), 0.7531,
                    0.753 * satol);

    saTri = geom_obj->triangleSolidAngle(V3D(20, 0, 0));
    TS_ASSERT_DELTA(saTri, 0.07850147, satol * 0.0785);
    saTri = geom_obj->triangleSolidAngle(V3D(200, 0, 0));
    TS_ASSERT_DELTA(saTri, 0.000715295, satol * 0.000715);
    saTri = geom_obj->triangleSolidAngle(V3D(2000, 0, 0));
    TS_ASSERT_DELTA(saTri, 7.08131e-6, satol * 7.08e-6);
  }
  void testSolidAngleSphereTri()
  /**
  Test solid angle calculation for a sphere from triangulation
  */
  {
    Object_sptr geom_obj = ComponentCreationHelper::createSphere(4.1);
    double satol = 1e-3; // tolerance for solid angle

    // Solid angle at distance 8.1 from centre of sphere radius 4.1 x/y/z
    // Expected solid angle calculated values from sa=2pi(1-cos(arcsin(R/r))
    // where R is sphere radius and r is distance of observer from sphere centre
    // Intercept for track in reverse direction now worked round
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(8.1, 0, 0)), 0.864364,
                    satol);
    // internal point (should be 4pi)
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(0, 0, 0)), 4 * M_PI,
                    satol);
    // surface point
    TS_ASSERT_DELTA(geom_obj->triangleSolidAngle(V3D(4.1, 0, 0)), 2 * M_PI,
                    satol);
  }

private:
  /// Surface type
  typedef std::map<int, boost::shared_ptr<Surface>> STYPE;

  /// set timeTest true to get time comparisons of soild angle methods
  const static bool timeTest = false;

  STYPE SMap; ///< Surface Map

  Object_sptr createCappedCylinder() {
    std::string C31 = "cx 3.0"; // cylinder x-axis radius 3
    std::string C32 = "px 1.2";
    std::string C33 = "px -3.2";

    // First create some surfaces
    std::map<int, boost::shared_ptr<Surface>> CylSurMap;
    CylSurMap[31] = boost::make_shared<Cylinder>();
    CylSurMap[32] = boost::make_shared<Plane>();
    CylSurMap[33] = boost::make_shared<Plane>();

    CylSurMap[31]->setSurface(C31);
    CylSurMap[32]->setSurface(C32);
    CylSurMap[33]->setSurface(C33);
    CylSurMap[31]->setName(31);
    CylSurMap[32]->setName(32);
    CylSurMap[33]->setName(33);

    // Capped cylinder (id 21)
    // using surface ids: 31 (cylinder) 32 (plane (top) ) and 33 (plane (base))
    std::string ObjCapCylinder = "-31 -32 33";

    Object_sptr retVal = Object_sptr(new Object);
    retVal->setObject(21, ObjCapCylinder);
    retVal->populate(CylSurMap);

    TS_ASSERT(retVal.get());

    return retVal;
  }

  // This creates a cylinder to test the solid angle that is more realistic in
  // size
  // for a detector cylinder
  Object_sptr createSmallCappedCylinder() {
    std::string C31 =
        "cx 0.005"; // cylinder x-axis radius 0.005 and height 0.003
    std::string C32 = "px -0.997";
    std::string C33 = "px -1.0";

    // First create some surfaces
    std::map<int, boost::shared_ptr<Surface>> CylSurMap;
    CylSurMap[31] = boost::make_shared<Cylinder>();
    CylSurMap[32] = boost::make_shared<Plane>();
    CylSurMap[33] = boost::make_shared<Plane>();

    CylSurMap[31]->setSurface(C31);
    CylSurMap[32]->setSurface(C32);
    CylSurMap[33]->setSurface(C33);
    CylSurMap[31]->setName(31);
    CylSurMap[32]->setName(32);
    CylSurMap[33]->setName(33);

    // Capped cylinder (id 21)
    // using surface ids: 31 (cylinder) 32 (plane (top) ) and 33 (plane (base))
    std::string ObjCapCylinder = "-31 -32 33";

    Object_sptr retVal = Object_sptr(new Object);
    retVal->setObject(21, ObjCapCylinder);
    retVal->populate(CylSurMap);

    return retVal;
  }

  void clearSurfMap()
  /**
  Clears the surface map for a new test
  or destruction.
  */
  {
    SMap.clear();
    return;
  }

  void createSurfaces(const std::string &desired)
  /**
  Creates a list of surfaces for used in the objects
  and populates the MObj layers.
  */
  {
    clearSurfMap();

    // PLANE SURFACES:

    typedef std::pair<int, std::string> SCompT;
    std::vector<SCompT> SurfLine;
    if (desired.find("60001") != std::string::npos)
      SurfLine.push_back(SCompT(60001, "px -1"));
    if (desired.find("60002") != std::string::npos)
      SurfLine.push_back(SCompT(60002, "px 1"));
    if (desired.find("60003") != std::string::npos)
      SurfLine.push_back(SCompT(60003, "py -2"));
    if (desired.find("60004") != std::string::npos)
      SurfLine.push_back(SCompT(60004, "py 2"));
    if (desired.find("60005") != std::string::npos)
      SurfLine.push_back(SCompT(60005, "pz -3"));
    if (desired.find("60006") != std::string::npos)
      SurfLine.push_back(SCompT(60006, "pz 3"));

    if (desired.find("80001") != std::string::npos)
      SurfLine.push_back(SCompT(80001, "px 4.5"));
    if (desired.find("80002") != std::string::npos)
      SurfLine.push_back(SCompT(80002, "px 6.5"));

    if (desired.find("71") != std::string::npos)
      SurfLine.push_back(SCompT(71, "so 0.8"));
    if (desired.find("72") != std::string::npos)
      SurfLine.push_back(SCompT(72, "s -0.7 0 0 0.3"));
    if (desired.find("73") != std::string::npos)
      SurfLine.push_back(SCompT(73, "s 0.6 0 0 0.4"));

    // Note that the testObject now manages the "new Plane"
    for (auto vc = SurfLine.cbegin(); vc != SurfLine.cend(); ++vc) {
      auto A = Geometry::SurfaceFactory::Instance()->processLine(vc->second);
      TSM_ASSERT("Expected a non-null surface from the factory", A);
      A->setName(vc->first);
      SMap.insert(STYPE::value_type(vc->first,
                                    boost::shared_ptr<Surface>(A.release())));
    }

    return;
  }

  Object_sptr createUnitCube() {
    std::string C1 = "px -0.5"; // cube +/-0.5
    std::string C2 = "px 0.5";
    std::string C3 = "py -0.5";
    std::string C4 = "py 0.5";
    std::string C5 = "pz -0.5";
    std::string C6 = "pz 0.5";

    // Create surfaces
    std::map<int, boost::shared_ptr<Surface>> CubeSurMap;
    CubeSurMap[1] = boost::make_shared<Plane>();
    CubeSurMap[2] = boost::make_shared<Plane>();
    CubeSurMap[3] = boost::make_shared<Plane>();
    CubeSurMap[4] = boost::make_shared<Plane>();
    CubeSurMap[5] = boost::make_shared<Plane>();
    CubeSurMap[6] = boost::make_shared<Plane>();

    CubeSurMap[1]->setSurface(C1);
    CubeSurMap[2]->setSurface(C2);
    CubeSurMap[3]->setSurface(C3);
    CubeSurMap[4]->setSurface(C4);
    CubeSurMap[5]->setSurface(C5);
    CubeSurMap[6]->setSurface(C6);
    CubeSurMap[1]->setName(1);
    CubeSurMap[2]->setName(2);
    CubeSurMap[3]->setName(3);
    CubeSurMap[4]->setName(4);
    CubeSurMap[5]->setName(5);
    CubeSurMap[6]->setName(6);

    // Cube (id 68)
    // using surface ids:  1-6
    std::string ObjCube = "1 -2 3 -4 5 -6";

    Object_sptr retVal = Object_sptr(new Object);
    retVal->setObject(68, ObjCube);
    retVal->populate(CubeSurMap);

    return retVal;
  }

  Object_sptr createCuboid(std::vector<std::string> &planes) {
    std::string C1 = planes[0];
    std::string C2 = planes[1];
    std::string C3 = planes[2];
    std::string C4 = planes[3];
    std::string C5 = planes[4];
    std::string C6 = planes[5];

    // Create surfaces
    std::map<int, boost::shared_ptr<Surface>> CubeSurMap;
    CubeSurMap[1] = boost::make_shared<Plane>();
    CubeSurMap[2] = boost::make_shared<Plane>();
    CubeSurMap[3] = boost::make_shared<Plane>();
    CubeSurMap[4] = boost::make_shared<Plane>();
    CubeSurMap[5] = boost::make_shared<Plane>();
    CubeSurMap[6] = boost::make_shared<Plane>();

    CubeSurMap[1]->setSurface(C1);
    CubeSurMap[2]->setSurface(C2);
    CubeSurMap[3]->setSurface(C3);
    CubeSurMap[4]->setSurface(C4);
    CubeSurMap[5]->setSurface(C5);
    CubeSurMap[6]->setSurface(C6);
    CubeSurMap[1]->setName(1);
    CubeSurMap[2]->setName(2);
    CubeSurMap[3]->setName(3);
    CubeSurMap[4]->setName(4);
    CubeSurMap[5]->setName(5);
    CubeSurMap[6]->setName(6);

    // Cube (id 68)
    // using surface ids:  1-6
    std::string ObjCube = "1 -2 3 -4 5 -6";

    Object_sptr retVal = Object_sptr(new Object);
    retVal->setObject(68, ObjCube);
    retVal->populate(CubeSurMap);

    return retVal;
  }

  Object_sptr createHexahedron(Hexahedron &hex) {
    // Create surfaces
    std::map<int, boost::shared_ptr<Surface>> HexSurMap;
    HexSurMap[1] = boost::make_shared<Plane>();
    HexSurMap[2] = boost::make_shared<Plane>();
    HexSurMap[3] = boost::make_shared<Plane>();
    HexSurMap[4] = boost::make_shared<Plane>();
    HexSurMap[5] = boost::make_shared<Plane>();
    HexSurMap[6] = boost::make_shared<Plane>();

    V3D normal;

    // add front face
    auto pPlaneFrontCutoff = boost::make_shared<Plane>();

    // calculate surface normal
    normal = (hex.rfb - hex.lfb).cross_prod(hex.lft - hex.lfb);
    // Ensure surfacenormal is pointing in the correct direction
    if (normal.scalar_prod(hex.rfb - hex.rbb) < 0)
      normal *= -1.0;
    pPlaneFrontCutoff->setPlane(hex.lfb, normal);
    HexSurMap[1] = pPlaneFrontCutoff;

    // add back face
    auto pPlaneBackCutoff = boost::make_shared<Plane>();
    normal = (hex.rbb - hex.lbb).cross_prod(hex.lbt - hex.lbb);
    if (normal.scalar_prod(hex.rfb - hex.rbb) < 0)
      normal *= -1.0;
    pPlaneBackCutoff->setPlane(hex.lbb, normal);
    HexSurMap[2] = pPlaneBackCutoff;

    // add left face
    auto pPlaneLeftCutoff = boost::make_shared<Plane>();
    normal = (hex.lbb - hex.lfb).cross_prod(hex.lft - hex.lfb);
    if (normal.scalar_prod(hex.rfb - hex.lfb) < 0)
      normal *= -1.0;
    pPlaneLeftCutoff->setPlane(hex.lfb, normal);
    HexSurMap[3] = pPlaneLeftCutoff;

    // add right face
    auto pPlaneRightCutoff = boost::make_shared<Plane>();
    normal = (hex.rbb - hex.rfb).cross_prod(hex.rft - hex.rfb);
    if (normal.scalar_prod(hex.rfb - hex.lfb) < 0)
      normal *= -1.0;
    pPlaneRightCutoff->setPlane(hex.rfb, normal);
    HexSurMap[4] = pPlaneRightCutoff;

    // add top face
    auto pPlaneTopCutoff = boost::make_shared<Plane>();
    normal = (hex.rft - hex.lft).cross_prod(hex.lbt - hex.lft);
    if (normal.scalar_prod(hex.rft - hex.rfb) < 0)
      normal *= -1.0;
    pPlaneTopCutoff->setPlane(hex.lft, normal);
    HexSurMap[5] = pPlaneTopCutoff;

    // add bottom face
    auto pPlaneBottomCutoff = boost::make_shared<Plane>();
    normal = (hex.rfb - hex.lfb).cross_prod(hex.lbb - hex.lfb);
    if (normal.scalar_prod(hex.rft - hex.rfb) < 0)
      normal *= -1.0;
    pPlaneBottomCutoff->setPlane(hex.lfb, normal);
    HexSurMap[6] = pPlaneBottomCutoff;

    // using surface ids:  1-6
    HexSurMap[1]->setName(1);
    HexSurMap[2]->setName(2);
    HexSurMap[3]->setName(3);
    HexSurMap[4]->setName(4);
    HexSurMap[5]->setName(5);
    HexSurMap[6]->setName(6);

    std::string ObjHex = "-1 2 3 -4 -5 6";

    Object_sptr retVal = Object_sptr(new Object);

    // Explicitly setting the GluGeometryHanler hexahedron allows
    // for the correct bounding box calculation.
    auto handler = boost::make_shared<GluGeometryHandler>(retVal);
    handler->setHexahedron(hex.lbb, hex.lfb, hex.rfb, hex.rbb, hex.lbt, hex.lft,
                           hex.rft, hex.rbt);
    retVal->setGeometryHandler(handler);

    retVal->setObject(68, ObjHex);
    retVal->populate(HexSurMap);
    return retVal;
  }

  static Poco::XML::AutoPtr<Poco::XML::Element>
  createCuboidTypeElement(const std::string &id, const double width,
                          const double height, const double thickness,
                          Poco::XML::AutoPtr<Poco::XML::Document> &document) {
    using namespace Poco::XML;
    AutoPtr<Element> shapeElement = document->createElement("cuboid");
    shapeElement->setAttribute("id", id);
    AutoPtr<Element> element =
        document->createElement("left-front-bottom-point");
    element->setAttribute("x", std::to_string(-width / 2));
    element->setAttribute("y", std::to_string(-height / 2));
    element->setAttribute("z", std::to_string(thickness / 2));
    shapeElement->appendChild(element);
    element = document->createElement("left-front-top-point");
    element->setAttribute("x", std::to_string(-width / 2));
    element->setAttribute("y", std::to_string(height / 2));
    element->setAttribute("z", std::to_string(thickness / 2));
    shapeElement->appendChild(element);
    element = document->createElement("left-back-bottom-point");
    element->setAttribute("x", std::to_string(-width / 2));
    element->setAttribute("y", std::to_string(-height / 2));
    element->setAttribute("z", std::to_string(-thickness / 2));
    shapeElement->appendChild(element);
    element = document->createElement("right-front-bottom-point");
    element->setAttribute("x", std::to_string(width / 2));
    element->setAttribute("y", std::to_string(-height / 2));
    element->setAttribute("z", std::to_string(thickness / 2));
    shapeElement->appendChild(element);
    return shapeElement;
  }

  static Poco::XML::AutoPtr<Poco::XML::Element>
  createSphereTypeElement(const std::string &id, const double radius,
                          Poco::XML::AutoPtr<Poco::XML::Document> &document) {
    using namespace Poco::XML;
    AutoPtr<Element> shapeElement = document->createElement("sphere");
    shapeElement->setAttribute("id", id);
    AutoPtr<Element> element = document->createElement("centre");
    element->setAttribute("x", "0.0");
    element->setAttribute("y", "0.0");
    element->setAttribute("z", "0.0");
    shapeElement->appendChild(element);
    element = document->createElement("radius");
    element->setAttribute("val", std::to_string(radius));
    shapeElement->appendChild(element);
    return shapeElement;
  }

  static Poco::XML::AutoPtr<Poco::XML::Element>
  createCylinderTypeElement(const std::string &id, const double height,
                            const double radius,
                            Poco::XML::AutoPtr<Poco::XML::Document> &document) {
    using namespace Poco::XML;
    AutoPtr<Element> shapeElement = document->createElement("cylinder");
    shapeElement->setAttribute("id", id);
    AutoPtr<Element> element = document->createElement("centre-of-bottom-base");
    element->setAttribute("x", std::to_string(-height / 2));
    element->setAttribute("y", "0.0");
    element->setAttribute("z", "0.0");
    shapeElement->appendChild(element);
    element = document->createElement("axis");
    element->setAttribute("x", "1.0");
    element->setAttribute("y", "0.0");
    element->setAttribute("z", "0.0");
    shapeElement->appendChild(element);
    element = document->createElement("radius");
    element->setAttribute("val", std::to_string(radius));
    shapeElement->appendChild(element);
    element = document->createElement("height");
    element->setAttribute("val", std::to_string(height));
    shapeElement->appendChild(element);
    return shapeElement;
  }
};

// -----------------------------------------------------------------------------
// Performance tests
// -----------------------------------------------------------------------------
class ObjectTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ObjectTestPerformance *createSuite() {
    return new ObjectTestPerformance();
  }
  static void destroySuite(ObjectTestPerformance *suite) { delete suite; }

  ObjectTestPerformance()
      : rng(200000), solid(ComponentCreationHelper::createSphere(0.1)),
        shell(ComponentCreationHelper::createHollowShell(0.009, 0.01)) {}

  void test_generatePointInside_Solid_Primitive() {
    const size_t maxAttempts(500);
    V3D dummy;
    for (size_t i = 0; i < npoints; ++i) {
      dummy = solid->generatePointInObject(rng, maxAttempts);
    }
  }

  void test_Point_Inside_Solid_Composite_With_Hole() {
    const size_t maxAttempts(500);
    V3D dummy;
    for (size_t i = 0; i < npoints; ++i) {
      dummy = shell->generatePointInObject(rng, maxAttempts);
    }
  }

private:
  const size_t npoints = 20000;
  Mantid::Kernel::MersenneTwister rng;
  Object_sptr solid;
  Object_sptr shell;
};

#endif // MANTID_TESTOBJECT__
