#ifndef EVENTSTEST_H_
#define EVENTSTEST_H_ 1

#include <cxxtest/TestSuite.h>
#include "MantidDataObjects/Events.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;

using std::runtime_error;
using std::size_t;
using std::vector;

class EventsTest : public CxxTest::TestSuite
{

public:
  void setUp()
  {
  }

  void test_Compare()
  {
    // tof event
    TofEvent tofEvent1(20.0, Kernel::DateAndTime("1990-01-02 00:00:02.000"));
    TofEvent tofEvent2(20.1, Kernel::DateAndTime("1990-01-02 00:00:02.000000001"));

    TS_ASSERT( tofEvent1 == tofEvent1 );
    TS_ASSERT( !(tofEvent1 == tofEvent2) );
    TS_ASSERT( tofEvent1.equals(tofEvent2, .1, 1) );

    // weighted
    WeightedEvent wghtEvent1(20.0, Kernel::DateAndTime("1990-01-02 00:00:02.000"), 1., 1.);
    WeightedEvent wghtEvent2(20.1, Kernel::DateAndTime("1990-01-02 00:00:02.000000001"), 1.1, 1.);

    TS_ASSERT( wghtEvent1 == wghtEvent1 );
    TS_ASSERT( !(wghtEvent1 == wghtEvent2) );
    TS_ASSERT( wghtEvent1.equals(wghtEvent2, .1, .1, 1) );

    // weighted no time
    WeightedEventNoTime notimeEvent1(20.0, 1., 1. );
    WeightedEventNoTime notimeEvent2(20.1, 1.1, 1.1 );

    TS_ASSERT( notimeEvent1 == notimeEvent1 );
    TS_ASSERT( !(notimeEvent1 == notimeEvent2) );
    TS_ASSERT( notimeEvent1.equals(notimeEvent2, .1, .1) );
  }
};

#endif /// EVENTSTEST_H_
