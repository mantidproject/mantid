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
    Interpolation interpolation;

    TS_ASSERT ( interpolation.containData() == false );

    interpolation.addPoint(200.0, 50);

    TS_ASSERT ( interpolation.containData() == true );

    interpolation.addPoint(201.0, 60);
    interpolation.addPoint(202.0, 100);
    interpolation.addPoint(204.0, 400);
    interpolation.addPoint(203.0, 300);

    // Test that all the base class member variables are correctly assigned to
    TS_ASSERT_DELTA( interpolation.value(100), -950.0 ,0.000000001); 
    TS_ASSERT_DELTA( interpolation.value(3000), 260400.0 ,0.000000001);
    TS_ASSERT_DELTA( interpolation.value(200.5), 55.0 ,0.000000001); 
    TS_ASSERT_DELTA( interpolation.value(201.25), 70.0 ,0.000000001); 
    TS_ASSERT_DELTA( interpolation.value(203.5), 350.0 ,0.000000001); 


    interpolation.setXUnit("Wavelength");
    interpolation.setYUnit("dSpacing");
    std::stringstream str;
    str << interpolation;
    TS_ASSERT( str.str().compare("linear ; Wavelength ; dSpacing ; 200 50 ; 201 60 ; 202 100 ; 203 300 ; 204 400") == 0 );

    Interpolation readIn;
    TS_ASSERT( readIn.getXUnit()->unitID() == "TOF" );
    TS_ASSERT( readIn.getYUnit()->unitID() == "TOF" );
    str >> readIn;
    TS_ASSERT( readIn.getXUnit()->unitID() == "Wavelength" );
    TS_ASSERT( readIn.getYUnit()->unitID() == "dSpacing" );

    // Test that all the base class member variables are correctly assigned to
    TS_ASSERT_DELTA( readIn.value(100), -950.0 ,0.000000001); 
    TS_ASSERT_DELTA( readIn.value(3000), 260400.0 ,0.000000001);
    TS_ASSERT_DELTA( readIn.value(200.5), 55.0 ,0.000000001); 
    TS_ASSERT_DELTA( readIn.value(201.25), 70.0 ,0.000000001); 
    TS_ASSERT_DELTA( readIn.value(203.5), 350.0 ,0.000000001); 
	}

	void testEmpty()
	{
    Interpolation interpolation;

    std::stringstream str;
    str << interpolation;
    TS_ASSERT( str.str().compare("linear ; TOF ; TOF") == 0 );

    Interpolation readIn;
    str >> readIn;

    TS_ASSERT( readIn.containData() == false );
	}
  
};

#endif /*INTERPOLATIONTEST_H_*/
