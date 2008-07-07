#ifndef MANTID_TESTOBJCOMPNENT__
#define MANTID_TESTOBJCOMPNENT__

#include <cxxtest/TestSuite.h>
#include "MantidGeometry/ObjComponent.h"
#include "MantidGeometry/Quadratic.h"
#include "MantidGeometry/Sphere.h"
#include "MantidGeometry/Cylinder.h"
#include "MantidGeometry/Plane.h"
#include "MantidGeometry/Object.h"
#include "MantidKernel/Exception.h"

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

class ObjComponentTest : public CxxTest::TestSuite
{
public:
  void testNameConstructor()
  {
    ObjComponent objComp("objComp1");
    TS_ASSERT_EQUALS(objComp.getName(),"objComp1");
    TS_ASSERT(!objComp.getParent());
  }

  void testNameParentConstructor()
  {
    Component parent("Parent");
    ObjComponent objComp("objComp1", &parent);
    TS_ASSERT_EQUALS(objComp.getName(),"objComp1");
    TS_ASSERT(objComp.getParent());
  }

  void testType()
  {
    ObjComponent objComp("objComp");
    TS_ASSERT_EQUALS(objComp.type(),"PhysicalComponent");
  }

  void testIsValid()
  {
    ObjComponent ocyl("ocyl", createCappedCylinder());
    ocyl.setPos(10,0,0);
    ocyl.setRot(Quat(90.0,V3D(0,0,1)));
    // Check centre point
    TS_ASSERT( ocyl.isValid(V3D(10,0,0)) )
    // Check a point that wouldn't be inside if the cylinder isn't rotated correctly
    TS_ASSERT( ocyl.isValid(V3D(10,-2.5,0)) )
    // Check that a point is not inside, that would be if no rotation
    TS_ASSERT( ! ocyl.isValid(V3D(11,0,0)) )
    // Now add a parent with a rotation of its own;
    Component parent("parent",V3D(0,10,0),Quat(90.0,V3D(0,1,0)));
    ocyl.setParent(&parent);
    // Check centre point
    TS_ASSERT( ocyl.isValid(V3D(0,10,-10)) )
    // Check a point that wouldn't be inside if the cylinder isn't rotated correctly
    TS_ASSERT( ocyl.isValid(V3D(0,10.5,-11.1)) )
    TS_ASSERT( ocyl.isValid(V3D(0.5,10,-7)) )
    // Check that a point is not inside, that would be if no rotation
    TS_ASSERT( ! ocyl.isValid(V3D(0,11.1,-10)) )
    TS_ASSERT( ! ocyl.isValid(V3D(1,10,-10)) )
    // Take out component's rotation - it should make no difference because it's about the cylinder axis
    ocyl.setRot(Quat(1,0,0,0));
    // and repeat tests above
    TS_ASSERT( ocyl.isValid(V3D(0,10,-10)) )
    TS_ASSERT( ocyl.isValid(V3D(0,10.5,-11.1)) )
    TS_ASSERT( ocyl.isValid(V3D(0.5,10,-7)) )
    TS_ASSERT( ! ocyl.isValid(V3D(0,11.1,-10)) )
    TS_ASSERT( ! ocyl.isValid(V3D(1,10,-10)) )

    // An ObjComponent without an associated geometric object is regarded as a point
    ObjComponent comp("noShape");
    comp.setPos(1,2,3);
    // Check the exact point passes
    TS_ASSERT( comp.isValid(V3D(1,2,3)) )
    // But that slightly off fails
    TS_ASSERT( ! comp.isValid(V3D(1.0001,2,3)) )
  }

  void testIsOnSide()
  {
    ObjComponent ocyl("ocyl", createCappedCylinder());
    ocyl.setPos(10,0,0);
    ocyl.setRot(Quat(90.0,V3D(0,0,1)));
    TS_ASSERT( ocyl.isOnSide(V3D(10.5,0,0)) )
    TS_ASSERT( ocyl.isOnSide(V3D(9.5,0,0)) )
    TS_ASSERT( ocyl.isOnSide(V3D(10,1,0.5)) )
    TS_ASSERT( ocyl.isOnSide(V3D(10,-3,-0.5)) )
    TS_ASSERT( ocyl.isOnSide(V3D(9.7,1.2,0.3)) )
    TS_ASSERT( ocyl.isOnSide(V3D(10,-3.2,0)) )
    TS_ASSERT( ! ocyl.isOnSide(V3D(0,0,0)) )
    // Now add a parent with a rotation of its own;
    Component parent("parent",V3D(0,10,0),Quat(90.0,V3D(0,1,0)));
    ocyl.setParent(&parent);
    TS_ASSERT( ocyl.isOnSide(V3D(0.5,10,-10)) )
    TS_ASSERT( ocyl.isOnSide(V3D(0,10.5,-9)) )
    TS_ASSERT( ocyl.isOnSide(V3D(0,10,-11.2)) )
    TS_ASSERT( ocyl.isOnSide(V3D(0.2,9.6,-6.8)) )
    TS_ASSERT( ocyl.isOnSide(V3D(-0.5,10,-11.2)) )
    TS_ASSERT( ocyl.isOnSide(V3D(0,9.5,-6.8)) )
    TS_ASSERT( ! ocyl.isOnSide(V3D(0,0,0)) )
    // Take out component's rotation - it should make no difference because it's about the cylinder axis
    ocyl.setRot(Quat(1,0,0,0));
    // and repeat tests above
    TS_ASSERT( ocyl.isOnSide(V3D(0.5,10,-10)) )
    TS_ASSERT( ocyl.isOnSide(V3D(0,10.5,-9)) )
    TS_ASSERT( ocyl.isOnSide(V3D(0,10,-11.2)) )
    TS_ASSERT( ocyl.isOnSide(V3D(0.2,9.6,-6.8)) )
    TS_ASSERT( ocyl.isOnSide(V3D(-0.5,10,-11.2)) )
    TS_ASSERT( ocyl.isOnSide(V3D(0,9.5,-6.8)) )
    TS_ASSERT( ! ocyl.isOnSide(V3D(0,0,0)) )

    // An ObjComponent without an associated geometric object is regarded as a point
    ObjComponent comp("noShape");
    comp.setPos(1,2,3);
    // Check the exact point passes
    TS_ASSERT( comp.isOnSide(V3D(1,2,3)) )
    // But that slightly off fails
    TS_ASSERT( ! comp.isOnSide(V3D(1.0001,2,3)) )
  }

  void testInterceptSurface()
  {
    ObjComponent ocyl("ocyl", createCappedCylinder());
    ocyl.setPos(10,0,0);
    ocyl.setRot(Quat(90.0,V3D(0,0,1)));
    Track track(V3D(0,0,0),V3D(1,0,0));

    TS_ASSERT_EQUALS( ocyl.interceptSurface(track), 1 )
    Track::LType::const_iterator it = track.begin();
    if (it == track.end()) return;
    TS_ASSERT_EQUALS( it->Dist, 10.5 )
    TS_ASSERT_DELTA( it->Length, 1, 0.0001 )
    TS_ASSERT_EQUALS( it->PtA, V3D(9.5,0,0) )
    TS_ASSERT_EQUALS( it->PtB, V3D(10.5,0,0) )
    // Now add a parent with a rotation of its own;
    Component parent("parent",V3D(0,10,0),Quat(90.0,V3D(0,1,0)));
    ocyl.setParent(&parent);
    // Check original track misses
    TS_ASSERT_EQUALS( ocyl.interceptSurface(track), 0 )
    // Create a new test track going from the origin down the line y = -x
    Track track2(V3D(0,0,0),V3D(0,1,-1));
    TS_ASSERT_EQUALS( ocyl.interceptSurface(track2), 1 )
    Track::LType::const_iterator it2 = track2.begin();
    if (it2 == track2.end()) return;
    TS_ASSERT_DELTA( it2->Dist, sqrt(2*10.5*10.5), 0.0001 )
    TS_ASSERT_DELTA( it2->Length, sqrt(2.0), 0.0001 )
    TS_ASSERT_EQUALS( it2->PtA, V3D(0,9.5,-9.5) )
    TS_ASSERT_EQUALS( it2->PtB, V3D(0,10.5,-10.5) )

    // Calling on an ObjComponent without an associated geometric object will throw
    ObjComponent comp("noShape");
    TS_ASSERT_THROWS( comp.interceptSurface(track), Exception::NullPointerException )
  }

  void testSolidAngleCappedCylinder()
  {
    ObjComponent A("ocyl", createCappedCylinder());
    A.setPos(10,0,0);
    A.setRot(Quat(90.0,V3D(0,0,1)));
    double satol=2e-2; // tolerance for solid angle

    TS_ASSERT_DELTA(A.solidAngle(V3D(10,1.7,0)),1.840302,satol);
    // Surface point
    TS_ASSERT_DELTA(A.solidAngle(V3D(10,-1,0.5)),2*M_PI,satol);

    // Add a parent with a rotation of its own;
    Component parent("parent",V3D(0,10,0),Quat(90.0,V3D(0,1,0)));
    A.setParent(&parent);

    // See testSolidAngleCappedCylinder in ObjectTest - these tests are a subset of them
    TS_ASSERT_DELTA(A.solidAngle(V3D(0,10,-11.7)),1.840302,satol);
    TS_ASSERT_DELTA(A.solidAngle(V3D(0,10,-6.13333333)),1.25663708,satol);
    // internal point (should be 4pi)
    TS_ASSERT_DELTA(A.solidAngle(V3D(0,10,-10)),4*M_PI,satol);
    // surface point
    TS_ASSERT_DELTA(A.solidAngle(V3D(0.5,10,-10)),2*M_PI,satol);

    // Calling on an ObjComponent without an associated geometric object will throw
    ObjComponent B("noShape");
    TS_ASSERT_THROWS( B.solidAngle(V3D(1,2,3)), Exception::NullPointerException )
  }

private:
  boost::shared_ptr<Object> createCappedCylinder()
  {
    std::string C31="cx 0.5";         // cylinder x-axis radius 0.5
    std::string C32="px 1.2";
    std::string C33="px -3.2";

    // First create some surfaces
    std::map<int,Surface*> CylSurMap;
    CylSurMap[31]=new Cylinder();
    CylSurMap[32]=new Plane();
    CylSurMap[33]=new Plane();

    CylSurMap[31]->setSurface(C31);
    CylSurMap[32]->setSurface(C32);
    CylSurMap[33]->setSurface(C33);
    CylSurMap[31]->setName(31);
    CylSurMap[32]->setName(32);
    CylSurMap[33]->setName(33);

    // Capped cylinder (id 21)
    // using surface ids: 31 (cylinder) 32 (plane (top) ) and 33 (plane (base))
    std::string ObjCapCylinder="-31 -32 33";

    boost::shared_ptr<Object> retVal = boost::shared_ptr<Object>(new Object);
    retVal->setObject(21,ObjCapCylinder);
    retVal->populate(CylSurMap);

    return retVal;
  }
};

#endif
