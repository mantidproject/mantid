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
      Sleep(200);
      TS_ASSERT_DELTA( timer.elapsed(), 0.2, 0.02 );
    #else
      usleep(200000);
      TS_ASSERT_LESS_THAN( 0.2, timer.elapsed() );
    #endif

    // Calling elapsed above should reset the timer
    #ifdef _WIN32
      Sleep(100);
      TS_ASSERT_DELTA( timer.elapsed(), 0.1, 0.02 );
    #else
      usleep(100000);
      TS_ASSERT_LESS_THAN( 0.1, timer.elapsed() );
    #endif

  }
};

#endif /*TIMERTEST_H_*/
