#ifndef INTERPOLATIONTEST_H_
#define INTERPOLATIONTEST_H_

#include <cxxtest/TestSuite.h>
#include <ctime>
#include "MantidKernel/Interpolation.h"

using namespace Mantid::Kernel;

class InterpolationTest : public CxxTest::TestSuite
{
public:
  
	void test1()
	{
    Interpolation interpolation("linear");

    interpolation.addPoint(200.0, 50);
    interpolation.addPoint(201.0, 60);
    interpolation.addPoint(202.0, 100);

    // Test that all the base class member variables are correctly assigned to
    TS_ASSERT_DELTA( interpolation.value(100), 50.0 ,0.000000001); 
    TS_ASSERT_DELTA( interpolation.value(300), 100.0 ,0.000000001);
    TS_ASSERT_DELTA( interpolation.value(200.5), 55.0 ,0.000000001); 
    TS_ASSERT_DELTA( interpolation.value(201.25), 70.0 ,0.000000001); 
	}
  
};

#endif /*INTERPOLATIONTEST_H_*/
