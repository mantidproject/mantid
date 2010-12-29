#ifndef MANTID_TESTQUAT__
#define MANTID_TESTQUAT__

#include <cxxtest/TestSuite.h>
#include <cmath>
#include <ostream>
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/Quat.h" 

using namespace Mantid::Geometry;

class testQuat : public CxxTest::TestSuite
{
private:

  Mantid::Geometry::Quat q,p;

public:
  void testoperatorbracket()
  {
    p[0]=0;
    p[1]=1;
    p[2]=2;
    p[3]=3;
    TS_ASSERT_EQUALS(p[0],0.0);
    TS_ASSERT_EQUALS(p[1],1.0);
    TS_ASSERT_EQUALS(p[2],2.0);
    TS_ASSERT_EQUALS(p[3],3.0);
  }

  void testEmptyConstructor()
  {
    TS_ASSERT_EQUALS(q[0],1.0);
    TS_ASSERT_EQUALS(q[1],0.0);
    TS_ASSERT_EQUALS(q[2],0.0);
    TS_ASSERT_EQUALS(q[3],0.0);
  }

  void testValueConstructor()
  {
    Mantid::Geometry::Quat q1(1,2,3,4);
    TS_ASSERT_EQUALS(q1[0],1.0);
    TS_ASSERT_EQUALS(q1[1],2.0);
    TS_ASSERT_EQUALS(q1[2],3.0);
    TS_ASSERT_EQUALS(q1[3],4.0);
  }

  void testAngleAxisConstructor()
  {
    Mantid::Geometry::V3D v(1,1,1);
    // Construct quaternion to represent rotation
    // of 45 degrees around the 111 axis.
    Mantid::Geometry::Quat q1(90.0,v);
    double c=1.0/sqrt(2.0);
    double s=c/sqrt(3.0);
    TS_ASSERT_DELTA(q1[0],c,0.000001);
    TS_ASSERT_DELTA(q1[1],s,0.000001);
    TS_ASSERT_DELTA(q1[2],s,0.000001);
    TS_ASSERT_DELTA(q1[3],s,0.000001);
  }

  void testoperatorassignmentfromdouble()
  {
    q(2,3,4,5);
    TS_ASSERT_EQUALS(q[0],2.0);
    TS_ASSERT_EQUALS(q[1],3.0);
    TS_ASSERT_EQUALS(q[2],4.0);
    TS_ASSERT_EQUALS(q[3],5.0);
  }

  void testoperatorassignmentfromangleaxis()
  {
    Mantid::Geometry::V3D v(1,1,1);
    q(90.0,v);
    double c=1.0/sqrt(2.0);
    double s=c/sqrt(3.0);
    TS_ASSERT_DELTA(q[0],c,0.000001);
    TS_ASSERT_DELTA(q[1],s,0.000001);
    TS_ASSERT_DELTA(q[2],s,0.000001);
    TS_ASSERT_DELTA(q[3],s,0.000001);

    //Now rotate 45 degrees around y
    q(45, V3D(0,1,0));
    V3D X(1,0,0);
    q.rotate(X);
    double a = sqrt(2.0)/2;
    TS_ASSERT(X==V3D(a,0,-a));
    //Now rotate -45 degrees around y
    q(-45, V3D(0,1,0));
    X(1,0,0);
    q.rotate(X);
    TS_ASSERT(X==V3D(a,0,a));
  }

  void testoperatorequal()
  {
    q=p;
    TS_ASSERT_EQUALS(q[0],p[0]);
    TS_ASSERT_EQUALS(q[1],p[1]);
    TS_ASSERT_EQUALS(q[2],p[2]);
    TS_ASSERT_EQUALS(q[3],p[3]);
  }

  void testlenmethod()
  {
    q(1,2,3,4);
    TS_ASSERT_EQUALS(q.len(),sqrt(30.0));
  }

  void testlen2method()
  {
    q(1,2,3,4);
    TS_ASSERT_EQUALS(q.len2(),30.0);
  }

  void testinitmehtod()
  {
    q.init();
    TS_ASSERT_EQUALS(q[0],1);
    TS_ASSERT_EQUALS(q[1],0);
    TS_ASSERT_EQUALS(q[2],0);
    TS_ASSERT_EQUALS(q[3],0);
  }

  void testnormalizemethod()
  {
    q(2,2,2,2);
    q.normalize();
    TS_ASSERT_DELTA(q[0],0.5,0.000001);
    TS_ASSERT_DELTA(q[1],0.5,0.000001);
    TS_ASSERT_DELTA(q[2],0.5,0.000001);
    TS_ASSERT_DELTA(q[3],0.5,0.000001);
  }

  void testconjugatemethod()
  {
    q(1,1,1,1);
    q.conjugate();
    TS_ASSERT_EQUALS(q[0],1);
    TS_ASSERT_EQUALS(q[1],-1);
    TS_ASSERT_EQUALS(q[2],-1);
    TS_ASSERT_EQUALS(q[3],-1);
  }

  void testinversemethod()
  {
    q(2,3,4,5);
    Mantid::Geometry::Quat qinv(q);
    qinv.inverse();
    q*=qinv;
    TS_ASSERT_DELTA(q[0],1,0.000001);
    TS_ASSERT_DELTA(q[1],0,0.000001);
    TS_ASSERT_DELTA(q[2],0,0.000001);
    TS_ASSERT_DELTA(q[3],0,0.000001);
  }

  void testoperatorplus()
  {
    q(1,1,1,1);
    p(-1,2,1,3);
    Mantid::Geometry::Quat res;
    res=p+q;
    TS_ASSERT_EQUALS(res[0],0);
    TS_ASSERT_EQUALS(res[1],3);
    TS_ASSERT_EQUALS(res[2],2);
    TS_ASSERT_EQUALS(res[3],4);
  }

  void testoperatorminus()
  {
    q(1,1,1,1);
    p(-1,2,1,3);
    Mantid::Geometry::Quat res;
    res=p-q;
    TS_ASSERT_EQUALS(res[0],-2);
    TS_ASSERT_EQUALS(res[1],1);
    TS_ASSERT_EQUALS(res[2],0);
    TS_ASSERT_EQUALS(res[3],2);
  }

  void testoperatortimes()
  {
    q(1,1,1,1);
    p(-1,2,1,3);
    Mantid::Geometry::Quat res;
    res=p*q;
    TS_ASSERT_EQUALS(res[0],-7);
    TS_ASSERT_EQUALS(res[1],-1);
    TS_ASSERT_EQUALS(res[2],1);
    TS_ASSERT_EQUALS(res[3],3);
  }

  void testoperatordoublequal()
  {
    p=q;
    TS_ASSERT(p==q);
    q(1,4,5,6);
    TS_ASSERT(p!=q);
  }

  void testoperatornotequal()
  {
    q(1,2,3,4);
    TS_ASSERT(p!=q);
    p=q;
    TS_ASSERT(!(p!=q));
  }

  void testRotateVector()
  {
    double a = sqrt(2.0)/2;

    //Trivial
    p(1,0,0,0);//Identity quaternion
    V3D v(1,0,0);
    V3D orig_v = v;
    p.rotate(v);
    TS_ASSERT(orig_v==v);

    //Now do more angles
    v = V3D(1,0,0);
    p(90., V3D(0,1,0)); //90 degrees, right-handed, around y
    p.rotate(v);
    TS_ASSERT(v==V3D(0,0,-1));

    v = V3D(1,0,0);
    p(45., V3D(0,0,1));
    p.rotate(v);
    TS_ASSERT(v==V3D(a, a, 0));

    v = V3D(1,0,0);
    p(-45., V3D(0,0,1));
    p.rotate(v);
    TS_ASSERT(v==V3D(a, -a, 0));

    v = V3D(1,0,0);
    p(30., V3D(0,0,1));
    p.rotate(v);
    TS_ASSERT(v==V3D(sqrt(3.0)/2, 0.5, 0));

    v = V3D(1,0,0);
    p(125., V3D(1,0,0));
    p.rotate(v);
    TS_ASSERT(v==V3D(1,0,0));

    //90 deg around +Z
    p(90, V3D(0,0,1));
    v = V3D(1,0,0);    p.rotate(v);    TS_ASSERT(v==V3D(0,1,0));
    v = V3D(0,1,0);    p.rotate(v);    TS_ASSERT(v==V3D(-1,0,0));

    //std::cout << "Rotated v is" << v << "\n";
  }

  void testSetFromDirectionCosineMatrix_trival()
  {
    Mantid::Geometry::V3D rX(1,0,0);
    Mantid::Geometry::V3D rY(0,1,0);
    Mantid::Geometry::V3D rZ(0,0,1);
    q(rX,rY,rZ);
    p(1,0,0,0); //Identity quaternion
    TS_ASSERT(p==q); //Trivial rotation
  }

  void testSetFromDirectionCosineMatrix2()
  {
    //Rotate 90 deg around Y
    V3D rX(0,0,-1);
    V3D rY(0,1,0);
    V3D rZ(1,0,0);
    q(rX,rY,rZ);
    p(90, V3D(0,1,0));
    TS_ASSERT(p==q);
  }

  void testSetFromDirectionCosineMatrix2b()
  {
    //Rotate -45 deg around Y
    double a = sqrt(2.0)/2;
    V3D rX(a,0,a);
    V3D rY(0,1,0);
    V3D rZ(-a,0,a);
    q(rX,rY,rZ);
    p(-45.0, V3D(0,1,0));
    TS_ASSERT(p==q);

    V3D oX(1,0,0);
    V3D oY(0,1,0);
    V3D oZ(0,0,1);
    q.rotate(oX);
    q.rotate(oY);
    q.rotate(oZ);
    TS_ASSERT(oX==rX);
    TS_ASSERT(oY==rY);
    TS_ASSERT(oZ==rZ);
  }

  void testSetFromDirectionCosineMatrix3()
  {
    //Rotate 90 deg around Z
    V3D rX(0,1,0);
    V3D rY(-1,0,0);
    V3D rZ(0,0,1);
    q(rX,rY,rZ);
    p(90, V3D(0,0,1));
    TS_ASSERT(p==q);
  }

  void testSetFromDirectionCosineMatrix4()
  {
    //Rotate 90 deg around X
    V3D rX(1,0,0);
    V3D rY(0,0,1);
    V3D rZ(0,-1,0);
    q(rX,rY,rZ);
    p(90, V3D(1,0,0));
    TS_ASSERT(p==q);
  }

  void compareArbitrary(const Quat& rotQ)
  {
    V3D oX(1,0,0);
    V3D oY(0,1,0);
    V3D oZ(0,0,1);
    V3D rX = oX;
    V3D rY = oY;
    V3D rZ = oZ;

    //Rotate the reference frame
    rotQ.rotate(rX);
    rotQ.rotate(rY);
    rotQ.rotate(rZ);

    //Now find it.
    q(rX,rY,rZ);

    q.rotate(oX);
    q.rotate(oY);
    q.rotate(oZ);
    TS_ASSERT(oX==rX);
    TS_ASSERT(oY==rY);
    TS_ASSERT(oZ==rZ);
    TS_ASSERT(rotQ==q);

    //    std::cout << "\nRotated coordinates are " << rX << rY << rZ << "\n";
    //    std::cout << "Expected (p) is" << p << "; got " << q << "\n";
    //    std::cout << "Re-Rotated coordinates are " << oX << oY << oZ << "\n";
  }

  void testSetFromDirectionCosineMatrix_arbitrary()
  {
    Quat rotQ;
    //Try a couple of random rotations
    rotQ = Quat(124.0, V3D(0.1, 0.2, sqrt(0.95)));
    this->compareArbitrary(rotQ);
    rotQ = Quat(-546.0, V3D(-0.5, 0.5, sqrt(0.5)));
    this->compareArbitrary(rotQ);
    rotQ = Quat(34.0, V3D(-0.5, 0.5, sqrt(0.5))) * Quat(-25.0, V3D(0.1, 0.2, sqrt(0.95)));
    this->compareArbitrary(rotQ);
  }

  void testConstructorFromDirectionCosine()
  {
    double a = sqrt(2.0)/2;
    V3D rX(a,0,a);
    V3D rY(0,1,0);
    V3D rZ(-a,0,a);
    Quat rotQ = Quat(rX,rY,rZ);
    p(-45.0, V3D(0,1,0));
    TS_ASSERT(rotQ==p);
  }

};

#endif
