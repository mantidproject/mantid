#ifndef MANTID_KERNEL_VMDTEST_H_
#define MANTID_KERNEL_VMDTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidKernel/VMD.h"

using namespace Mantid;
using namespace Mantid::Kernel;

class VMDTest : public CxxTest::TestSuite
{
public:

  void test_constructors()
  {
    TS_ASSERT_EQUALS( VMD().getNumDims(), 1 );
    TS_ASSERT_EQUALS( VMD(27).getNumDims(), 27 );
    TS_ASSERT_EQUALS( VMD(2), VMD(0.0, 0.0) );
    TS_ASSERT_EQUALS( VMD(3), VMD(0.0, 0.0, 0.0) );
    TS_ASSERT_EQUALS( VMD(4), VMD(0.0, 0.0, 0.0, 0.0) );
    TS_ASSERT_EQUALS( VMD( V3D(1,2,3) ), VMD(1,2,3) );
    double v1[4] = {1,2,3,4};
    TS_ASSERT_EQUALS( VMD(4, v1), VMD(1,2,3,4) );
    std::vector<double> v2; v2.push_back(1); v2.push_back(2);
    TS_ASSERT_EQUALS( VMD(v2), VMD(1,2) );
    std::vector<float> v3; v3.push_back(1); v3.push_back(2);
    TS_ASSERT_EQUALS( VMD(v3), VMD(1,2) );
    // Copy constructor
    VMD a(1,2,3,4);
    TS_ASSERT_EQUALS( VMD(a), VMD(1,2,3,4) );
  }

  /// Constructors that should throw
  void test_constructors_throw()
  {
    double v1[4] = {1,2,3,4};
    TS_ASSERT_THROWS_ANYTHING( VMD(0) );
    TS_ASSERT_THROWS_ANYTHING( VMD(0, v1) );
  }

  void test_notEquals()
  {
    TS_ASSERT( VMD(1,2,3) != VMD(1,2,3.0001) );
  }

  void test_assign()
  {
    VMD a(1,2,3);
    VMD b(1,2);
    b = a;
    TS_ASSERT( a == b);
  }

  void test_brackets()
  {
    VMD a(1,2,3,4);
    TS_ASSERT_DELTA( a[0], 1.0, 1e-5);
    TS_ASSERT_DELTA( a[1], 2.0, 1e-5);
    TS_ASSERT_DELTA( a[2], 3.0, 1e-5);
    TS_ASSERT_DELTA( a[3], 4.0, 1e-5);
  }

  void test_operators_throw_ifNonMatchingDimensions()
  {
    VMD a(1,2,3);
    VMD b(1,2);
    TS_ASSERT_THROWS_ANYTHING( a+b );
    TS_ASSERT_THROWS_ANYTHING( a-b );
    TS_ASSERT_THROWS_ANYTHING( a*b );
    TS_ASSERT_THROWS_ANYTHING( a/b );
    TS_ASSERT_THROWS_ANYTHING( a += b );
    TS_ASSERT_THROWS_ANYTHING( a -= b );
    TS_ASSERT_THROWS_ANYTHING( a *= b );
    TS_ASSERT_THROWS_ANYTHING( a /= b );
  }

  void test_plus()
  {
    VMD a(1,2,3);
    VMD b(2,3,4);
    VMD c(3,5,7);
    TS_ASSERT_EQUALS( (a+b), c);
    b += a;
    TS_ASSERT_EQUALS( b, c);
  }

  void test_minus()
  {
    VMD a(1,2,3);
    VMD b(2,3,4);
    VMD c(-1,-1,-1);
    TS_ASSERT_EQUALS( (a-b), c);
    a -= b;
    TS_ASSERT_EQUALS( a, c);
  }

  void test_mult()
  {
    VMD a(1,2,3);
    VMD b(2,3,4);
    VMD c(2,6,12);
    TS_ASSERT_EQUALS( (a*b), c);
    b *= a;
    TS_ASSERT_EQUALS( b, c);
  }

  void test_div()
  {
    VMD a(1,2,3);
    VMD b(2,3,4);
    VMD c(0.5,2.0/3.0, 0.75);
    TS_ASSERT_EQUALS( (a/b), c);
    a /= b;
    TS_ASSERT_EQUALS( a, c);
  }

  void test_mult_scalar()
  {
    VMD a(1,2,3);
    VMD b(2,4,6);
    TS_ASSERT_EQUALS( (a*2.0), b);
    a *= 2.0;
    TS_ASSERT_EQUALS( a, b);
  }

  void test_div_scalar()
  {
    VMD a(1,2,3);
    VMD b(0.5,1,1.5);
    TS_ASSERT_EQUALS( (a/2.0), b);
    a /= 2.0;
    TS_ASSERT_EQUALS( a, b);
  }

  void test_scalar_prod()
  {
    VMD a(1,2,3);
    VMD b(2,3,4);
    TS_ASSERT_EQUALS( a.scalar_prod(b), 2+6+12);
  }

  void test_length()
  {
    VMD a(3,4,sqrt(39.0));
    TS_ASSERT_EQUALS( a.length(), 8.0);
  }

  void test_normalize()
  {
    VMD a(3,4,sqrt(39.0));
    VMD b(3./8,4./8,sqrt(39.0)/8.);
    TS_ASSERT_EQUALS( a.normalize(), 8.0);
    TS_ASSERT_EQUALS( a, b);
  }

};


#endif /* MANTID_KERNEL_VMDTEST_H_ */

