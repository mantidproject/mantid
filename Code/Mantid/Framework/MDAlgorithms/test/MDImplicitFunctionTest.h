#ifndef MANTID_MDALGORITHMS_MDIMPLICITFUNCTIONTEST_H_
#define MANTID_MDALGORITHMS_MDIMPLICITFUNCTIONTEST_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidMDAlgorithms/MDImplicitFunction.h"
#include "MantidMDAlgorithms/MDPlane.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;
using namespace Mantid;

class MDImplicitFunctionTest : public CxxTest::TestSuite
{
public:

  void test_addPlane()
  {
    MDImplicitFunction f;

    coord_t normal[3] = {1.234, 4.56, 6.78};
    coord_t point[3] = {1,2,3};
    MDPlane p1(3, normal, point);
    MDPlane p2(2, normal, point);
    MDPlane p3(3, normal, point);

    TS_ASSERT_THROWS_NOTHING(f.addPlane(p1) );
    TS_ASSERT_THROWS_ANYTHING( f.addPlane(p2) );
    TS_ASSERT_THROWS_NOTHING(f.addPlane(p3) );
  }


};


#endif /* MANTID_MDALGORITHMS_MDIMPLICITFUNCTIONTEST_H_ */

