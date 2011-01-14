#ifndef TOFEVENTTEST_H_
#define TOFEVENTTEST_H_ 1

#include <cxxtest/TestSuite.h>
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
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
class EventsTest : public CxxTest::TestSuite
{
private:
  TofEvent e;

public:
  EventsTest()
  {
    e = TofEvent(123, 456);
  }

  void testInit()
  {
    TS_ASSERT_EQUALS(e.tof(), 123);
    TS_ASSERT_EQUALS(e.pulseTime(), 456);
  }

  void testAssign()
  {
    TofEvent e2;
    e2 = e;
    TS_ASSERT_EQUALS(e2.tof(), 123);
    TS_ASSERT_EQUALS(e2.pulseTime(), 456);
  }

  void testConstructors()
  {
    TofEvent e2 = TofEvent(e);
    TS_ASSERT_EQUALS(e2.tof(), 123);
    TS_ASSERT_EQUALS(e2.pulseTime(), 456);

    TofEvent e3 = TofEvent(890.234, 321);
    TS_ASSERT_EQUALS(e3.tof(), 890.234);
    TS_ASSERT_EQUALS(e3.pulseTime(), 321);
  }

};


#endif

