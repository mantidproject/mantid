#ifndef MANTID_KERNEL_LOGGERTEST_H_
#define MANTID_KERNEL_LOGGERTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidKernel/Logger.h"
#include "MantidKernel/MultiThreaded.h"

using namespace Mantid::Kernel;

class LoggerTest : public CxxTest::TestSuite
{
public:

  void do_test_logger_in_parallel(bool do_parallel)
  {
    PARALLEL_FOR_IF(do_parallel)
    for (int i=0; i<1000; i++)
    {
      Logger::get("MyTestLogger");
    }
  }

  void test_logger()
  {
    do_test_logger_in_parallel(true);
  }

  void test_logger_in_parallel()
  {
    do_test_logger_in_parallel(true);
  }


};


#endif /* MANTID_KERNEL_LOGGERTEST_H_ */

