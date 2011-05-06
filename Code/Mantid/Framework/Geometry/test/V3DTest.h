#ifndef MANTID_TESTV3D__
#define MANTID_TESTV3D__

#include <cxxtest/TestSuite.h>
#include <cmath>
#include <ostream>
#include <vector>

#include "MantidGeometry/V3D.h"

using namespace Mantid::Geometry;

class V3DTest : public CxxTest::TestSuite
{
private:

  Mantid::Geometry::V3D a,b,c,d;

public:
	void testEmptyConstructor()
	{
		// very important as a MD geometry rely on it later
		TS_ASSERT_EQUALS(a.X(),0.0);
		TS_ASSERT_EQUALS(a.Y(),0.0);
		TS_ASSERT_EQUALS(a.Z(),0.0);
	}
	void testDefaultConstructor()
	{
		Mantid::Geometry::V3D d(1.0,2.0,3.0);
		TS_ASSERT_EQUALS(d.X(),1.0);
		TS_ASSERT_EQUALS(d.Y(),2.0);
		TS_ASSERT_EQUALS(d.Z(),3.0);
	}
	void testAssignment()
	{
		a(1.0,1.0,1.0);
		TS_ASSERT_EQUALS(a.X(),1.0);
		TS_ASSERT_EQUALS(a.Y(),1.0);
		TS_ASSERT_EQUALS(a.Z(),1.0);
	}
	void testcopyConstructor()
	{
		a(2.0,2.0,2.0);
		Mantid::Geometry::V3D d(a);
		TS_ASSERT_EQUALS(d.X(),2.0);
		TS_ASSERT_EQUALS(d.Y(),2.0);
		TS_ASSERT_EQUALS(d.Z(),2.0);
	}
	void testOperatorEqual()
	{
		a(-1.0,-1.0,-1.0);
		b=a;
		TS_ASSERT_EQUALS(b.X(),-1.0);
		TS_ASSERT_EQUALS(b.Y(),-1.0);
		TS_ASSERT_EQUALS(b.Z(),-1.0);
	}
	void testConstructorPointer()
	{
		double* t=new double[3];
		t[0]=1.0;t[1]=2.0;t[2]=3.0;
		Mantid::Geometry::V3D d(t);
		TS_ASSERT_EQUALS(d.X(),1.0);
		TS_ASSERT_EQUALS(d.Y(),2.0);
		TS_ASSERT_EQUALS(d.Z(),3.0);
		delete[] t;
	}
	void testPlusOperation()
	{
		a(1.0,1.0,1.0);
		b(2.0,3.0,4.0);
		c=a+b;
		TS_ASSERT_EQUALS(c.X(),3.0);
		TS_ASSERT_EQUALS(c.Y(),4.0);
		TS_ASSERT_EQUALS(c.Z(),5.0);
	}
	void testMinusOperation()
	{
		a(1.0,2.0,3.0);
		b(1.0,2.0,3.0);
		c=a-b;
		TS_ASSERT_EQUALS(c.X(),0.0);
		TS_ASSERT_EQUALS(c.Y(),0.0);
		TS_ASSERT_EQUALS(c.Z(),0.0);
	}
	void testMultipliesOperation()
	{
		a(1.0,2.0,3.0);
		b(1.0,2.0,3.0);
		c=a*b;
		TS_ASSERT_EQUALS(c.X(),1.0);
		TS_ASSERT_EQUALS(c.Y(),4.0);
		TS_ASSERT_EQUALS(c.Z(),9.0);
		a*=a;
        // I want a TS_ASSERT here... a==c
	}
	void testDividesOperation()
	{
		a(1.0,2.0,3.0);
		b(1.0,2.0,3.0);
		c=a/b;
		TS_ASSERT_EQUALS(c.X(),1.0);
		TS_ASSERT_EQUALS(c.Y(),1.0);
		TS_ASSERT_EQUALS(c.Z(),1.0);
	}
	void testPlusEqualOperation()
	{
		a(1.0,2.0,3.0);
		b(0.0,0.0,0.0);
		b+=a;
		TS_ASSERT_EQUALS(b.X(),1.0);
		TS_ASSERT_EQUALS(b.Y(),2.0);
		TS_ASSERT_EQUALS(b.Z(),3.0);
	}
	void testMinusEqualOperation()
	{
		a(1.0,2.0,3.0);
		b(0.0,0.0,0.0);
		b-=a;
		TS_ASSERT_EQUALS(b.X(),-1.0);
		TS_ASSERT_EQUALS(b.Y(),-2.0);
		TS_ASSERT_EQUALS(b.Z(),-3.0);
	}
	void testMultipliesEqualOperation()
	{
		a(1.0,2.0,3.0);
		b(2.0,2.0,2.0);
		b*=a;
		TS_ASSERT_EQUALS(b.X(),2.0);
		TS_ASSERT_EQUALS(b.Y(),4.0);
		TS_ASSERT_EQUALS(b.Z(),6.0);
	}
	void testDividesEqualOperation()
	{
		a(1.0,2.0,3.0);
		b(2.0,2.0,2.0);
		b/=a;
		TS_ASSERT_EQUALS(b.X(),2.0);
		TS_ASSERT_EQUALS(b.Y(),1.0);
		TS_ASSERT_EQUALS(b.Z(),2.0/3.0);
	}
	void testScaleMultiplies()
	{
		a(1.0,2.0,3.0);
		b=a * -2.0;
		TS_ASSERT_EQUALS(b.X(),-2.0);
		TS_ASSERT_EQUALS(b.Y(),-4.0);
		TS_ASSERT_EQUALS(b.Z(),-6.0);
	}
	void testScaleMultipliesEqual()
	{
		a(1.0,2.0,3.0);
		a*=2.0;
		TS_ASSERT_EQUALS(a.X(),2.0);
		TS_ASSERT_EQUALS(a.Y(),4.0);
		TS_ASSERT_EQUALS(a.Z(),6.0);
	}
	void testScaleDivides()
	{
		a(1.0,2.0,3.0);
		b=a/2.0;
		TS_ASSERT_EQUALS(b.X(),0.5);
		TS_ASSERT_EQUALS(b.Y(),1.0);
		TS_ASSERT_EQUALS(b.Z(),1.5);
	}
	void testScaleDividesEqual()
	{
		a(1.0,2.0,3.0);
		a/=2.0;
		TS_ASSERT_EQUALS(a.X(),0.5);
		TS_ASSERT_EQUALS(a.Y(),1.0);
		TS_ASSERT_EQUALS(a.Z(),1.5);
	}
	void testEqualEqualOperator()
	{
		a(1.0,1.0,1.0);
		b=a;
		TS_ASSERT(a==b);
	}
	void testLessStrictOperator()
	{
		a(1.0,1.0,1.0);
		b(2.0,1.0,0.0);
		TS_ASSERT(a<b);
		a(1.0,1.0,1.0);
		b(1.0,2.0,0.0);
		TS_ASSERT(a<b);
		a(1.0,1.0,1.0);
		b(1.0,1.0,2.0);
		TS_ASSERT(a<b);
		b=a;
		TS_ASSERT(!(a<b));
	}
	void testGetX()
	{
		a(1.0,0.0,0.0);
		TS_ASSERT_EQUALS(a.X(),1.0);
	}
	void testGetY()
	{
		a(1.0,2.0,0.0);
		TS_ASSERT_EQUALS(a.Y(),2.0);
	}
	void testGetZ()
	{
		a(1.0,0.0,3.0);
		TS_ASSERT_EQUALS(a.Z(),3.0);
	}
	void testOperatorBracketNonConst()
	{
		a(1.0,2.0,3.0);
		TS_ASSERT_EQUALS(a[0],1.0);
		TS_ASSERT_EQUALS(a[1],2.0);
		TS_ASSERT_EQUALS(a[2],3.0);
		a[0]=-1.0;
		a[1]=-2.0;
		a[2]=-3.0;
		TS_ASSERT_EQUALS(a[0],-1.0);
		TS_ASSERT_EQUALS(a[1],-2.0);
		TS_ASSERT_EQUALS(a[2],-3.0);
	}
	void testOperatorBracketConst()
	{
		const Mantid::Geometry::V3D d(1.0,2.0,3.0);
		TS_ASSERT_EQUALS(d[0],1.0);
		TS_ASSERT_EQUALS(d[1],2.0);
		TS_ASSERT_EQUALS(d[2],3.0);
	}
	void testOperatorBracketNonConstThrows()
	{
		TS_ASSERT_THROWS(a[-1],std::runtime_error&);
		TS_ASSERT_THROWS(a[3],std::runtime_error&);
	}
	void testOperatorBracketConstThrows()
	{
		const Mantid::Geometry::V3D d(1.0,2.0,3.0);
		TS_ASSERT_THROWS(d[-1],std::runtime_error&);
		TS_ASSERT_THROWS(d[3],std::runtime_error&);
	}
	void testNorm()
	{
		a(1.0,-5.0,8.0);
		TS_ASSERT_EQUALS(a.norm(),sqrt(90.0));
	}
	void testNorm2()
	{
		a(1.0,-5.0,8.0);
		TS_ASSERT_EQUALS(a.norm2(),90.0);
	}
	void testNormalize()
	{
		a(1.0,1.0,1.0);
		b=a;
		b.normalize();
		TS_ASSERT_EQUALS(b[0],1.0/sqrt(3.0));
		TS_ASSERT_EQUALS(b[1],1.0/sqrt(3.0));
		TS_ASSERT_EQUALS(b[2],1.0/sqrt(3.0));
	}
	void testScalarProduct()
	{
		a(1.0,2.0,1.0);
		b(1.0,-2.0,-1.0);
		double sp=a.scalar_prod(b);
		TS_ASSERT_EQUALS(sp,-4.0);
	}
	void testCrossProduct()
	{
		a(1.0,0.0,0.0);
		b(0.0,1.0,0.0);
		c=a.cross_prod(b);
		TS_ASSERT_EQUALS(c[0],0.0);
		TS_ASSERT_EQUALS(c[1],0.0);
		TS_ASSERT_EQUALS(c[2],1.0);
	}
	void testDistance()
	{
		a(0.0,0.0,0.0);
		b(2.0,2.0,2.0);
		double d=a.distance(b);
		TS_ASSERT_EQUALS(d,2.0*sqrt(3.0));
	}

	void testZenith()
	{
	  b(0.0,0.0,0.0);
	  a(9.9,7.6,0.0);
	  TS_ASSERT_EQUALS( a.zenith(a), 0.0 );
	  TS_ASSERT_DELTA( a.zenith(b), M_PI/2.0, 0.0001 );
	  a(-1.1,0.0,0.0);
    TS_ASSERT_DELTA( a.zenith(b), M_PI/2.0, 0.0001 );
    a(0.0,0.0,1.0);
    TS_ASSERT_EQUALS( a.zenith(b), 0.0 );
    a(1.0,0.0,1.0);
    TS_ASSERT_DELTA( a.zenith(b), M_PI/4.0, 0.0001 );
    a(1.0,0.0,-1.0);
    TS_ASSERT_DELTA( a.zenith(b), 3.0*M_PI/4.0, 0.0001 );
	}

	void testAngle()
	{
	  a(2.0,0.0,0.0);
	  b(0.0,1.0,0.0);
	  c(1.0,1.0,0.0);
	  d(-1.0,0.0,0.0);
	  TS_ASSERT_DELTA( a.angle(a), 0.0, 0.0001 );
	  TS_ASSERT_DELTA( a.angle(b), M_PI/2.0, 0.0001 );
    TS_ASSERT_DELTA( a.angle(c), M_PI/4.0, 0.0001 );
    TS_ASSERT_DELTA( a.angle(d), M_PI, 0.0001 );
	}

  void testSpherical()
  {
    double r=3,theta=45.0,phi=45.0;
    a(0.0,0.0,0.0);
    b(0.0,0.0,0.0);
    b.spherical(r,theta,phi);
    double d=a.distance(b);
    TS_ASSERT_DELTA(d,r,0.0001);
    TS_ASSERT_DELTA(b.X(), 1.5, 0.0001 );
    TS_ASSERT_DELTA(b.Y(), 1.5, 0.0001 );
    TS_ASSERT_DELTA(b.Z(), 3.0/sqrt(2.0), 0.0001 );
    // Test getSpherical returns the original values
    TS_ASSERT_THROWS_NOTHING( b.getSpherical(r,theta,phi) );
    TS_ASSERT_EQUALS( r,     3.0 );
    TS_ASSERT_EQUALS( theta, 45.0 );
    TS_ASSERT_EQUALS( phi,   45.0 );
  }

  void test_spherical_rad()
  {
    a(0.0,0.0,0.0);
    a.spherical_rad(1, 0, 0);
    TS_ASSERT(a == V3D(0, 0, 1) );
    a.spherical_rad(1, M_PI/2, 0);
    TS_ASSERT(a == V3D(1, 0, 0) );
    a.spherical_rad(1, M_PI/2, M_PI/2);
    TS_ASSERT(a == V3D(0, 1, 0) );
    a.spherical_rad(1, M_PI, 0);
    TS_ASSERT(a == V3D(0, 0, -1) );
    a.spherical_rad(2, M_PI/4, 0);
    TS_ASSERT(a == V3D(sqrt(2.0), 0, sqrt(2.0)) );
  }

  void test_azimuth_polar_SNS()
  {
    a(0.0,0.0,0.0);
    a.azimuth_polar_SNS( 1.0, 0, M_PI/2);
    TS_ASSERT(a == V3D(1.0, 0, 0) );
    a.azimuth_polar_SNS( 1.0, M_PI/2, M_PI/2);
    TS_ASSERT(a == V3D(0.0, 0, 1.0) );
    a.azimuth_polar_SNS( 2, 0, 0);
    TS_ASSERT(a == V3D(0, 2, 0) );
    a.azimuth_polar_SNS( 2, 0, M_PI);
    TS_ASSERT(a == V3D(0, -2, 0) );
    a.azimuth_polar_SNS( 2, 0, M_PI/4);
    TS_ASSERT(a == V3D(sqrt(2.0), sqrt(2.0), 0) );
  }

  /** Round each component to the nearest integer */
  void test_round()
  {
    a(1.2, 0.9, 4.34);
    a.round();
    TS_ASSERT(a == V3D(1.0, 1.0, 4.0) );

    a(-1.2, -1.9, -3.9);
    a.round();
    TS_ASSERT(a == V3D(-1.0, -2.0, -4.0) );
  }

};

#endif
