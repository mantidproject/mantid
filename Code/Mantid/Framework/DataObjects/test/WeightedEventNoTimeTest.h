#ifndef WEIGHTEDEVENTNOTIMETEST_H_
#define WEIGHTEDEVENTNOTIMETEST_H_ 1

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
class WeightedEventNoTimeTest : public CxxTest::TestSuite
{
private:

public:
  WeightedEventNoTimeTest()
  {
  }

  void testConstructors()
  {
    TofEvent e(123, 456);
    WeightedEvent we;
    WeightedEventNoTime wen, wen2;

    //Empty
    wen = WeightedEventNoTime();
    TS_ASSERT_EQUALS(wen.tof(), 0);
    TS_ASSERT_EQUALS(wen.pulseTime(), 0);
    TS_ASSERT_EQUALS(wen.weight(), 1.0);
    TS_ASSERT_EQUALS(wen.error(), 1.0);

    // From WeightedEvent
    we = WeightedEvent(456, 789, 2.5, 1.5*1.5);
    wen = WeightedEventNoTime(we);
    TS_ASSERT_EQUALS(wen.tof(), 456);
    TS_ASSERT_EQUALS(wen.pulseTime(), 0); // Lost the time!
    TS_ASSERT_EQUALS(wen.weight(), 2.5);
    TS_ASSERT_EQUALS(wen.error(), 1.5);

    //Default one weight from TofEvent
    wen = WeightedEventNoTime(e);
    TS_ASSERT_EQUALS(wen.tof(), 123);
    TS_ASSERT_EQUALS(wen.pulseTime(), 0);
    TS_ASSERT_EQUALS(wen.weight(), 1.0);
    TS_ASSERT_EQUALS(wen.error(), 1.0);

    //TofEvent + weights
    wen = WeightedEventNoTime(e, 3.5, 0.5*0.5);
    TS_ASSERT_EQUALS(wen.tof(), 123);
    TS_ASSERT_EQUALS(wen.pulseTime(), 0);
    TS_ASSERT_EQUALS(wen.weight(), 3.5);
    TS_ASSERT_EQUALS(wen.error(), 0.5);

    //Full constructor
    wen = WeightedEventNoTime(456, 2.5, 1.5*1.5);
    TS_ASSERT_EQUALS(wen.tof(), 456);
    TS_ASSERT_EQUALS(wen.pulseTime(), 0); // Never had time
    TS_ASSERT_EQUALS(wen.weight(), 2.5);
    TS_ASSERT_EQUALS(wen.error(), 1.5);
  }

  void testAssignAndCopy_AndEquality()
  {
    WeightedEventNoTime wen, wen2;

    //Copy constructor
    wen = WeightedEventNoTime();
    wen2 = WeightedEventNoTime(456, 2.5, 1.5*1.5);
    TS_ASSERT(!(wen == wen2));

    wen = wen2;
    TS_ASSERT_EQUALS(wen.tof(), 456);
    TS_ASSERT_EQUALS(wen.pulseTime(), 0); // Never had time
    TS_ASSERT_EQUALS(wen.weight(), 2.5);
    TS_ASSERT_EQUALS(wen.error(), 1.5);

    TS_ASSERT(wen == wen2);
  }




};



#endif /// EVENTLISTTEST_H_

