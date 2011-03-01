#ifndef MANTID_API_DIMENSIONTEST_H_
#define MANTID_API_DIMENSIONTEST_H_

#include <cxxtest/TestSuite.h>
#include <MantidKernel/Timer.h>
#include <MantidKernel/System.h>
#include <iostream>
#include <iomanip>

#include "MantidAPI/Dimension.h"

using namespace Mantid::API;

class DimensionTest : public CxxTest::TestSuite
{
public:

  void test_Constructor()
  {
    Dimension d(-10, +5.78, "TestX", "Furlongs");
    TS_ASSERT_DELTA( d.getMin(), -10.0, 1e-6);
    TS_ASSERT_DELTA( d.getMax(), +5.78, 1e-6);
    TS_ASSERT_EQUALS( d.getName(), "TestX");
    TS_ASSERT_EQUALS( d.getUnits(), "Furlongs");
  }


};


#endif /* MANTID_API_DIMENSIONTEST_H_ */

