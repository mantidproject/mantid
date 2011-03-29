#ifndef MANTID_KERNEL_UTILSTEST_H_
#define MANTID_KERNEL_UTILSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidKernel/Utils.h"

using namespace Mantid::Kernel;

class UtilsTest : public CxxTest::TestSuite
{
public:


  /** Make a nested loop with each counter resetting at 0 */
  void test_nestedForLoopIncrement()
  {
    size_t * counters = Utils::nestedForLoopSetUp(3);
    size_t * counters_max = Utils::nestedForLoopSetUp(3, 10);

    // The data
    size_t data[10][10][10];
    memset(data, 0, sizeof(data));

    bool allDone = false;
    while (!allDone)
    {
      data[counters[0]][counters[1]][counters[2]] = counters[0] * 10000 + counters[1] * 100 + counters[2];
      allDone = Utils::nestedForLoopIncrement(3, counters, counters_max);
    }

    for (size_t x=0; x<10; x++)
      for (size_t y=0; y<10; y++)
        for (size_t z=0; z<10; z++)
        {
          TS_ASSERT_EQUALS( data[x][y][z], x*10000+y*100+z);
        }

    delete counters;
    delete counters_max;
  }


  /** Make a nested loop but use a non-zero starting index for each counter */
  void test_nestedForLoopIncrement_nonZeroMinimum()
  {
    size_t * counters = Utils::nestedForLoopSetUp(3, 4);
    size_t * counters_min = Utils::nestedForLoopSetUp(3, 4);
    size_t * counters_max = Utils::nestedForLoopSetUp(3, 8);

    // The data
    size_t data[10][10][10];
    memset(data, 0, sizeof(data));

    bool allDone = false;
    while (!allDone)
    {
      data[counters[0]][counters[1]][counters[2]] = counters[0] * 10000 + counters[1] * 100 + counters[2];
      allDone = Utils::nestedForLoopIncrement(3, counters, counters_max, counters_min);
    }

    for (size_t x=0; x<10; x++)
      for (size_t y=0; y<10; y++)
        for (size_t z=0; z<10; z++)
        {
          if ((x<4 || y<4 || z<4) || (x>=8 || y>=8 || z>=8))
            { TS_ASSERT_EQUALS( data[x][y][z], 0); }
          else
            { TS_ASSERT_EQUALS( data[x][y][z], x*10000+y*100+z);}
        }

    delete counters;
    delete counters_min;
    delete counters_max;
  }


};


#endif /* MANTID_KERNEL_UTILSTEST_H_ */

