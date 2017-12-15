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
    TS_ASSERT_EQUALS(geom_obj->isOnSide(V3D(0.5, -0.5, 0.2)), true);
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
    TS_ASSERT_EQUALS(geom_obj->isValid(V3D(0.5, -0.5, 0.2)), true);
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
    TS_ASSERT_EQUALS(geom_obj->calcValidType(V3D(-0.2, -0.3, -0.5), V3D(-1, -1, -1)), 1);
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
    auto geom_obj = createLShape();
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

  void testInterceptLShapeTwoPass() {
    std::vector<Link> expectedResults;
    IObject_sptr geom_obj = createLShape();
    Track track(V3D(0, 2.5, 0.5), V3D(0.707, -0.707, 0));

    // format = startPoint, endPoint, total distance so far
    expectedResults.push_back(
      Link(V3D(0.5, 2, 0.5), V3D(1, 1.5, 0.5), 1.414, *geom_obj));
    expectedResults.push_back(
      Link(V3D(1.5, 1, 0.5), V3D(2, 0.5, 0.5), 2.828, *geom_obj));
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


  void checkTrackIntercept(IObject_sptr obj, Track &track,
    const std::vector<Link> &expectedResults) {
    int unitCount = obj->interceptSurface(track);
    TS_ASSERT_EQUALS(unitCount, expectedResults.size());
    checkTrackIntercept(track, expectedResults);
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

  

private:

  boost::shared_ptr<MeshObject> createCube(double size) {
    /**
    * Create cube of side length size with vertex at origin,
    * parellel to axes and non-negative vertex coordinates.
    */
    boost::shared_ptr<MeshObject> retVal =
    boost::shared_ptr<MeshObject>(new MeshObject);
    return retVal;
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

};

// -----------------------------------------------------------------------------
// Performance tests
// -----------------------------------------------------------------------------


#endif // MANTID_TESTMESHOBJECT__
