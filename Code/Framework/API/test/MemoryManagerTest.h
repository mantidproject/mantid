#ifndef MEMORYMANAGERTEST_H_
#define MEMORYMANAGERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/MemoryManager.h"
#include "MantidKernel/Exception.h"

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

};

#endif /*MEMORYMANAGERTEST_H_*/
