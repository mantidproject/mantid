#ifndef EVENTLISTTEST_H_
#define EVENTLISTTEST_H_ 1

#include <cxxtest/TestSuite.h>
#include "MantidDataObjects/EventList.h"

using namespace Mantid::DataObjects;

class TofEventTest : public CxxTest::TestSuite
{
private:
  TofEvent e;

public:
  TofEventTest()
  {
    //e = TofEvent(123, 456);
  }

  void xtestInit()
  {
    TS_ASSERT_EQUALS(e.tof(), 123);
  }
};

#endif /// EVENTLISTTEST_H_

