#ifndef MANTID_DATAOBJECTS_EVENTWORKSPACEMRUTEST_H_
#define MANTID_DATAOBJECTS_EVENTWORKSPACEMRUTEST_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/EventWorkspaceMRU.h"

using namespace Mantid::DataObjects;

class EventWorkspaceMRUTest : public CxxTest::TestSuite {
public:
  void test_emptyList() {
    EventWorkspaceMRU mru;
    TS_ASSERT_THROWS_NOTHING(mru.MRUSize());
    TS_ASSERT_EQUALS(mru.MRUSize(), 0);
  }
};

#endif /* MANTID_DATAOBJECTS_EVENTWORKSPACEMRUTEST_H_ */
