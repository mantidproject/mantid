#ifndef INTERPOLATIONTEST_H_
#define INTERPOLATIONTEST_H_

#include <cxxtest/TestSuite.h>
#include <ctime>
#include "MantidKernel/Interpolation.h"

using namespace Mantid::Kernel;

class InterpolationTest : public CxxTest::TestSuite
{
public:
    void testCopyConstruction()
    {
        Interpolation interpolation;
        interpolation.setMethod("linear");
        interpolation.setXUnit("Wavelength");
        interpolation.setYUnit("dSpacing");

        interpolation.addPoint(200.0, 2.0);

        Interpolation other = interpolation;

        TS_ASSERT_EQUALS(other.getMethod(), "linear");
        TS_ASSERT_EQUALS(other.getXUnit()->unitID(), "Wavelength");
        TS_ASSERT_EQUALS(other.getYUnit()->unitID(), "dSpacing");
        TS_ASSERT_EQUALS(other.value(200.0), 2.0);
    }

    void testContainData()
    {
        Interpolation interpolation;

        TS_ASSERT ( interpolation.containData() == false );

        interpolation.addPoint(200.0, 50);

        TS_ASSERT ( interpolation.containData() == true );
    }

    void testResetData()
    {
        Interpolation interpolation = getInitializedInterpolation("Wavelength", "dSpacing");

        TS_ASSERT(interpolation.containData());
        interpolation.resetData();
        TS_ASSERT(interpolation.containData() == false);
    }

    void testAddPoint()
    {
        Interpolation interpolation;

        // insertion order does not matter, interpolation table is always sorted internally
        interpolation.addPoint(200.0, 50);
        interpolation.addPoint(201.0, 60);
        interpolation.addPoint(202.0, 100);
        interpolation.addPoint(204.0, 400);
        interpolation.addPoint(203.0, 300);

        // Test that all the base class member variables are correctly assigned to
        TS_ASSERT_DELTA( interpolation.value(100), -950.0 ,0.000000001);
        TS_ASSERT_DELTA( interpolation.value(3000), 280000.0 ,0.000000001);
        TS_ASSERT_DELTA( interpolation.value(200.5), 55.0 ,0.000000001);
        TS_ASSERT_DELTA( interpolation.value(201.25), 70.0 ,0.000000001);
        TS_ASSERT_DELTA( interpolation.value(203.5), 350.0 ,0.000000001);

        TS_ASSERT_EQUALS(interpolation.value(204.0), 400.0);
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

    void testStreamOperators()
    {
        std::string xUnit = "Wavelength";
        std::string yUnit = "dSpacing";

        Interpolation interpolation = getInitializedInterpolation(xUnit, yUnit);

        // Output stream
        std::stringstream str;
        str << interpolation;
        TS_ASSERT( str.str().compare("linear ; Wavelength ; dSpacing ; 200 50 ; 201 60 ; 202 100 ; 203 300 ; 204 400") == 0 );

        // Input stream for empty interpolation object
        Interpolation readIn;
        TS_ASSERT( readIn.getXUnit()->unitID() == "TOF" );
        TS_ASSERT( readIn.getYUnit()->unitID() == "TOF" );
        str >> readIn;
        TS_ASSERT( readIn.getXUnit()->unitID() == xUnit );
        TS_ASSERT( readIn.getYUnit()->unitID() == yUnit );

        // Test that all the base class member variables are correctly assigned to
        TS_ASSERT_DELTA( readIn.value(100), -950.0 ,0.000000001);
        TS_ASSERT_DELTA( readIn.value(3000), 280000.0 ,0.000000001);
        TS_ASSERT_DELTA( readIn.value(200.5), 55.0 ,0.000000001);
        TS_ASSERT_DELTA( readIn.value(201.25), 70.0 ,0.000000001);
        TS_ASSERT_DELTA( readIn.value(203.5), 350.0 ,0.000000001);
    }

    void testStreamOperatorsNonEmpty()
    {
        Interpolation interpolation = getInitializedInterpolation("Wavelength", "dSpacing");

        std::stringstream str;
        str << interpolation;

        // Reconstruct on existing object.
        str >> interpolation;

        TS_ASSERT_DELTA( interpolation.value(100), -950.0 ,0.000000001);
        TS_ASSERT_DELTA( interpolation.value(3000), 280000.0 ,0.000000001);
        TS_ASSERT_DELTA( interpolation.value(200.5), 55.0 ,0.000000001);
        TS_ASSERT_DELTA( interpolation.value(201.25), 70.0 ,0.000000001);
        TS_ASSERT_DELTA( interpolation.value(203.5), 350.0 ,0.000000001);

        TS_ASSERT_EQUALS( interpolation.value(201.0), 60.0)
    }

private:
    Interpolation getInitializedInterpolation(std::string xUnit, std::string yUnit)
    {
        Interpolation interpolation;

        interpolation.addPoint(200.0, 50);
        interpolation.addPoint(201.0, 60);
        interpolation.addPoint(202.0, 100);
        interpolation.addPoint(203.0, 300);
        interpolation.addPoint(204.0, 400);

        interpolation.setXUnit(xUnit);
        interpolation.setYUnit(yUnit);

        return interpolation;
    }
  
};

#endif /*INTERPOLATIONTEST_H_*/
