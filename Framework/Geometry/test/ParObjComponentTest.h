#ifndef MANTID_TESTPAROBJCOMPNENT__
#define MANTID_TESTPAROBJCOMPNENT__

#include <cxxtest/TestSuite.h>
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Surfaces/Quadratic.h"
#include "MantidGeometry/Surfaces/Sphere.h"
#include "MantidGeometry/Surfaces/Cylinder.h"
#include "MantidGeometry/Surfaces/Plane.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidKernel/Exception.h"

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

class ParObjComponentTest : public CxxTest::TestSuite {
public:
  void testNameConstructor() {
    ObjComponent objComp("objComp1");

    ParameterMap_const_sptr pmap(new ParameterMap());
    ObjComponent pobjComp(&objComp, pmap.get());

    TS_ASSERT_EQUALS(pobjComp.getName(), "objComp1");
    TS_ASSERT(!pobjComp.getParent());
  }

  void testNameParentConstructor() {
    Component parent("Parent");
    ObjComponent objComp("objComp1", &parent);

    ParameterMap_const_sptr pmap(new ParameterMap());
    ObjComponent pobjComp(&objComp, pmap.get());

    TS_ASSERT_EQUALS(pobjComp.getName(), "objComp1");
    TS_ASSERT(pobjComp.getParent());
  }

  void testType() {
    ObjComponent objComp("objComp");

    ParameterMap_const_sptr pmap(new ParameterMap());
    ObjComponent pobjComp(&objComp, pmap.get());

    TS_ASSERT_EQUALS(objComp.type(), "PhysicalComponent");
  }

  void testIsValid() {
    ObjComponent ocyl("ocyl", createCappedCylinder());

    ParameterMap_const_sptr pmap(new ParameterMap());
    ObjComponent pocyl(&ocyl, pmap.get());

    ocyl.setPos(10, 0, 0);
    ocyl.setRot(Quat(90.0, V3D(0, 0, 1)));
    // Check centre point
    TS_ASSERT(pocyl.isValid(V3D(10, 0, 0)));
    // Check a point that wouldn't be inside if the cylinder isn't rotated
    // correctly
    TS_ASSERT(pocyl.isValid(V3D(10, -2.5, 0)));
    // Check that a point is not inside, that would be if no rotation
    TS_ASSERT(!pocyl.isValid(V3D(11, 0, 0)));
    // Now add a parent with a rotation of its own;
    Component parent("parent", V3D(0, 10, 0), Quat(90.0, V3D(0, 1, 0)));
    ocyl.setParent(&parent);
    // Check centre point
    TS_ASSERT(pocyl.isValid(V3D(0, 10, -10)));
    // Check a point that wouldn't be inside if the cylinder isn't rotated
    // correctly
    TS_ASSERT(pocyl.isValid(V3D(0, 11.1, -10.5)));
    TS_ASSERT(pocyl.isValid(V3D(0.5, 7, -10)));
    // Check that a point is not inside, that would be if no rotation
    TS_ASSERT(!pocyl.isValid(V3D(0, 10, -11.1)));
    TS_ASSERT(!pocyl.isValid(V3D(1, 10, -10)));
    // Take out component's rotation - it should make no difference because it's
    // about the cylinder axis
    ocyl.setRot(Quat(1, 0, 0, 0));
    // and repeat tests above
    TS_ASSERT(pocyl.isValid(V3D(0, 10, -10)));
    TS_ASSERT(pocyl.isValid(V3D(0, 10.5, -11.1)));
    TS_ASSERT(pocyl.isValid(V3D(0.5, 10, -7)));
    TS_ASSERT(!pocyl.isValid(V3D(0, 11.1, -10)));
    TS_ASSERT(!pocyl.isValid(V3D(1, 10, -10)));

    // An ObjComponent without an associated geometric object is regarded as a
    // point
    ObjComponent comp("noShape");
    comp.setPos(1, 2, 3);

    ObjComponent pcomp(&comp, pmap.get());

    // Check the exact point passes
    TS_ASSERT(pcomp.isValid(V3D(1, 2, 3)));
    // But that slightly off fails
    TS_ASSERT(!pcomp.isValid(V3D(1.0001, 2, 3)));
  }

  void testIsOnSide() {
    ObjComponent ocyl("ocyl", createCappedCylinder());
    ocyl.setPos(10, 0, 0);
    ocyl.setRot(Quat(90.0, V3D(0, 0, 1)));

    ParameterMap_const_sptr pmap(new ParameterMap());
    ObjComponent pocyl(&ocyl, pmap.get());

    TS_ASSERT(pocyl.isOnSide(V3D(10.5, 0, 0)));
    TS_ASSERT(pocyl.isOnSide(V3D(9.5, 0, 0)));
    TS_ASSERT(pocyl.isOnSide(V3D(10, 1, 0.5)));
    TS_ASSERT(pocyl.isOnSide(V3D(10, -3, -0.5)));
    TS_ASSERT(pocyl.isOnSide(V3D(9.7, 1.2, 0.3)));
    TS_ASSERT(pocyl.isOnSide(V3D(10, -3.2, 0)));
    TS_ASSERT(!pocyl.isOnSide(V3D(0, 0, 0)));
    // Now add a parent with a rotation of its own;
    Component parent("parent", V3D(0, 10, 0), Quat(90.0, V3D(0, 1, 0)));
    ocyl.setParent(&parent);
    TS_ASSERT(pocyl.isOnSide(V3D(0.5, 10, -10)));
    TS_ASSERT(pocyl.isOnSide(V3D(0, 9, -10.5)));
    TS_ASSERT(pocyl.isOnSide(V3D(0, 11.2, -10)));
    TS_ASSERT(pocyl.isOnSide(V3D(0.2, 6.8, -9.6)));
    TS_ASSERT(pocyl.isOnSide(V3D(-0.5, 11.2, -10)));
    TS_ASSERT(pocyl.isOnSide(V3D(0, 6.8, -9.5)));
    TS_ASSERT(!pocyl.isOnSide(V3D(0, 0, 0)));
    // Take out component's rotation - it should make no difference because it's
    // about the cylinder axis
    ocyl.setRot(Quat(1, 0, 0, 0));
    // and repeat tests above
    TS_ASSERT(pocyl.isOnSide(V3D(0.5, 10, -10)));
    TS_ASSERT(pocyl.isOnSide(V3D(0, 10.5, -9)));
    TS_ASSERT(pocyl.isOnSide(V3D(0, 10, -11.2)));
    TS_ASSERT(pocyl.isOnSide(V3D(0.2, 9.6, -6.8)));
    TS_ASSERT(pocyl.isOnSide(V3D(-0.5, 10, -11.2)));
    TS_ASSERT(pocyl.isOnSide(V3D(0, 9.5, -6.8)));
    TS_ASSERT(!pocyl.isOnSide(V3D(0, 0, 0)));

    // An ObjComponent without an associated geometric object is regarded as a
    // point
    ObjComponent comp("noShape");
    comp.setPos(1, 2, 3);

    ObjComponent pcomp(&comp, pmap.get());

    // Check the exact point passes
    TS_ASSERT(pcomp.isOnSide(V3D(1, 2, 3)));
    // But that slightly off fails
    TS_ASSERT(!pcomp.isOnSide(V3D(1.0001, 2, 3)));
  }

  void testInterceptSurface() {
    ObjComponent ocyl("ocyl", createCappedCylinder());
    ocyl.setPos(10, 0, 0);
    ocyl.setRot(Quat(90.0, V3D(0, 0, 1)));
    Track track(V3D(0, 0, 0), V3D(1, 0, 0));

    ParameterMap_const_sptr pmap(new ParameterMap());
    ObjComponent pocyl(&ocyl, pmap.get());

    TS_ASSERT_EQUALS(pocyl.interceptSurface(track), 1);
    Track::LType::const_iterator it = track.begin();
    if (it == track.end())
      return;
    TS_ASSERT_EQUALS(it->distFromStart, 10.5);
    TS_ASSERT_DELTA(it->distInsideObject, 1, 0.0001);
    TS_ASSERT_EQUALS(it->entryPoint, V3D(9.5, 0, 0));
    TS_ASSERT_EQUALS(it->exitPoint, V3D(10.5, 0, 0));
    // Now add a parent with a rotation of its own;
    Component parent("parent", V3D(0, 10, 0), Quat(90.0, V3D(0, 1, 0)));
    ocyl.setParent(&parent);
    // Check original track misses
    TS_ASSERT_EQUALS(pocyl.interceptSurface(track), 0);
    // Create a new test track going from the origin down the line y = -x
    Track track2(V3D(0, 0, 0), V3D(0, 1, -1));
    TS_ASSERT_EQUALS(pocyl.interceptSurface(track2), 1);
    Track::LType::const_iterator it2 = track2.begin();
    if (it2 == track2.end())
      return;
    TS_ASSERT_DELTA(it2->distFromStart, sqrt(2 * 10.5 * 10.5), 0.0001);
    TS_ASSERT_DELTA(it2->distInsideObject, sqrt(2.0), 0.0001);
    TS_ASSERT_EQUALS(it2->entryPoint, V3D(0, 9.5, -9.5));
    TS_ASSERT_EQUALS(it2->exitPoint, V3D(0, 10.5, -10.5));

    // Calling on an ObjComponent without an associated geometric object will
    // throw
    ObjComponent comp("noShape");

    ObjComponent pcomp(&comp, pmap.get());

    TS_ASSERT_THROWS(pcomp.interceptSurface(track),
                     Exception::NullPointerException);
  }

  void testSolidAngleCappedCylinder() {
    ObjComponent A("ocyl", createCappedCylinder());
    A.setPos(10, 0, 0);
    A.setRot(Quat(90.0, V3D(0, 0, 1)));
    double satol = 2e-2; // tolerance for solid angle

    ParameterMap_const_sptr pmap(new ParameterMap());
    ObjComponent pA(&A, pmap.get());

    TS_ASSERT_DELTA(pA.solidAngle(V3D(10, 1.7, 0)), 1.840302, satol);
    // Surface point
    TS_ASSERT_DELTA(pA.solidAngle(V3D(10, -1, 0.5)), 2 * M_PI, satol);

    // Add a parent with a rotation of its own;
    Component parent("parent", V3D(0, 10, 0), Quat(90.0, V3D(0, 1, 0)));
    A.setParent(&parent);

    // See testSolidAngleCappedCylinder in ObjectTest - these tests are a subset
    // of them
    TS_ASSERT_DELTA(pA.solidAngle(V3D(0, 11.7, -10)), 1.840302, satol);
    TS_ASSERT_DELTA(pA.solidAngle(V3D(0, 6.13333333, -10)), 1.25663708, satol);
    // internal point (should be 4pi)
    TS_ASSERT_DELTA(pA.solidAngle(V3D(0, 10, -10)), 4 * M_PI, satol);
    // surface point
    TS_ASSERT_DELTA(pA.solidAngle(V3D(0.5, 10, -10)), 2 * M_PI, satol);

    // Calling on an ObjComponent without an associated geometric object will
    // throw
    ObjComponent B("noShape");

    ObjComponent pB(&B, pmap.get());

    TS_ASSERT_THROWS(pB.solidAngle(V3D(1, 2, 3)),
                     Exception::NullPointerException);
  }

  void testBoundingBoxCappedCylinder() {
    // Check that getBoundingBox transforms input guess to Object coordinates
    // and
    // result back to ObjComponent
    ObjComponent A("ocyl", createCappedCylinder());
    A.setPos(10, 0, 0);
    A.setRot(Quat(90.0, V3D(0, 0, 1)));
    ParameterMap_sptr pmap(new ParameterMap());
    pmap->addV3D(&A, "pos", V3D(11, 0, 0));
    ObjComponent pA(&A, pmap.get());

    BoundingBox absoluteBox;
    pA.getBoundingBox(absoluteBox);
    TS_ASSERT_DELTA(absoluteBox.xMin(), 10.5, 1e-5);
    TS_ASSERT_DELTA(absoluteBox.xMax(), 11.5, 1e-5);
    // Add a parent with a rotation of its own;
    Component parent("parent", V3D(0, 10, 0), Quat(90.0, V3D(0, 1, 0)));
    A.setParent(&parent);
    // note that input values are ignored in this case as cached results used.
    pA.getBoundingBox(absoluteBox);
    TS_ASSERT_DELTA(absoluteBox.zMax(), -10.5, 1e-08);
    TS_ASSERT_DELTA(absoluteBox.zMin(), -11.5, 1e-08);
  }

  void testgetPointInObject() {
    // Check that getPointInObject transforms result back to ObjComponent
    ObjComponent A("ocyl", createCappedCylinder());
    A.setPos(10, 0, 0);
    A.setRot(Quat(90.0, V3D(0, 0, 1)));

    ParameterMap_const_sptr pmap(new ParameterMap());
    ObjComponent pA(&A, pmap.get());

    V3D point;
    TS_ASSERT_EQUALS(pA.getPointInObject(point), 1);
    TS_ASSERT_DELTA(point.X(), 10.0, 1e-6);
    TS_ASSERT_DELTA(point.Y(), 0.0, 1e-6);
    TS_ASSERT_DELTA(point.Z(), 0.0, 1e-6);
    // Add a parent with a rotation/translation of its own;
    Component parent("parent", V3D(0, 10, 0), Quat(90.0, V3D(0, 1, 0)));
    A.setParent(&parent);
    TS_ASSERT_EQUALS(pA.getPointInObject(point), 1);
    TS_ASSERT_DELTA(point.X(), 0.0, 1e-6);
    TS_ASSERT_DELTA(point.Y(), 10.0, 1e-6);
    TS_ASSERT_DELTA(point.Z(), -10.0, 1e-6);
    // Cuboid not on principle axes
    std::vector<std::string> planes;
    planes.push_back("px 0.5");
    planes.push_back("px 1.5");
    planes.push_back("py -22");
    planes.push_back("py -21");
    planes.push_back("pz -0.5");
    planes.push_back("pz 0.5");
    ObjComponent D("ocube", createCuboid(planes));
    D.setPos(10, 0, 0);
    D.setRot(Quat(90.0, V3D(0, 0, 1)));

    ObjComponent pD(&D, pmap.get());

    TS_ASSERT_EQUALS(pD.getPointInObject(point), 1);
    TS_ASSERT_DELTA(point.X(), 31.5, 1e-6);
    TS_ASSERT_DELTA(point.Y(), 1.0, 1e-6);
    TS_ASSERT_DELTA(point.Z(), 0.0, 1e-6);
    // Add a parent with a rotation/translation of its own;
    // Component parent("parent",V3D(0,10,0),Quat(90.0,V3D(0,1,0)));
    D.setParent(&parent);
    TS_ASSERT_EQUALS(pD.getPointInObject(point), 1);
    TS_ASSERT_DELTA(point.X(), 0, 1e-6);
    TS_ASSERT_DELTA(point.Y(), 11, 1e-6);
    TS_ASSERT_DELTA(point.Z(), -31.5, 1e-6);
  }

private:
  boost::shared_ptr<Object> createCappedCylinder() {
    std::string C31 = "cx 0.5"; // cylinder x-axis radius 0.5
    std::string C32 = "px 1.2";
    std::string C33 = "px -3.2";

    // First create some surfaces
    std::map<int, Surface *> CylSurMap;
    CylSurMap[31] = new Cylinder();
    CylSurMap[32] = new Plane();
    CylSurMap[33] = new Plane();

    CylSurMap[31]->setSurface(C31);
    CylSurMap[32]->setSurface(C32);
    CylSurMap[33]->setSurface(C33);
    CylSurMap[31]->setName(31);
    CylSurMap[32]->setName(32);
    CylSurMap[33]->setName(33);

    // Capped cylinder (id 21)
    // using surface ids: 31 (cylinder) 32 (plane (top) ) and 33 (plane (base))
    std::string ObjCapCylinder = "-31 -32 33";

    boost::shared_ptr<Object> retVal = boost::shared_ptr<Object>(new Object);
    retVal->setObject(21, ObjCapCylinder);
    retVal->populate(CylSurMap);

    return retVal;
  }

  boost::shared_ptr<Object> createCuboid(std::vector<std::string> &planes) {
    std::string C1 = planes[0];
    std::string C2 = planes[1];
    std::string C3 = planes[2];
    std::string C4 = planes[3];
    std::string C5 = planes[4];
    std::string C6 = planes[5];

    // Create surfaces
    std::map<int, Surface *> CubeSurMap;
    CubeSurMap[1] = new Plane();
    CubeSurMap[2] = new Plane();
    CubeSurMap[3] = new Plane();
    CubeSurMap[4] = new Plane();
    CubeSurMap[5] = new Plane();
    CubeSurMap[6] = new Plane();

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

    boost::shared_ptr<Object> retVal = boost::shared_ptr<Object>(new Object);
    retVal->setObject(68, ObjCube);
    retVal->populate(CubeSurMap);

    return retVal;
  }
};

#endif
