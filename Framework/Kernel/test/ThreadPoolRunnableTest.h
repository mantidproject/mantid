// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/ThreadPoolRunnable.h"
#include "MantidKernel/ThreadScheduler.h"
#include "MantidKernel/Timer.h"

#include <cxxtest/TestSuite.h>
#include <memory>

using namespace Mantid::Kernel;

int ThreadPoolRunnableTest_value;

class ThreadPoolRunnableTest : public CxxTest::TestSuite {
public:
  void test_constructor() { TS_ASSERT_THROWS(ThreadPoolRunnable(0, nullptr), const std::invalid_argument &); }

  //=======================================================================================
  class SimpleTask : public Task {
    void run() override { ThreadPoolRunnableTest_value = 1234; }
  };

  void test_run() {
    std::unique_ptr<ThreadPoolRunnable> tpr;
    std::unique_ptr<ThreadScheduler> sc = std::make_unique<ThreadSchedulerFIFO>();
    tpr = std ::make_unique<ThreadPoolRunnable>(0, sc.get());
    sc->push(std::make_shared<SimpleTask>());
    TS_ASSERT_EQUALS(sc->size(), 1);

    // Run it
    ThreadPoolRunnableTest_value = 0;
    tpr->run();

    // The task worked
    TS_ASSERT_EQUALS(ThreadPoolRunnableTest_value, 1234);
    // Nothing more in the queue.
    TS_ASSERT_EQUALS(sc->size(), 0);
  }

  //=======================================================================================
  /** Class that throws an exception */
  class TaskThatThrows : public Task {
    void run() override {
      ThreadPoolRunnableTest_value += 1;
      throw Mantid::Kernel::Exception::NotImplementedError("Test exception from TaskThatThrows.");
    }
  };

  void test_run_throws() {
    std::unique_ptr<ThreadPoolRunnable> tpr;
    std::unique_ptr<ThreadScheduler> sc = std::make_unique<ThreadSchedulerFIFO>();
    tpr = std::make_unique<ThreadPoolRunnable>(0, sc.get());

    // Put 10 tasks in
    for (size_t i = 0; i < 10; i++)
      sc->push(std::make_shared<TaskThatThrows>());

    // The task throws but the runnable just aborts instead
    ThreadPoolRunnableTest_value = 0;
    TS_ASSERT_THROWS_NOTHING(tpr->run());

    // Nothing more in the queue.
    TS_ASSERT_EQUALS(sc->size(), 0);
    // Yet only one task actually ran.
    TS_ASSERT_EQUALS(ThreadPoolRunnableTest_value, 1);

    TS_ASSERT(sc->getAborted());
    // Get the reason for aborting
    std::runtime_error e = sc->getAbortException();
    std::string what = e.what();
    TS_ASSERT_EQUALS(what, "Test exception from TaskThatThrows.");
  }
};
