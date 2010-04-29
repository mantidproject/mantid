#ifndef TIMERTEST_H_
#define TIMERTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"

class TimerTest : public CxxTest::TestSuite
{
public:
  void testTimer()
  {
    // Instantiating the object starts the timer
    Mantid::Kernel::Timer timer;
    sleep(2);
    TS_ASSERT_DELTA( timer.elapsed(), 2.00, 0.01 );
    sleep(1);
    // Calling elapsed above should reset the timer
    TS_ASSERT_DELTA( timer.elapsed(), 1.00, 0.01 );
  }
};

#endif /*TIMERTEST_H_*/
