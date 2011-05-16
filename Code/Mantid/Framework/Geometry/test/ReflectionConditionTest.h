#ifndef MANTID_GEOMETRY_REFLECTIONCONDITIONTEST_H_
#define MANTID_GEOMETRY_REFLECTIONCONDITIONTEST_H_

#include "MantidGeometry/Crystal/ReflectionCondition.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid::Geometry;

class ReflectionConditionTest : public CxxTest::TestSuite
{
public:

  void checkRC(ReflectionCondition & rc, int * h, int * k, int * l, int * valid, size_t count)
  {
    for (size_t i=0; i<count; i++)
    {
      bool v = rc.isAllowed(h[i], k[i], l[i]);
      TS_ASSERT_EQUALS( (valid[i]==1), v);
    }
  }

  void test_ReflectionConditionCFaceCentred()
  {
    ReflectionConditionCFaceCentred rc;
    int h[9] = {0,0,0,1,1,1,2,2,2};
    int k[9] = {0,1,2,0,1,2,0,1,2};
    int l[9] = {0,1,3,4,5,6,7,8,9};
    int v[9] = {1,0,1,0,1,0,1,0,1};
    checkRC(rc, h,k,l, v, 9);
  }

  void test_ReflectionConditionAllFaceCentred()
  {
    ReflectionConditionAllFaceCentred rc;
    int h[5] = {0,1,0,1,1};
    int k[5] = {0,1,0,3,2};
    int l[5] = {0,1,1,1,3};
    int v[5] = {1,1,0,1,0};
    checkRC(rc, h,k,l, v, 5);
  }



};


#endif /* MANTID_GEOMETRY_REFLECTIONCONDITIONTEST_H_ */

