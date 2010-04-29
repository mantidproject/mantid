#ifndef TIMERTEST_H_
#define TIMERTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#ifdef _WIN32
#include <Windows.h>
#endif

class TimerTest : public CxxTest::TestSuite
{
public:
  void testTimer()
  {
    // Instantiating the object starts the timer
    Mantid::Kernel::Timer timer;
    #ifdef _WIN32
      Sleep(2000);
    #else
      sleep(2);
    #endif
    TS_ASSERT_DELTA( timer.elapsed(), 2.00, 0.01 );
    #ifdef _WIN32
      Sleep(1000);
    #else
      sleep(1);
    #endif
    // Calling elapsed above should reset the timer
    TS_ASSERT_DELTA( timer.elapsed(), 1.00, 0.01 );
  }
};

#endif /*TIMERTEST_H_*/
