#ifndef TIME_TO_TIMESTEP_TEST_H_
#define TIME_TO_TIMESTEP_TEST_H_

#include <cxxtest/TestSuite.h>
#include <vtkRectilinearGrid.h>
#include "MantidVatesAPI/TimeToTimeStep.h"

class TimeToTimeStepTest: public CxxTest::TestSuite
{
public:

  void testPeformsRescaling()
  {
    using namespace Mantid::VATES;
    //Test that this type can perform rescaling from time to an index.
    double tMin = 0;
    double tMax = 200;
    double t = 51; // just over 25% of the range, but after truncation, exactly 25%.

    int nBins = 100;
    TimeToTimeStep converter = TimeToTimeStep::construct(tMin, tMax, nBins);
    TSM_ASSERT_EQUALS("The timeStep index has not been calculated properly. Wrong result.", 25, converter(t))
    TSM_ASSERT_EQUALS("The timeStep index has not been calculated properly. Wrong result.", 0, converter(tMin))
    TSM_ASSERT_EQUALS("The timeStep index has not been calculated properly. Wrong result.", nBins, converter(tMax))
  }

  void testHandle_t_OutOfRange()
  {
    //Assume that t MUST sit somewhere between tmin and tmax. Zero out otherwise.
    using namespace Mantid::VATES;
    //Test that this type can perform rescaling from time to an index.
    double tMin = 0;
    double tMax = 100;
    int nBins = 200;
    TimeToTimeStep converter = TimeToTimeStep::construct(tMin, tMax, nBins);
    TSM_ASSERT_EQUALS("T is greater is inside range. Should interpolate.", 60, converter(30));
    TSM_ASSERT_EQUALS("T is greater than max. Should have zero'd out.", 0, converter(101));
    TSM_ASSERT_EQUALS("T is less than max. Should have zero'd out.", 0, converter(-1));
  }

  void testBadTimeRangeThrows()
  {
    using namespace Mantid::VATES;
    //Test that this type can perform rescaling from time to an index.
    double tMin = 0;
    double tMax = -200;
    int nBins = 100;

    TSM_ASSERT_THROWS("Range is negative, should throw.", TimeToTimeStep::construct(tMin, tMax, nBins), std::runtime_error);
  }

  void testUseWithDefaultConstructorThrows()
  {
   Mantid::VATES::TimeToTimeStep converter;
   TSM_ASSERT_THROWS("Constructing via default constructor should throw when used.", converter(1), std::runtime_error);
  }

};

#endif
