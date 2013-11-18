#ifndef MANTID_KERNEL_LOGGERTEST_H_
#define MANTID_KERNEL_LOGGERTEST_H_

#include "MantidKernel/Logger.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>
#include <Poco/AutoPtr.h>
#include <Poco/File.h>
#include <Poco/Logger.h>
#include <Poco/SimpleFileChannel.h>
#include "MantidKernel/ConfigService.h"
#include <fstream>
#include "MantidKernel/ThreadPool.h"
#include <boost/bind.hpp>
#include "MantidKernel/FunctionTask.h"

using namespace Mantid::Kernel;
using Poco::SimpleFileChannel;
using Poco::AutoPtr;

class LoggerTest : public CxxTest::TestSuite
{
  std::string m_logFile;
  Logger & log;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoggerTest *createSuite() { return new LoggerTest(); }
  static void destroySuite( LoggerTest *suite ) { delete suite; }
  
  LoggerTest()
  : log(Logger::get("TestLogger"))
  {
  }

  //---------------------------------------------------------------------------
  /** Load the log file. Look at the first line
   * and compare to expected */
  void checkContents(std::string expected)
  {
    if (Poco::File(m_logFile).exists())
    {
      std::ifstream t;
      t.open(m_logFile.c_str());
      std::string line;
      std::getline(t, line);
      std::cout << "LINE IS " << line << std::endl;
      TS_ASSERT_EQUALS( line, expected );
    }
    else
    {
      TSM_ASSERT("test.log file was not found", 0);
    }
  }

  //---------------------------------------------------------------------------
  /** Get the same logger from many threads. */
  void test_Logger_get_inParallel()
  {
    PARALLEL_FOR_NO_WSP_CHECK()
    for (int i=0; i<1000; i++)
    {
      Logger::get("MyOtherTestLogger");
    }
  }

  //---------------------------------------------------------------------------
  /** TODO: Figure out a way to read back the log.
   * I tried the checkContents() call above but with no luck.
   */
  void test_basics()
  {
    log.information() << "Information Message" << std::endl;
  }

  //---------------------------------------------------------------------------
  /** Log very quickly from a lot of OpenMP threads*/
  void test_OpenMP_ParallelLogging()
  {
    PRAGMA_OMP(parallel for)
    for (int i=0; i<10000; i++)
    {
      log.information() << "Information Message " << i << std::endl;
    }
  }

  /** This will be called from the ThreadPool */
  void doLogInParallel(int num)
  {
    log.information() << "Information Message " << num << std::endl;
  }

  //---------------------------------------------------------------------------
  /** Log very quickly from a lot of Poco Threads.
   * The test passes if it does not segfault. */
  void test_ThreadPool_ParallelLogging()
  {
    ThreadPool tp;
    for (int i=0; i<10000; i++)
      tp.schedule(new FunctionTask(boost::bind(&LoggerTest::doLogInParallel, &*this, i)));
    tp.joinAll();
  }


};


#endif /* MANTID_KERNEL_LOGGERTEST_H_ */

