#ifndef MANTID_KERNEL_THREADPOOLRUNNABLETEST_H_
#define MANTID_KERNEL_THREADPOOLRUNNABLETEST_H_

#include <cxxtest/TestSuite.h>
#include <MantidKernel/Timer.h>
#include <MantidKernel/System.h>
#include <iostream>
#include <iomanip>

#include <MantidKernel/ThreadPoolRunnable.h>

using namespace Mantid::Kernel;

class ThreadPoolRunnableTest : public CxxTest::TestSuite
{
public:

  void testSomething()
  {
    TS_ASSERT(1);
  }


};


#endif /* MANTID_KERNEL_THREADPOOLRUNNABLETEST_H_ */

