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
    #if defined _WIN64
      Sleep(200);
      TS_ASSERT_LESS_THAN( 0.18, timer.elapsed() );
    #elif defined _WIN32 
      // 32-bit windows doesn't seem to have brilliant resolution
      // as the Sleep(200) can quite often cause the elapsed 
      // time to be less than 0.19 seconds!
      Sleep(200);
      TS_ASSERT_LESS_THAN( 0.18, timer.elapsed() );
    #else
      usleep(200000);
      TS_ASSERT_LESS_THAN( 0.18, timer.elapsed() );
    #endif

    // Calling elapsed above should reset the timer
    #ifdef _WIN32
      Sleep(100);
      TS_ASSERT_LESS_THAN( 0.08, timer.elapsed() );
    #else
      usleep(100000);
      TS_ASSERT_LESS_THAN( 0.08, timer.elapsed() );
    #endif

  }
};

#endif /*TIMERTEST_H_*/
