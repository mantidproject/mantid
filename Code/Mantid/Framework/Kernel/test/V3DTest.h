#ifndef MANTID_TESTV3D__
#define MANTID_TESTV3D__

#include <cxxtest/TestSuite.h>
#include <cmath>
#include <ostream>
#include <sstream>
#include <vector>

#include "MantidKernel/V3D.h"
#include "MantidTestHelpers/NexusTestHelper.h"

using Mantid::Kernel::V3D;

class V3DTest: public CxxTest::TestSuite
{
private:

  Mantid::Kernel::V3D a, b, c, d;

public:
  void testEmptyConstructor()
  {
    // very important as a MD geometry rely on it later
    TS_ASSERT_EQUALS(a.X(), 0.0);
    TS_ASSERT_EQUALS(a.Y(), 0.0);
    TS_ASSERT_EQUALS(a.Z(), 0.0);
  }
  void testDefaultConstructor()
  {
    Mantid::Kernel::V3D d(1.0, 2.0, 3.0);
    TS_ASSERT_EQUALS(d.X(), 1.0);
    TS_ASSERT_EQUALS(d.Y(), 2.0);
    TS_ASSERT_EQUALS(d.Z(), 3.0);
  }
  void testAssignment()
  {
    a(1.0, 1.0, 1.0);
    TS_ASSERT_EQUALS(a.X(), 1.0);
    TS_ASSERT_EQUALS(a.Y(), 1.0);
    TS_ASSERT_EQUALS(a.Z(), 1.0);
  }
  void testcopyConstructor()
  {
    a(2.0, 2.0, 2.0);
    Mantid::Kernel::V3D d(a);
    TS_ASSERT_EQUALS(d.X(), 2.0);
    TS_ASSERT_EQUALS(d.Y(), 2.0);
    TS_ASSERT_EQUALS(d.Z(), 2.0);
  }
  void testOperatorEqual()
  {
    a(-1.0, -1.0, -1.0);
    b = a;
    TS_ASSERT_EQUALS(b.X(), -1.0);
    TS_ASSERT_EQUALS(b.Y(), -1.0);
    TS_ASSERT_EQUALS(b.Z(), -1.0);
  }

  void testPlusOperation()
  {
    a(1.0, 1.0, 1.0);
    b(2.0, 3.0, 4.0);
    c = a + b;
    TS_ASSERT_EQUALS(c.X(), 3.0);
    TS_ASSERT_EQUALS(c.Y(), 4.0);
    TS_ASSERT_EQUALS(c.Z(), 5.0);
  }
  void testMinusOperation()
  {
    a(1.0, 2.0, 3.0);
    b(1.0, 2.0, 3.0);
    c = a - b;
    TS_ASSERT_EQUALS(c.X(), 0.0);
    TS_ASSERT_EQUALS(c.Y(), 0.0);
    TS_ASSERT_EQUALS(c.Z(), 0.0);
  }
  void testMultipliesOperation()
  {
    a(1.0, 2.0, 3.0);
    b(1.0, 2.0, 3.0);
    c = a * b;
    TS_ASSERT_EQUALS(c.X(), 1.0);
    TS_ASSERT_EQUALS(c.Y(), 4.0);
    TS_ASSERT_EQUALS(c.Z(), 9.0);
    a *= a;
    // I want a TS_ASSERT here... a==c
  }
  void testDividesOperation()
  {
    a(1.0, 2.0, 3.0);
    b(1.0, 2.0, 3.0);
    c = a / b;
    TS_ASSERT_EQUALS(c.X(), 1.0);
    TS_ASSERT_EQUALS(c.Y(), 1.0);
    TS_ASSERT_EQUALS(c.Z(), 1.0);
  }
  void testPlusEqualOperation()
  {
    a(1.0, 2.0, 3.0);
    b(0.0, 0.0, 0.0);
    b += a;
    TS_ASSERT_EQUALS(b.X(), 1.0);
    TS_ASSERT_EQUALS(b.Y(), 2.0);
    TS_ASSERT_EQUALS(b.Z(), 3.0);
  }
  void testMinusEqualOperation()
  {
    a(1.0, 2.0, 3.0);
    b(0.0, 0.0, 0.0);
    b -= a;
    TS_ASSERT_EQUALS(b.X(), -1.0);
    TS_ASSERT_EQUALS(b.Y(), -2.0);
    TS_ASSERT_EQUALS(b.Z(), -3.0);
  }
  void testMultipliesEqualOperation()
  {
    a(1.0, 2.0, 3.0);
    b(2.0, 2.0, 2.0);
    b *= a;
    TS_ASSERT_EQUALS(b.X(), 2.0);
    TS_ASSERT_EQUALS(b.Y(), 4.0);
    TS_ASSERT_EQUALS(b.Z(), 6.0);
  }
  void testDividesEqualOperation()
  {
    a(1.0, 2.0, 3.0);
    b(2.0, 2.0, 2.0);
    b /= a;
    TS_ASSERT_EQUALS(b.X(), 2.0);
    TS_ASSERT_EQUALS(b.Y(), 1.0);
    TS_ASSERT_EQUALS(b.Z(), 2.0/3.0);
  }
  void testScaleMultiplies()
  {
    a(1.0, 2.0, 3.0);
    b = a * -2.0;
    TS_ASSERT_EQUALS(b.X(), -2.0);
    TS_ASSERT_EQUALS(b.Y(), -4.0);
    TS_ASSERT_EQUALS(b.Z(), -6.0);
  }
  void testScaleMultipliesEqual()
  {
    a(1.0, 2.0, 3.0);
    a *= 2.0;
    TS_ASSERT_EQUALS(a.X(), 2.0);
    TS_ASSERT_EQUALS(a.Y(), 4.0);
    TS_ASSERT_EQUALS(a.Z(), 6.0);
  }
  void testScaleDivides()
  {
    a(1.0, 2.0, 3.0);
    b = a / 2.0;
    TS_ASSERT_EQUALS(b.X(), 0.5);
    TS_ASSERT_EQUALS(b.Y(), 1.0);
    TS_ASSERT_EQUALS(b.Z(), 1.5);
  }
  void testScaleDividesEqual()
  {
    a(1.0, 2.0, 3.0);
    a /= 2.0;
    TS_ASSERT_EQUALS(a.X(), 0.5);
    TS_ASSERT_EQUALS(a.Y(), 1.0);
    TS_ASSERT_EQUALS(a.Z(), 1.5);
  }
  void testEqualEqualOperator()
  {
    a(1.0, 1.0, 1.0);
    b = a;
    TS_ASSERT(a==b);
  }
  void testLessStrictOperator()
  {
    a(1.0, 1.0, 1.0);
    b(2.0, 1.0, 0.0);
    TS_ASSERT(a<b);
    a(1.0, 1.0, 1.0);
    b(1.0, 2.0, 0.0);
    TS_ASSERT(a<b);
    a(1.0, 1.0, 1.0);
    b(1.0, 1.0, 2.0);
    TS_ASSERT(a<b);
    b = a;
    TS_ASSERT(!(a<b));
  }
  void testGetX()
  {
    a(1.0, 0.0, 0.0);
    TS_ASSERT_EQUALS(a.X(), 1.0);
  }
  void testGetY()
  {
    a(1.0, 2.0, 0.0);
    TS_ASSERT_EQUALS(a.Y(), 2.0);
  }
  void testGetZ()
  {
    a(1.0, 0.0, 3.0);
    TS_ASSERT_EQUALS(a.Z(), 3.0);
  }
  void testOperatorBracketNonConst()
  {
    a(1.0, 2.0, 3.0);
    TS_ASSERT_EQUALS(a[0], 1.0);
    TS_ASSERT_EQUALS(a[1], 2.0);
    TS_ASSERT_EQUALS(a[2], 3.0);
    a[0] = -1.0;
    a[1] = -2.0;
    a[2] = -3.0;
    TS_ASSERT_EQUALS(a[0], -1.0);
    TS_ASSERT_EQUALS(a[1], -2.0);
    TS_ASSERT_EQUALS(a[2], -3.0);
  }
  void testOperatorBracketConst()
  {
    const Mantid::Kernel::V3D d(1.0, 2.0, 3.0);
    TS_ASSERT_EQUALS(d[0], 1.0);
    TS_ASSERT_EQUALS(d[1], 2.0);
    TS_ASSERT_EQUALS(d[2], 3.0);
  }
  void testOperatorBracketNonConstThrows()
  {
    TS_ASSERT_THROWS(a[-1], std::runtime_error&);
    TS_ASSERT_THROWS(a[3], std::runtime_error&);
  }
  void testOperatorBracketConstThrows()
  {
    const Mantid::Kernel::V3D d(1.0, 2.0, 3.0);
    TS_ASSERT_THROWS(d[-1], std::runtime_error&);
    TS_ASSERT_THROWS(d[3], std::runtime_error&);
  }
  void testNorm()
  {
    a(1.0, -5.0, 8.0);
    TS_ASSERT_EQUALS(a.norm(), sqrt(90.0));
  }
  void testNorm2()
  {
    a(1.0, -5.0, 8.0);
    TS_ASSERT_EQUALS(a.norm2(), 90.0);
  }
  void testNormalize()
  {
    a(1.0, 1.0, 1.0);
    b = a;
    b.normalize();
    TS_ASSERT_EQUALS(b[0], 1.0/sqrt(3.0));
    TS_ASSERT_EQUALS(b[1], 1.0/sqrt(3.0));
    TS_ASSERT_EQUALS(b[2], 1.0/sqrt(3.0));
  }
  void testScalarProduct()
  {
    a(1.0, 2.0, 1.0);
    b(1.0, -2.0, -1.0);
    double sp = a.scalar_prod(b);
    TS_ASSERT_EQUALS(sp, -4.0);
  }
  void testCrossProduct()
  {
    a(1.0, 0.0, 0.0);
    b(0.0, 1.0, 0.0);
    c = a.cross_prod(b);
    TS_ASSERT_EQUALS(c[0], 0.0);
    TS_ASSERT_EQUALS(c[1], 0.0);
    TS_ASSERT_EQUALS(c[2], 1.0);
  }
  void testDistance()
  {
    a(0.0, 0.0, 0.0);
    b(2.0, 2.0, 2.0);
    double d = a.distance(b);
    TS_ASSERT_EQUALS(d, 2.0*sqrt(3.0));
  }

  void testZenith()
  {
    b(0.0, 0.0, 0.0);
    a(9.9, 7.6, 0.0);
    TS_ASSERT_EQUALS( a.zenith(a), 0.0);
    TS_ASSERT_DELTA( a.zenith(b), M_PI/2.0, 0.0001);
    a(-1.1, 0.0, 0.0);
    TS_ASSERT_DELTA( a.zenith(b), M_PI/2.0, 0.0001);
    a(0.0, 0.0, 1.0);
    TS_ASSERT_EQUALS( a.zenith(b), 0.0);
    a(1.0, 0.0, 1.0);
    TS_ASSERT_DELTA( a.zenith(b), M_PI/4.0, 0.0001);
    a(1.0, 0.0, -1.0);
    TS_ASSERT_DELTA( a.zenith(b), 3.0*M_PI/4.0, 0.0001);
  }

  void testAngle()
  {
    a(2.0, 0.0, 0.0);
    b(0.0, 1.0, 0.0);
    c(1.0, 1.0, 0.0);
    d(-1.0, 0.0, 0.0);
    TS_ASSERT_DELTA( a.angle(a), 0.0, 0.0001);
    TS_ASSERT_DELTA( a.angle(b), M_PI/2.0, 0.0001);
    TS_ASSERT_DELTA( a.angle(c), M_PI/4.0, 0.0001);
    TS_ASSERT_DELTA( a.angle(d), M_PI, 0.0001);
  }

  void testSpherical()
  {
    double r = 3, theta = 45.0, phi = 45.0;
    a(0.0, 0.0, 0.0);
    b(0.0, 0.0, 0.0);
    b.spherical(r, theta, phi);
    double d = a.distance(b);
    TS_ASSERT_DELTA(d, r, 0.0001);
    TS_ASSERT_DELTA(b.X(), 1.5, 0.0001);
    TS_ASSERT_DELTA(b.Y(), 1.5, 0.0001);
    TS_ASSERT_DELTA(b.Z(), 3.0/sqrt(2.0), 0.0001);
    // Test getSpherical returns the original values
    TS_ASSERT_THROWS_NOTHING( b.getSpherical(r,theta,phi));
    TS_ASSERT_EQUALS( r, 3.0);
    TS_ASSERT_EQUALS( theta, 45.0);
    TS_ASSERT_EQUALS( phi, 45.0);
  }

  void test_spherical_rad()
  {
    a(0.0, 0.0, 0.0);
    a.spherical_rad(1, 0, 0);
    TS_ASSERT(a == V3D(0, 0, 1));
    a.spherical_rad(1, M_PI / 2, 0);
    TS_ASSERT(a == V3D(1, 0, 0));
    a.spherical_rad(1, M_PI / 2, M_PI / 2);
    TS_ASSERT(a == V3D(0, 1, 0));
    a.spherical_rad(1, M_PI, 0);
    TS_ASSERT(a == V3D(0, 0, -1));
    a.spherical_rad(2, M_PI / 4, 0);
    TS_ASSERT(a == V3D(sqrt(2.0), 0, sqrt(2.0)));
  }

  void test_azimuth_polar_SNS()
  {
    a(0.0, 0.0, 0.0);
    a.azimuth_polar_SNS(1.0, 0, M_PI / 2);
    TS_ASSERT(a == V3D(1.0, 0, 0));
    a.azimuth_polar_SNS(1.0, M_PI / 2, M_PI / 2);
    TS_ASSERT(a == V3D(0.0, 0, 1.0));
    a.azimuth_polar_SNS(2, 0, 0);
    TS_ASSERT(a == V3D(0, 2, 0));
    a.azimuth_polar_SNS(2, 0, M_PI);
    TS_ASSERT(a == V3D(0, -2, 0));
    a.azimuth_polar_SNS(2, 0, M_PI / 4);
    TS_ASSERT(a == V3D(sqrt(2.0), sqrt(2.0), 0));
  }

  /** Round each component to the nearest integer */
  void test_round()
  {
    a(1.2, 0.9, 4.34);
    a.round();
    TS_ASSERT(a == V3D(1.0, 1.0, 4.0));

    a(-1.2, -1.9, -3.9);
    a.round();
    TS_ASSERT(a == V3D(-1.0, -2.0, -4.0));
  }

  void test_toString()
  {
    V3D a(1, 2, 3);
    TS_ASSERT_EQUALS( a.toString(), "1 2 3");
    V3D b;
    b.fromString("4 5 6");
    TS_ASSERT_EQUALS( b, V3D(4,5,6));
  }

  void test_nexus()
  {
    NexusTestHelper th(true);
    th.createFile("V3DTest.nxs");
    V3D a(1, 2, 3);
    a.saveNexus(th.file, "vector");
    th.reopenFile();
    V3D b;
    b.loadNexus(th.file, "vector");
    TS_ASSERT_EQUALS( a, b);
  }

  void test_makeVectorsOrthogonal()
  {
    std::vector<V3D> in, out;
    // Simple case
    in.clear();
    in.push_back(V3D(1, 0, 0));
    in.push_back(V3D(0, 1, 0));
    out = V3D::makeVectorsOrthogonal(in);
    TS_ASSERT( out[0] == V3D(1,0,0));
    TS_ASSERT( out[1] == V3D(0,1,0));
    TS_ASSERT( out[2] == V3D(0,0,1));

    // Non-unit vectors
    in.clear();
    in.push_back(V3D(0.5, 0, 0));
    in.push_back(V3D(0.5, 1.23, 0));
    out = V3D::makeVectorsOrthogonal(in);
    TS_ASSERT( out[0] == V3D(1,0,0));
    TS_ASSERT( out[1] == V3D(0,1,0));
    TS_ASSERT( out[2] == V3D(0,0,1));

    // Flip it over
    in.clear();
    in.push_back(V3D(0.5, 0, 0));
    in.push_back(V3D(0.5, -1.23, 0));
    out = V3D::makeVectorsOrthogonal(in);
    TS_ASSERT( out[0] == V3D(1,0,0));
    TS_ASSERT( out[1] == V3D(0,-1,0));
    TS_ASSERT( out[2] == V3D(0,0,-1));
  }

  void test_to_ostream()
  {
    V3D a(1, 2, 3);
    std::ostringstream ostr;
    ostr << a;
    TS_ASSERT_EQUALS( ostr.str(), "[1,2,3]");
  }

  void test_from_istream()
  {
    V3D a;
    std::istringstream istr("[4,5,6]");
    istr >> a;
    TS_ASSERT_EQUALS( a, V3D(4,5,6));
  }

  void test_toCrystllographic()
  {
    V3D a0;
    TS_ASSERT_THROWS(a0.toMillerIndexes(), std::invalid_argument);

    V3D a1(0.1, 0.2, 5);
    TS_ASSERT_THROWS_NOTHING(a1.toMillerIndexes());

    TS_ASSERT_DELTA(1, a1[0], 1.e-3);
    TS_ASSERT_DELTA(2, a1[1], 1.e-3);
    TS_ASSERT_DELTA(50, a1[2], 1.e-3);

    V3D a2(0.02, 0, 2);
    TS_ASSERT_THROWS_NOTHING(a2.toMillerIndexes());

    TS_ASSERT_DELTA(1, a2[0], 1.e-3);
    TS_ASSERT_DELTA(0, a2[1], 1.e-3);
    TS_ASSERT_DELTA(100, a2[2], 1.e-3);

    V3D a3(0.2, 1.54321, 2);
    TS_ASSERT_THROWS_NOTHING(a3.toMillerIndexes(0.1));

    TS_ASSERT_DELTA(5.18, a3[0], 1.e-1);
    TS_ASSERT_DELTA(40, a3[1], 1.e-1);
    TS_ASSERT_DELTA(51.84, a3[2], 1.e-1);

    V3D a4(-0.6, -0.80321, -3);
    TS_ASSERT_THROWS_NOTHING(a4.toMillerIndexes(0.001));

    TS_ASSERT_DELTA(-1245, a4[0], 1.e-1);
    TS_ASSERT_DELTA(-1666.6, a4[1], 1.e-1);
    TS_ASSERT_DELTA(-6225, a4[2], 1.e-1);

    V3D a5(-3, -0.80321, -0.6);
    TS_ASSERT_THROWS_NOTHING(a5.toMillerIndexes(0.1));

    TS_ASSERT_DELTA(-62.25, a5[0], 1.e-1);
    TS_ASSERT_DELTA(-16.66, a5[1], 1.e-1);
    TS_ASSERT_DELTA(-12.45, a5[2], 1.e-1);

    V3D a6(-3, 5, -6);
    TS_ASSERT_THROWS_NOTHING(a6.toMillerIndexes(0.001));

    TS_ASSERT_DELTA(-3, a6[0], 1.e-3);
    TS_ASSERT_DELTA( 5, a6[1], 1.e-3);
    TS_ASSERT_DELTA(-6, a6[2], 1.e-3);

    V3D a7(-3, 0.5, -6);
    TS_ASSERT_THROWS_NOTHING(a7.toMillerIndexes(0.001));

    TS_ASSERT_DELTA(-6, a7[0], 1.e-3);
    TS_ASSERT_DELTA( 1, a7[1], 1.e-3);
    TS_ASSERT_DELTA(-12, a7[2], 1.e-3);

    V3D a8(-3, 0.3333, -6);
    TS_ASSERT_THROWS_NOTHING(a8.toMillerIndexes(0.1));

    TS_ASSERT_DELTA(-9, a8[0], 1.e-2);
    TS_ASSERT_DELTA( 1, a8[1], 1.e-2);
    TS_ASSERT_DELTA(-18, a8[2], 1.e-2);

    /*    V3D a9(-3,5,-6);
     a9.normalize();
     TS_ASSERT_THROWS_NOTHING(a9.toMillerIndexes(0.1));

     TS_ASSERT_DELTA(-3,a9[0],1.e-3);
     TS_ASSERT_DELTA( 5,a9[1],1.e-3);
     TS_ASSERT_DELTA(-6,a9[2],1.e-3);*/

  }

  void test_directionAngles_cubic_default()
  {
    const V3D orthoNormal(1.0, 1.0, 1.0);
    const V3D angles = orthoNormal.directionAngles();
    const double expectedAngle = acos(1.0 / sqrt(3.0)) * 180 / M_PI;
    TS_ASSERT_DELTA(expectedAngle, angles[0], 1e-6);
    TS_ASSERT_DELTA(expectedAngle, angles[1], 1e-6);
    TS_ASSERT_DELTA(expectedAngle, angles[2], 1e-6);
  }

  void test_directionAngles_cubic_radians()
  {
    const V3D orthoNormal(1.0, 1.0, 1.0);
    const bool inDegrees = false;
    const V3D angles = orthoNormal.directionAngles(inDegrees);
    const double expectedAngle = acos(1.0 / sqrt(3.0));
    TS_ASSERT_DELTA(expectedAngle, angles[0], 1e-6);
    TS_ASSERT_DELTA(expectedAngle, angles[1], 1e-6);
    TS_ASSERT_DELTA(expectedAngle, angles[2], 1e-6);
  }

  void test_directionAngles_orthorombic()
  {
    const V3D v1(1.0, 1.0, 2.0);
    V3D angles = v1.directionAngles();
    const double modv1 = v1.norm();
    TS_ASSERT_DELTA(acos(1.0 / modv1) * 180 / M_PI, angles[0], 1e-6);
    TS_ASSERT_DELTA(acos(1.0 / modv1) * 180 / M_PI, angles[1], 1e-6);
    TS_ASSERT_DELTA(acos(2.0 / modv1) * 180 / M_PI, angles[2], 1e-6);

    const V3D v2(2.0, 3.0, 4.0);
    angles = v2.directionAngles();
    const double modv2 = v2.norm();
    TS_ASSERT_DELTA(acos(2.0 / modv2) * 180 / M_PI, angles[0], 1e-6);
    TS_ASSERT_DELTA(acos(3.0 / modv2) * 180 / M_PI, angles[1], 1e-6);
    TS_ASSERT_DELTA(acos(4.0 / modv2) * 180 / M_PI, angles[2], 1e-6);
  }

};

#endif
