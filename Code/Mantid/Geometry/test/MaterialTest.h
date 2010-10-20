#ifndef MANTID_TESTMATERIAL__
#define MANTID_TESTMATERIAL__

#include <cxxtest/TestSuite.h>
#include <cmath>

#include "MantidGeometry/Objects/Material.h"


using namespace Mantid;
using namespace Geometry;

class testMaterial: public CxxTest::TestSuite
{
public:
	void testConstructor()
  {
    const double testDensity(1e-03), testTemp(300.), testPressure(1e+5), 
      testCohXsec(20.), testIncohXsec(2.0), testAbsXsec(1.5); 
		Material fake("fake", testDensity, testTemp, testPressure, testCohXsec, testIncohXsec, testAbsXsec); 
		TS_ASSERT_EQUALS(fake.name(), "fake");
    TS_ASSERT_EQUALS(fake.density(), testDensity);
    TS_ASSERT_EQUALS(fake.temperature(), testTemp);
    TS_ASSERT_EQUALS(fake.pressure(), testPressure);
    TS_ASSERT_EQUALS(fake.coherentCrossSection(), testCohXsec);
    TS_ASSERT_EQUALS(fake.incoherentCrossSection(), testIncohXsec);
    TS_ASSERT_EQUALS(fake.absorptionCrossSection(), testAbsXsec);
    TS_ASSERT_DELTA(fake.totalCrossSection(), testCohXsec + testIncohXsec, 1e-08);
	}

};
#endif
