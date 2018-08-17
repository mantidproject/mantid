#ifndef MANTID_PARALLEL_EVENTSLISTSSHMEMMANAGERTEST_H_
#define MANTID_PARALLEL_EVENTSLISTSSHMEMMANAGERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidParallel/IO/EventsListsShmemManager.h"

using Mantid::Parallel::EventsListsShmemManager;

class EventsListsShmemManagerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EventsListsShmemManagerTest *createSuite() { return new EventsListsShmemManagerTest(); }
  static void destroySuite(EventsListsShmemManagerTest *suite) { delete suite; }

  void test_Something() {
    TS_FAIL("You forgot to write a test!");
  }

};

#endif /* MANTID_PARALLEL_EVENTSLISTSSHMEMMANAGERTEST_H_ */