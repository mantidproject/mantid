// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_LOGGERTEST_H_
#define MANTID_KERNEL_LOGGERTEST_H_

#include "MantidKernel/FunctionTask.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/ThreadPool.h"

#include <Poco/AutoPtr.h>
#include <Poco/File.h>
#include <Poco/Logger.h>
#include <Poco/SimpleFileChannel.h>

#include <cxxtest/TestSuite.h>
#include <fstream>

using namespace Mantid::Kernel;
using Poco::AutoPtr;
using Poco::SimpleFileChannel;

class LoggerTest : public CxxTest::TestSuite {
  std::string m_logFile;
  Logger log;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoggerTest *createSuite() { return new LoggerTest(); }
  static void destroySuite(LoggerTest *suite) { delete suite; }

  LoggerTest() : log("TestLogger") {}

  //---------------------------------------------------------------------------
  /** Load the log file. Look at the first line
   * and compare to expected */
  void checkContents(std::string expected) {
    if (Poco::File(m_logFile).exists()) {
      std::ifstream t;
      t.open(m_logFile.c_str());
      std::string line;
      std::getline(t, line);
      std::cout << "LINE IS " << line << '\n';
      TS_ASSERT_EQUALS(line, expected);
    } else {
      TSM_ASSERT("test.log file was not found", 0);
    }
  }

  //---------------------------------------------------------------------------
  /** Build same named logger from many threads. */
  void test_Logger_get_inParallel() {
    int level(0);

    PARALLEL_FOR_NO_WSP_CHECK()
    for (int i = 0; i < 1000; i++) {
      Logger otherLogger("MyOtherTestLogger");
      level =
          otherLogger.getLevel(); // here so the optimiser doesn't kill the loop
    }

    UNUSED_ARG(level);
  }

  //---------------------------------------------------------------------------
  /** TODO: Figure out a way to read back the log.
   * I tried the checkContents() call above but with no luck.
   */
  void test_basics() { log.information() << "Information Message\n"; }

  //---------------------------------------------------------------------------
  /** Log very quickly from a lot of OpenMP threads*/
  void test_OpenMP_ParallelLogging() {
    PRAGMA_OMP(parallel for)
    for (int i = 0; i < 1000; i++) {
      log.information() << "Information Message " << i << '\n';
    }
  }

  /** This will be called from the ThreadPool */
  void doLogInParallel(int num) {
    log.information() << "Information Message " << num << '\n';
  }

  //---------------------------------------------------------------------------
  /** Log very quickly from a lot of Poco Threads.
   * The test passes if it does not segfault. */
  void test_ThreadPool_ParallelLogging() {
    ThreadPool tp;
    for (int i = 0; i < 1000; i++)
      tp.schedule(std::make_shared<FunctionTask>(
          boost::bind(&LoggerTest::doLogInParallel, &*this, i)));
    tp.joinAll();
  }
};

//================================= Performance Tests
//=======================================
class LoggerTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoggerTestPerformance *createSuite() {
    return new LoggerTestPerformance();
  }
  static void destroySuite(LoggerTestPerformance *suite) { delete suite; }

  void test_Logging_At_High_Frequency_At_Equal_Level_To_Current_Level() {
    Logger logger("LoggerTestPerformance");
    logger.setLevel(Logger::Priority::PRIO_INFORMATION);

    for (int i = 0; i < 100000; i++) {
      logger.information() << "Information Message " << i << '\n';
    }
  }

  void
  test_Logging_At_High_Frequency_In_Parallel_At_Equal_Level_To_Current_Level() {
    Logger logger("LoggerTestPerformance");
    logger.setLevel(Logger::Priority::PRIO_INFORMATION);

    PRAGMA_OMP(parallel for)
    for (int i = 0; i < 100000; i++) {
      logger.information() << "Information Message " << i << '\n';
    }
  }

  void test_Logging_At_High_Frequency_At_Lower_Than_Current_Level() {
    Logger logger("LoggerTestPerformance");
    logger.setLevel(Logger::Priority::PRIO_INFORMATION);

    for (int i = 0; i < 100000; i++) {
      logger.debug() << "Debug Message " << i << '\n';
    }
  }

  void
  test_Logging_At_High_Frequency_In_Parallel_At_Lower_Than_Current_Level() {
    Logger logger("LoggerTestPerformance");
    logger.setLevel(Logger::Priority::PRIO_INFORMATION);

    PRAGMA_OMP(parallel for)
    for (int i = 0; i < 100000; i++) {
      logger.debug() << "Debug Message " << i << '\n';
    }
  }
};

#endif /* MANTID_KERNEL_LOGGERTEST_H_ */
