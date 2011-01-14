#ifndef WEIGHTEDEVENTTEST_H_
#define WEIGHTEDEVENTTEST_H_ 1

#include <cxxtest/TestSuite.h>
#include "MantidDataObjects/Events.h"
#include "MantidKernel/Timer.h"
#include <cmath>
#include <boost/math/special_functions/fpclassify.hpp>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

using std::runtime_error;
using std::size_t;
using std::vector;


//==========================================================================================
class WeightedEventTest : public CxxTest::TestSuite
{
private:

public:
  WeightedEventTest()
  {
  }

  void testConstructors()
  {
    TofEvent e(123, 456);
    WeightedEvent we, we2;

    //Empty
    we = WeightedEvent();
    TS_ASSERT_EQUALS(we.tof(), 0);
    TS_ASSERT_EQUALS(we.pulseTime(), 0);
    TS_ASSERT_EQUALS(we.weight(), 1.0);
    TS_ASSERT_EQUALS(we.error(), 1.0);

    //Default one weight
    we = WeightedEvent(e);
    TS_ASSERT_EQUALS(we.tof(), 123);
    TS_ASSERT_EQUALS(we.pulseTime(), 456);
    TS_ASSERT_EQUALS(we.weight(), 1.0);
    TS_ASSERT_EQUALS(we.error(), 1.0);

    //TofEvent + weights
    we = WeightedEvent(e, 3.5, 0.5*0.5);
    TS_ASSERT_EQUALS(we.tof(), 123);
    TS_ASSERT_EQUALS(we.pulseTime(), 456);
    TS_ASSERT_EQUALS(we.weight(), 3.5);
    TS_ASSERT_EQUALS(we.error(), 0.5);

    //Full constructor
    we = WeightedEvent(456, 789, 2.5, 1.5*1.5);
    TS_ASSERT_EQUALS(we.tof(), 456);
    TS_ASSERT_EQUALS(we.pulseTime(), 789);
    TS_ASSERT_EQUALS(we.weight(), 2.5);
    TS_ASSERT_EQUALS(we.error(), 1.5);
  }

  void testAssignAndCopy()
  {
    WeightedEvent we, we2;

    //Copy constructor
    we = WeightedEvent();
    we2 = WeightedEvent(456, 789, 2.5, 1.5*1.5);
    we = we2;
    TS_ASSERT_EQUALS(we.tof(), 456);
    TS_ASSERT_EQUALS(we.pulseTime(), 789);
    TS_ASSERT_EQUALS(we.weight(), 2.5);
    TS_ASSERT_EQUALS(we.error(), 1.5);
  }


};



#endif /// EVENTLISTTEST_H_

