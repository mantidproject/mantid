#ifndef MANTID_TESTMESHOBJECT__
#define MANTID_TESTMESHOBJECT__

#include "MantidGeometry/Objects/MeshObject.h"

#include "MantidGeometry/Math/Algebra.h"
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

class MeshObjectTest : public CxxTest::TestSuite {

public:
  void testDefaultObjectHasEmptyMaterial() {
    CSGObject obj;

    TSM_ASSERT_DELTA("Expected a zero number density", 0.0,
                     obj.material().numberDensity(), 1e-12);
  }

  void testObjectSetMaterialReplacesExisting() {
    using Mantid::Kernel::Material;
    CSGObject obj;

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
    int objType(-1);
    double radius(-1.0), height(-1.0);
    std::vector<V3D> pts;
    original.GetObjectGeom(objType, pts, radius, height);
    TS_ASSERT_EQUALS(3, objType);
    TS_ASSERT(boost::dynamic_pointer_cast<GluGeometryHandler>(
      original.getGeometryHandler())); // Change to actual geometry handler
    // when available.

    MeshObject copy(original);
    // The copy should be a primitive object with a GluGeometryHandler
    objType = -1;
    copy.GetObjectGeom(objType, pts, radius, height);

    TS_ASSERT_EQUALS("sp-1", copy.id());
    TS_ASSERT_EQUALS(3, objType);
    TS_ASSERT(boost::dynamic_pointer_cast<GluGeometryHandler>(
      copy.getGeometryHandler()));
    TS_ASSERT_EQUALS(copy.getName(), original.getName());
    // Check the string representation is the same
    //TS_ASSERT_EQUALS(copy.str(), original.str());
    //TS_ASSERT_EQUALS(copy.getSurfaceIndex(), original.getSurfaceIndex());
  }

  void testAssignmentOperatorGivesObjectWithSameAttributes() {
    auto original_ptr = ComponentCreationHelper::createSphere(1.0);
    auto &original = dynamic_cast<MeshObject &>(*original_ptr);
    original.setID("sp-1");
    int objType(-1);
    double radius(-1.0), height(-1.0);
    std::vector<V3D> pts;
    original.GetObjectGeom(objType, pts, radius, height);
    TS_ASSERT_EQUALS(3, objType);
    TS_ASSERT(boost::dynamic_pointer_cast<GluGeometryHandler>(
      original.getGeometryHandler()));  // Change to actual geometry handler
    // when available.

    MeshObject lhs;  // initialize
    lhs = original; // assign
                    // The copy should be a primitive object with a GluGeometryHandler
    objType = -1;
    lhs.GetObjectGeom(objType, pts, radius, height);

    TS_ASSERT_EQUALS("sp-1", lhs.id());
    TS_ASSERT_EQUALS(3, objType);
    TS_ASSERT(boost::dynamic_pointer_cast<GluGeometryHandler>(
      lhs.getGeometryHandler()));  // Change to actual geometry handler
    // when available.
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

  void testIsValidCappedCylinder() {
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

    // not quite on the normal
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(0.0, 0.5, 0.5), V3D(0.5, 0.5, 0)), 1);
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(1.0, 0.5, 0.5), V3D(0.5, 0.5, 0)), -1);
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

private:

  boost::shared_ptr<MeshObject> createCube(double size) {

    boost::shared_ptr<MeshObject> retVal =
    boost::shared_ptr<MeshObject>(new MeshObject);
    return retVal;
  }

};

// -----------------------------------------------------------------------------
// Performance tests
// -----------------------------------------------------------------------------


#endif // MANTID_TESTMESHOBJECT__
