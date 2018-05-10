#ifndef MANTID_TESTOBJCOMPNENT__
#define MANTID_TESTOBJCOMPNENT__

#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidGeometry/Surfaces/Cylinder.h"
#include "MantidGeometry/Surfaces/Plane.h"
#include "MantidGeometry/Surfaces/Quadratic.h"
#include "MantidGeometry/Surfaces/Sphere.h"
#include "MantidKernel/Exception.h"
#include <cxxtest/TestSuite.h>

#include "MantidKernel/Timer.h"

#include "boost/make_shared.hpp"

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

class ObjComponentTest : public CxxTest::TestSuite {
public:
  void testNameConstructor() {
    ObjComponent objComp("objComp1");
    TS_ASSERT_EQUALS(objComp.getName(), "objComp1");
    TS_ASSERT(!objComp.getParent());
  }

  void testNameParentConstructor() {
    Component parent("Parent");
    ObjComponent objComp("objComp1", &parent);
    TS_ASSERT_EQUALS(objComp.getName(), "objComp1");
    TS_ASSERT(objComp.getParent());
  }

  void testType() {
    ObjComponent objComp("objComp");
    TS_ASSERT_EQUALS(objComp.type(), "PhysicalComponent");
  }

  void testShape() {
    // Create an empty shape and put it in an ObjComponent
    IObject_const_sptr shape(new CSGObject);
    ObjComponent objComp("obj", shape);
    // Get it back - it's the same one
    TS_ASSERT_EQUALS(objComp.shape(), shape);
    // Put a different shape object in there and check we get back that one
    IObject_const_sptr shape2(new CSGObject);
    objComp.setShape(shape2);
    TS_ASSERT_DIFFERS(objComp.shape(), shape);
    TS_ASSERT_EQUALS(objComp.shape(), shape2);
  }

  void testIsValid() {
    ObjComponent ocyl("ocyl", createCappedCylinder());

    ocyl.setPos(10, 0, 0);
    ocyl.setRot(Quat(90.0, V3D(0, 0, 1)));
    // Check centre point
    TS_ASSERT(ocyl.isValid(V3D(10, 0, 0)));
    // Check a point that wouldn't be inside if the cylinder isn't rotated
    // correctly
    TS_ASSERT(ocyl.isValid(V3D(10, -2.5, 0)));
    // Check that a point is not inside, that would be if no rotation
    TS_ASSERT(!ocyl.isValid(V3D(11, 0, 0)));
    // Now add a parent with a rotation of its own;
    Component parent("parent", V3D(0, 10, 0), Quat(90.0, V3D(0, 1, 0)));
    ocyl.setParent(&parent);

    // Check centre point
    TS_ASSERT(ocyl.isValid(V3D(0, 10, -10)));
    // Check a point that wouldn't be inside if the cylinder isn't rotated
    // correctly
    TS_ASSERT(ocyl.isValid(V3D(0, 11.1, -10.5)));
    TS_ASSERT(ocyl.isValid(V3D(0.5, 7, -10)));
    // Check that a point is not inside, that would be if no rotation
    TS_ASSERT(!ocyl.isValid(V3D(0, 10, -11.1)));
    TS_ASSERT(!ocyl.isValid(V3D(1, 10, -10)));
    // Take out component's rotation - it should make no difference because it's
    // about the cylinder axis
    ocyl.setRot(Quat(1, 0, 0, 0));
    // and repeat tests above
    TS_ASSERT(ocyl.isValid(V3D(0, 10, -10)));
    TS_ASSERT(ocyl.isValid(V3D(0, 10.5, -11.1)));
    TS_ASSERT(ocyl.isValid(V3D(0.5, 10, -7)));
    TS_ASSERT(!ocyl.isValid(V3D(0, 11.1, -10)));
    TS_ASSERT(!ocyl.isValid(V3D(1, 10, -10)));

    // An ObjComponent without an associated geometric object is regarded as a
    // point
    ObjComponent comp("noShape");
    comp.setPos(1, 2, 3);
    // Check the exact point passes
    TS_ASSERT(comp.isValid(V3D(1, 2, 3)));
    // But that slightly off fails
    TS_ASSERT(!comp.isValid(V3D(1.0001, 2, 3)));
  }

  void testIsOnSide() {
    ObjComponent ocyl("ocyl", createCappedCylinder());
    ocyl.setPos(10, 0, 0);
    ocyl.setRot(Quat(90.0, V3D(0, 0, 1)));
    TS_ASSERT(ocyl.isOnSide(V3D(10.5, 0, 0)));
    TS_ASSERT(ocyl.isOnSide(V3D(9.5, 0, 0)));
    TS_ASSERT(ocyl.isOnSide(V3D(10, 1, 0.5)));
    TS_ASSERT(ocyl.isOnSide(V3D(10, -3, -0.5)));
    TS_ASSERT(ocyl.isOnSide(V3D(9.7, 1.2, 0.3)));
    TS_ASSERT(ocyl.isOnSide(V3D(10, -3.2, 0)));
    TS_ASSERT(!ocyl.isOnSide(V3D(0, 0, 0)));
    // Now add a parent with a rotation of its own;
    Component parent("parent", V3D(0, 10, 0), Quat(90.0, V3D(0, 1, 0)));
    ocyl.setParent(&parent);
    TS_ASSERT(ocyl.isOnSide(V3D(0.5, 10, -10)));
    TS_ASSERT(ocyl.isOnSide(V3D(0, 9, -10.5)));
    TS_ASSERT(ocyl.isOnSide(V3D(0, 11.2, -10)));
    TS_ASSERT(ocyl.isOnSide(V3D(0.2, 6.8, -9.6)));
    TS_ASSERT(ocyl.isOnSide(V3D(-0.5, 11.2, -10)));
    TS_ASSERT(ocyl.isOnSide(V3D(0, 6.8, -9.5)));
    TS_ASSERT(!ocyl.isOnSide(V3D(0, 0, 0)));
    // Take out component's rotation - it should make no difference because it's
    // about the cylinder axis
    ocyl.setRot(Quat(1, 0, 0, 0));
    // and repeat tests above
    TS_ASSERT(ocyl.isOnSide(V3D(0.5, 10, -10)));
    TS_ASSERT(ocyl.isOnSide(V3D(0, 10.5, -9)));
    TS_ASSERT(ocyl.isOnSide(V3D(0, 10, -11.2)));
    TS_ASSERT(ocyl.isOnSide(V3D(0.2, 9.6, -6.8)));
    TS_ASSERT(ocyl.isOnSide(V3D(-0.5, 10, -11.2)));
    TS_ASSERT(ocyl.isOnSide(V3D(0, 9.5, -6.8)));
    TS_ASSERT(!ocyl.isOnSide(V3D(0, 0, 0)));

    // An ObjComponent without an associated geometric object is regarded as a
    // point
    ObjComponent comp("noShape");
    comp.setPos(1, 2, 3);
    // Check the exact point passes
    TS_ASSERT(comp.isOnSide(V3D(1, 2, 3)));
    // But that slightly off fails
    TS_ASSERT(!comp.isOnSide(V3D(1.0001, 2, 3)));
  }

  void testInterceptSurface() {
    ObjComponent ocyl("ocyl", createCappedCylinder());
    ocyl.setPos(10, 0, 0);
    ocyl.setRot(Quat(90.0, V3D(0, 0, 1)));
    Track track(V3D(0, 0, 0), V3D(1, 0, 0));

    TS_ASSERT_EQUALS(ocyl.interceptSurface(track), 1);
    Track::LType::const_iterator it = track.cbegin();
    if (it == track.cend())
      return;
    TS_ASSERT_EQUALS(it->distFromStart, 10.5);
    TS_ASSERT_DELTA(it->distInsideObject, 1, 0.0001);
    TS_ASSERT_EQUALS(it->entryPoint, V3D(9.5, 0, 0));
    TS_ASSERT_EQUALS(it->exitPoint, V3D(10.5, 0, 0));
    // Now add a parent with a rotation of its own;
    Component parent("parent", V3D(0, 10, 0), Quat(90.0, V3D(0, 1, 0)));
    ocyl.setParent(&parent);
    // Check original track misses
    TS_ASSERT_EQUALS(ocyl.interceptSurface(track), 0);
    // Create a new test track going from the origin down the line y = -x
    Track track2(V3D(0, 0, 0), V3D(0, 1, -1));
    TS_ASSERT_EQUALS(ocyl.interceptSurface(track2), 1);
    Track::LType::const_iterator it2 = track2.cbegin();
    if (it2 == track2.cend())
      return;
    TS_ASSERT_DELTA(it2->distFromStart, sqrt(2 * 10.5 * 10.5), 0.0001);
    TS_ASSERT_DELTA(it2->distInsideObject, M_SQRT2, 0.0001);
    TS_ASSERT_EQUALS(it2->entryPoint, V3D(0, 9.5, -9.5));
    TS_ASSERT_EQUALS(it2->exitPoint, V3D(0, 10.5, -10.5));

    // Calling on an ObjComponent without an associated geometric object will
    // throw
    ObjComponent comp("noShape");
    TS_ASSERT_THROWS(comp.interceptSurface(track),
                     Exception::NullPointerException);
  }

  void testSolidAngleCappedCylinder() {
    ObjComponent A("ocyl", createCappedCylinder());
    A.setPos(10, 0, 0);
    A.setRot(Quat(90.0, V3D(0, 0, 1)));
    double satol = 2e-2; // tolerance for solid angle

    TS_ASSERT_DELTA(A.solidAngle(V3D(10, 1.7, 0)), 1.840302, satol);
    // Surface point
    TS_ASSERT_DELTA(A.solidAngle(V3D(10, -1, 0.5)), 2 * M_PI, satol);

    // Add a parent with a rotation of its own;
    Component parent("parent", V3D(0, 10, 0), Quat(90.0, V3D(0, 1, 0)));
    A.setParent(&parent);

    // See testSolidAngleCappedCylinder in ObjectTest - these tests are a subset
    // of them
    TS_ASSERT_DELTA(A.solidAngle(V3D(0, 11.7, -10)), 1.840302, satol);
    TS_ASSERT_DELTA(A.solidAngle(V3D(0, 6.13333333, -10)), 1.25663708, satol);
    // internal point (should be 4pi)
    TS_ASSERT_DELTA(A.solidAngle(V3D(0, 10, -10)), 4 * M_PI, satol);
    // surface point
    TS_ASSERT_DELTA(A.solidAngle(V3D(0.5, 10, -10)), 2 * M_PI, satol);

    // Calling on an ObjComponent without an associated geometric object will
    // throw
    ObjComponent B("noShape");
    TS_ASSERT_THROWS(B.solidAngle(V3D(1, 2, 3)),
                     Exception::NullPointerException);
  }

  void testBoundingBoxCappedCylinder() {
    // Check that getBoundingBox transforms input guess to Object coordinates
    // and
    // result back to ObjComponent
    ObjComponent A("ocyl", createCappedCylinder());
    A.setPos(10, 0, 0);
    A.setRot(Quat(90.0, V3D(0, 0, 1)));

    BoundingBox boundingBox;
    A.getBoundingBox(boundingBox);
    TS_ASSERT_DELTA(boundingBox.xMax(), 10.5, 1e-5);
    TS_ASSERT_DELTA(boundingBox.yMax(), 1.2, 1e-5);
    TS_ASSERT_DELTA(boundingBox.zMax(), 0.5, 1e-5);
    TS_ASSERT_DELTA(boundingBox.xMin(), 9.5, 1e-5);
    TS_ASSERT_DELTA(boundingBox.yMin(), -3.2, 1e-5);
    TS_ASSERT_DELTA(boundingBox.zMin(), -0.5, 1e-5);

    // Add a parent with a rotation of its own;
    Component parent("parent", V3D(0, 10, 0), Quat(90.0, V3D(0, 1, 0)));
    A.setParent(&parent);
    // note that input values are ignored in this case as cached results used.
    A.getBoundingBox(boundingBox);
    // consistent with the solid angle results
    TS_ASSERT_DELTA(boundingBox.zMax(), -9.5, 1e-5);
    TS_ASSERT_DELTA(boundingBox.zMin(), -10.5, 1e-5);
  }
  void testNAL_BoundingBoxCappedCylinder() {
    // Check that getBoundingBox transforms input guess to Object coordinates
    // and
    // result back to ObjComponent
    ObjComponent A("ocyl", createCappedCylinder());
    A.setPos(10, 0, 0);
    A.setRot(Quat(90.0, V3D(0, 0, 1)));

    BoundingBox boundingBox;
    std::vector<V3D> ort(3);
    ort[0] = V3D(0, 0, 1); // x is now old Z
    ort[1] = V3D(1, 0, 0); // y is now old X
    ort[2] = V3D(0, 1, 0); // z is now old Y

    boundingBox.setBoxAlignment(V3D(10, 0, 0), ort);
    A.getBoundingBox(boundingBox);

    TS_ASSERT_DELTA(boundingBox.xMax(), 0.5, 1e-5);
    TS_ASSERT_DELTA(boundingBox.xMin(), -0.5, 1e-5);

    TS_ASSERT_DELTA(boundingBox.yMax(), 0.5, 1e-5);
    TS_ASSERT_DELTA(boundingBox.yMin(), -0.5, 1e-5);

    TS_ASSERT_DELTA(boundingBox.zMax(), 1.2, 1e-5);
    TS_ASSERT_DELTA(boundingBox.zMin(), -3.2, 1e-5);
  }

  void testgetPointInObject() {
    // Check that getPointInObject transforms result back to ObjComponent
    ObjComponent A("ocyl", createCappedCylinder());
    A.setPos(10, 0, 0);
    A.setRot(Quat(90.0, V3D(0, 0, 1)));
    V3D point;
    TS_ASSERT_EQUALS(A.getPointInObject(point), 1);
    TS_ASSERT_DELTA(point.X(), 10.0, 1e-6);
    TS_ASSERT_DELTA(point.Y(), 0.0, 1e-6);
    TS_ASSERT_DELTA(point.Z(), 0.0, 1e-6);
    // Add a parent with a rotation/translation of its own;
    Component parent("parent", V3D(0, 10, 0), Quat(90.0, V3D(0, 1, 0)));
    A.setParent(&parent);
    TS_ASSERT_EQUALS(A.getPointInObject(point), 1);
    TS_ASSERT_DELTA(point.X(), 0.0, 1e-6);
    TS_ASSERT_DELTA(point.Y(), 10.0, 1e-6);
    TS_ASSERT_DELTA(point.Z(), -10.0, 1e-6);
    // Cuboid not on principle axes
    std::vector<std::string> planes{"px 0.5", "px 1.5",  "py -22",
                                    "py -21", "pz -0.5", "pz 0.5"};
    ObjComponent D("ocube", createCuboid(planes));
    D.setPos(10, 0, 0);
    D.setRot(Quat(90.0, V3D(0, 0, 1)));
    TS_ASSERT_EQUALS(D.getPointInObject(point), 1);
    TS_ASSERT_DELTA(point.X(), 31.5, 1e-6);
    TS_ASSERT_DELTA(point.Y(), 1.0, 1e-6);
    TS_ASSERT_DELTA(point.Z(), 0.0, 1e-6);
    // Add a parent with a rotation/translation of its own;
    // Component parent("parent",V3D(0,10,0),Quat(90.0,V3D(0,1,0)));
    D.setParent(&parent);
    TS_ASSERT_EQUALS(D.getPointInObject(point), 1);
    TS_ASSERT_DELTA(point.X(), 0, 1e-6);
    TS_ASSERT_DELTA(point.Y(), 11., 1e-6);
    TS_ASSERT_DELTA(point.Z(), -31.5, 1e-6);
  }

  ObjComponent *MakeWithScaleFactor(const ObjComponent *parent,
                                    ParameterMap *map, double X, double Y,
                                    double Z) {
    ObjComponent *ret = new ObjComponent(parent, map);
    map->addV3D(ret, "sca", V3D(X, Y, Z));
    return ret;
  }

  void testIsValidWithScaleFactor() {
    ParameterMap map;
    ObjComponent ocyl_base("ocyl", createCappedCylinder());
    ObjComponent *ocyl = MakeWithScaleFactor(&ocyl_base, &map, 2.0, 1.0, 1.0);
    TS_ASSERT(ocyl->isValid(V3D(2.4, 0.0, 0.0)));
    TS_ASSERT(ocyl->isValid(V3D(-6.4, 0.0, 0.0)));
    TS_ASSERT(!ocyl->isValid(V3D(2.5, 0.0, 0.0)));
    TS_ASSERT(!ocyl->isValid(V3D(-6.5, 0.0, 0.0)));
    TS_ASSERT(ocyl->isValid(V3D(2.3, 0.0, 0.0)));
    TS_ASSERT(ocyl->isValid(V3D(-6.3, 0.0, 0.0)));
    delete ocyl;
  }

  void testIsOnSideWithScaleFactor() {
    ParameterMap map;
    ObjComponent ocyl_base("ocyl", createCappedCylinder());
    ObjComponent *ocyl = MakeWithScaleFactor(&ocyl_base, &map, 2.0, 1.0, 1.0);
    TS_ASSERT(ocyl->isOnSide(V3D(2.4, 0.0, 0.0)));
    TS_ASSERT(ocyl->isOnSide(V3D(-6.4, 0.0, 0.0)));
    TS_ASSERT(!ocyl->isOnSide(V3D(2.5, 0.0, 0.0)));
    TS_ASSERT(!ocyl->isOnSide(V3D(-6.5, 0.0, 0.0)));
    TS_ASSERT(!ocyl->isOnSide(V3D(2.3, 0.0, 0.0)));
    TS_ASSERT(!ocyl->isOnSide(V3D(-6.3, 0.0, 0.0)));
    delete ocyl;
  }

  void testInterceptSurfaceWithScaleFactor() {
    ParameterMap map;
    ObjComponent ocyl_base("ocyl", createCappedCylinder());
    ObjComponent *ocyl = MakeWithScaleFactor(&ocyl_base, &map, 2.0, 1.0, 3.0);

    Track trackScale(V3D(-6.5, 0, 0), V3D(1.0, 0, 0));
    TS_ASSERT_EQUALS(ocyl->interceptSurface(trackScale), 1);
    Track::LType::const_iterator itscale = trackScale.cbegin();
    TS_ASSERT_EQUALS(itscale->distFromStart, 8.9);
    TS_ASSERT_EQUALS(itscale->entryPoint, V3D(-6.4, 0.0, 0.0));
    TS_ASSERT_EQUALS(itscale->exitPoint, V3D(2.4, 0.0, 0.0));

    Track trackScaleY(V3D(0.0, -2, 0), V3D(0, 2.0, 0));
    TS_ASSERT_EQUALS(ocyl->interceptSurface(trackScaleY), 1);
    Track::LType::const_iterator itscaleY = trackScaleY.cbegin();
    TS_ASSERT_EQUALS(itscaleY->distFromStart, 2.5);
    TS_ASSERT_EQUALS(itscaleY->entryPoint, V3D(0.0, -0.5, 0.0));
    TS_ASSERT_EQUALS(itscaleY->exitPoint, V3D(0.0, 0.5, 0.0));

    Track trackScaleW(V3D(0, 0, -5), V3D(0, 0, 5));
    TS_ASSERT_EQUALS(ocyl->interceptSurface(trackScaleW), 1);
    Track::LType::const_iterator itscaleW = trackScaleW.cbegin();
    TS_ASSERT_DELTA(itscaleW->distFromStart, 6.5, 1e-6);
    TS_ASSERT_EQUALS(itscaleW->entryPoint, V3D(0, 0, -1.5));
    TS_ASSERT_EQUALS(itscaleW->exitPoint, V3D(0, 0, +1.5));
    delete ocyl;
  }

  void testBoundingBoxWithScaleFactor() {
    ParameterMap map;
    ObjComponent A_base("ocyl", createCappedCylinder());
    ObjComponent *A = MakeWithScaleFactor(&A_base, &map, 2.0, 1.0, 1.0);
    BoundingBox bbox;
    A->getBoundingBox(bbox);
    TS_ASSERT_DELTA(bbox.xMax(), 2.4, 0.00001);
    TS_ASSERT_DELTA(bbox.xMin(), -6.4, 0.00001);
    delete A;
  }

  void testPointInObjectWithScaleFactor() {
    ParameterMap map;
    ObjComponent A_base("ocyl", createCappedCylinder());
    ObjComponent *A = MakeWithScaleFactor(&A_base, &map, 2.0, 1.0, 1.0);
    V3D scalept;
    TS_ASSERT_EQUALS(A->getPointInObject(scalept), 1);
    TS_ASSERT_DELTA(scalept.X(), 0.0, 1e-6);
    TS_ASSERT_DELTA(scalept.Y(), 0.0, 1e-6);
    TS_ASSERT_DELTA(scalept.Z(), 0.0, 1e-6);
    delete A;
  }

  void testPointInObjectWithScaleFactor2() {
    ParameterMap map;
    ObjComponent A_base("ocyl", createCappedCylinder());
    A_base.setRot(Quat(90.0, V3D(0, 0, 1)));
    ObjComponent *A = MakeWithScaleFactor(&A_base, &map, 2.0, 1.0, 1.0);
    V3D scalept(0, 0, 0);
    TS_ASSERT_EQUALS(A->getPointInObject(scalept), 1);
    TS_ASSERT_DELTA(scalept.X(), 0.0, 1e-6);
    TS_ASSERT_DELTA(scalept.Y(), 0.0, 1e-6);
    TS_ASSERT_DELTA(scalept.Z(), 0.0, 1e-6);
    delete A;
  }

  void testPointInObjectWithScaleFactorAndWithOffset() {
    ParameterMap map;
    ObjComponent A_base("ocyl", createCappedCylinder());
    A_base.setPos(10, 0, 0);
    ObjComponent *A = MakeWithScaleFactor(&A_base, &map, 2.0, 1.0, 1.0);
    V3D scalept(0, 0, 0);
    TS_ASSERT_EQUALS(A->getPointInObject(scalept), 1);
    TS_ASSERT_DELTA(scalept.X(), 10.0, 1e-6);
    TS_ASSERT_DELTA(scalept.Y(), 0.0, 1e-6);
    TS_ASSERT_DELTA(scalept.Z(), 0.0, 1e-6);
    delete A;
  }

  void testSolidAngleCappedCylinderWithScaleFactor() {
    ParameterMap map;
    ObjComponent A_base("ocyl", createCappedCylinder());
    ObjComponent *A = MakeWithScaleFactor(&A_base, &map, 2.0, 1.0, 1.0);

    A_base.setPos(10, 0, 0);
    A_base.setRot(Quat(90.0, V3D(0, 0, 1)));
    double satol = 3e-3; // tolerance for solid angle

    // this point should be 0.5 above the cylinder on its axis of sym
    TS_ASSERT_DELTA(A->solidAngle(V3D(10, 2.9, 0)), 1.840302, satol);
    // Surface point on the edge of cylinder
    TS_ASSERT_DELTA(A->solidAngle(V3D(10, 2.4001, 0)), 2 * M_PI, 1e-2);
    // Internal point
    TS_ASSERT_DELTA(A->solidAngle(V3D(10, 0, 0)), 4 * M_PI, satol);

    // Add a parent with a rotation of its own;
    Component parent("parent", V3D(0, 10, 0), Quat(0.0, V3D(0, 1, 0)));
    A_base.setParent(&parent);

    // See testSolidAngleCappedCylinder in ObjectTest - these tests are a subset
    // of them
    // assume this is the same position as above
    TS_ASSERT_DELTA(A->solidAngle(V3D(10, 12.9, 0)), 1.840302, satol);
    // internal point (should be 4pi)
    TS_ASSERT_DELTA(A->solidAngle(V3D(10, 10, 0)), 4 * M_PI, satol);

    // Calling on an ObjComponent without an associated geometric object will
    // throw
    ObjComponent B("noShape");
    TS_ASSERT_THROWS(B.solidAngle(V3D(1, 2, 3)),
                     Exception::NullPointerException)
    delete A;
  }

private:
  boost::shared_ptr<CSGObject> createCappedCylinder() {
    std::string C31 = "cx 0.5"; // cylinder x-axis radius 0.5
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

    auto retVal = boost::make_shared<CSGObject>();
    retVal->setObject(21, ObjCapCylinder);
    retVal->populate(CylSurMap);

    return retVal;
  }

  boost::shared_ptr<CSGObject> createCappedCylinder2() {
    std::string C31 = "cx 0.5"; // cylinder x-axis radius 0.5
    std::string C32 = "px -1.0";
    std::string C33 = "px -3.0";

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

    boost::shared_ptr<CSGObject> retVal = boost::make_shared<CSGObject>();
    retVal->setObject(21, ObjCapCylinder);
    retVal->populate(CylSurMap);

    return retVal;
  }

  boost::shared_ptr<CSGObject> createCuboid(std::vector<std::string> &planes) {
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

    boost::shared_ptr<CSGObject> retVal = boost::make_shared<CSGObject>();
    retVal->setObject(68, ObjCube);
    retVal->populate(CubeSurMap);

    return retVal;
  }
};

#endif
