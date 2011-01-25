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
      Sleep(40);
    #else
      usleep(40000);
    #endif
    TS_ASSERT_DELTA( timer.elapsed(), 0.04, 0.002 );
    #ifdef _WIN32
      Sleep(20);
    #else
      usleep(20000);
    #endif
    // Calling elapsed above should reset the timer
    TS_ASSERT_DELTA( timer.elapsed(), 0.02, 0.002 );
  }
};

#endif /*TIMERTEST_H_*/
