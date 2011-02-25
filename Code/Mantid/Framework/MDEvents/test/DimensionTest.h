#ifndef MANTID_MDEVENTS_DIMENSIONTEST_H_
#define MANTID_MDEVENTS_DIMENSIONTEST_H_

#include <cxxtest/TestSuite.h>
#include <MantidKernel/Timer.h>
#include <MantidKernel/System.h>
#include <iostream>
#include <iomanip>

#include <MantidMDEvents/Dimension.h>

using namespace Mantid::MDEvents;

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


#endif /* MANTID_MDEVENTS_DIMENSIONTEST_H_ */

