#ifndef MEMORYMANAGERTEST_H_
#define MEMORYMANAGERTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/MemoryManager.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/FunctionTask.h"
#include "MantidKernel/ThreadPool.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

class MemoryManagerTest : public CxxTest::TestSuite
{
public:
  void testResult()
  {
    MemoryInfo minfo = MemoryManager::Instance().getMemoryInfo();
    TS_ASSERT_LESS_THAN( 1000, minfo.totalMemory )
    TS_ASSERT_LESS_THAN( minfo.totalMemory, 100000000 )// totalMemory is in KB
    TS_ASSERT_LESS_THAN( minfo.availMemory, minfo.totalMemory )
    TS_ASSERT_LESS_THAN( minfo.freeRatio, 100 )
  }

  void testRelease()
  {
    TS_ASSERT_THROWS_NOTHING( MemoryManager::Instance().releaseFreeMemory(); );
    TS_ASSERT_THROWS_NOTHING( MemoryManager::Instance().releaseFreeMemoryIfAbove(0.9); );
  }


  /// Update in parallel to test thread safety
  void test_parallel()
  {
    PARALLEL_FOR_IF(true)
    for(int i=0; i<500; i++)
    {
      MemoryManager::Instance().releaseFreeMemoryIfAbove(0.9);
    }
  }

  /// Update in parallel to test thread safety
  void test_parallel_threadpool()
  {
    ThreadPool pool;
    for(int i=0; i<500; i++)
    {
      pool.schedule(new FunctionTask(boost::bind(&MemoryManagerImpl::releaseFreeMemoryIfAbove, &MemoryManager::Instance(), 0.9)) );
    }
    pool.joinAll();
  }

};

#endif /*MEMORYMANAGERTEST_H_*/
