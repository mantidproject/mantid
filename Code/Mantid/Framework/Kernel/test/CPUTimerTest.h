#ifndef MANTID_KERNEL_CPUTIMERTEST_H_
#define MANTID_KERNEL_CPUTIMERTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidKernel/CPUTimer.h"

using namespace Mantid::Kernel;

class CPUTimerTest : public CxxTest::TestSuite
{
public:

  /** Since timer tests are difficult to make reliable,
   * simple tests for not throwing only.
   */
  void test_throws_nothing()
  {
    TS_ASSERT_THROWS_NOTHING( CPUTimer timer1; )
    CPUTimer tim1;
    TS_ASSERT_THROWS_NOTHING( tim1.reset(); )
    TS_ASSERT_THROWS_NOTHING( tim1.elapsed(); )
    TS_ASSERT_THROWS_NOTHING( tim1.elapsed(true); )
    TS_ASSERT_THROWS_NOTHING( tim1.elapsed(false); )
    TS_ASSERT_THROWS_NOTHING( tim1.CPUfraction(); )
    TS_ASSERT_THROWS_NOTHING( tim1.CPUfraction(true); )
    TS_ASSERT_THROWS_NOTHING( tim1.CPUfraction(false); )
  }


};


#endif /* MANTID_KERNEL_CPUTIMERTEST_H_ */

