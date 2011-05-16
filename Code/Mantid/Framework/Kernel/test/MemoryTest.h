#ifndef MANTID_KERNEL_MEMORYTEST_H_
#define MANTID_KERNEL_MEMORYTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidKernel/Memory.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/FunctionTask.h"
#include "MantidKernel/ThreadPool.h"

using namespace Mantid::Kernel;

void MemoryTest_myTaskFunction()
{
  MemoryStats mem;
  mem.update();
  mem.getFreeRatio();

}

class MemoryTest : public CxxTest::TestSuite
{
public:

  void test_update()
  {
    MemoryStats mem;
    TS_ASSERT_THROWS_NOTHING( mem.update() );
    TS_ASSERT_LESS_THAN_EQUALS(0, mem.availMem() );
    TS_ASSERT_DIFFERS( mem.availMemStr(), "" );
    TS_ASSERT_LESS_THAN_EQUALS(0, mem.reservedMem());
    TS_ASSERT_LESS_THAN_EQUALS(0, mem.totalMem());
    TS_ASSERT_DIFFERS( mem.totalMemStr(), "" );
  }


  /// Update in parallel to test thread safety
  void test_parallel()
  {
    PARALLEL_FOR_IF(true)
    for(int i=0; i<500; i++)
    {
      MemoryStats mem;
      mem.update();
      mem.getFreeRatio();
    }
  }


  void test_parallel_threadpool()
  {
    ThreadPool pool;
    for(int i=0; i<500; i++)
    {
      pool.schedule(new FunctionTask(&MemoryTest_myTaskFunction, 1.0) );
    }
    pool.joinAll();
  }

};


#endif /* MANTID_KERNEL_MEMORYTEST_H_ */

