// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include "MantidKernel/Task.h"
#include "MantidKernel/ThreadScheduler.h"

using namespace Mantid::Kernel;

int ThreadSchedulerTest_numDestructed;

class ThreadSchedulerTest : public CxxTest::TestSuite {
public:
  class TaskDoNothing : public Task {
  public:
    TaskDoNothing() : Task() {}
    TaskDoNothing(double cost) : Task() { m_cost = cost; }

    ~TaskDoNothing() override {
      // To keep track of proper deleting of Task pointers
      ThreadSchedulerTest_numDestructed++;
    }

    void run() override {}
  };

  void do_basic_test(std::unique_ptr<ThreadScheduler> sc) {
    ThreadSchedulerTest_numDestructed = 0;
    TS_ASSERT(!sc->getAborted());
    TS_ASSERT_EQUALS(std::string(sc->getAbortException().what()), "");
    TS_ASSERT_EQUALS(sc->size(), 0);

    sc->push(std::make_shared<TaskDoNothing>());
    TS_ASSERT_EQUALS(sc->size(), 1);
    sc->push(std::make_shared<TaskDoNothing>());
    TS_ASSERT_EQUALS(sc->size(), 2);

    // Clear empties the queue
    ThreadSchedulerTest_numDestructed = 0;
    sc->clear();
    TS_ASSERT_EQUALS(sc->size(), 0);
    // And deletes the tasks properly
    TS_ASSERT_EQUALS(ThreadSchedulerTest_numDestructed, 2);
  }

  void test_basic_ThreadSchedulerFIFO() { do_basic_test(std::make_unique<ThreadSchedulerFIFO>()); }

  void test_basic_ThreadSchedulerLIFO() { do_basic_test(std::make_unique<ThreadSchedulerLIFO>()); }

  void test_basic_ThreadSchedulerLargestCost() { do_basic_test(std::make_unique<ThreadSchedulerLargestCost>()); }

  //==================================================================================================

  void do_test(ThreadScheduler *sc, double *costs, size_t *poppedIndices) {
    ThreadSchedulerTest_numDestructed = 0;

    // Create and push them in order
    TaskDoNothing *tasks[4];
    for (size_t i = 0; i < 4; i++) {
      auto temp = std::make_shared<TaskDoNothing>(costs[i]);
      sc->push(temp);
      tasks[i] = temp.get();
    }
    std::vector<std::shared_ptr<TaskDoNothing>> task(4, nullptr);

    // Pop them, and check that we get them in the order we expected
    for (size_t i = 0; i < 4; i++) {

      task[i] = std::dynamic_pointer_cast<TaskDoNothing>(sc->pop(0));
      size_t index = 0;
      for (index = 0; index < 4; index++)
        if (task[i]->cost() == tasks[index]->cost())
          break;
      TS_ASSERT_EQUALS(index, poppedIndices[i]);
    }

    // Nothing is left
    TS_ASSERT_EQUALS(sc->size(), 0);

    // And ThreadScheduler does not delete popped tasks in this way
    TS_ASSERT_EQUALS(ThreadSchedulerTest_numDestructed, 0);
  }

  void test_ThreadSchedulerFIFO() {
    std::unique_ptr<ThreadScheduler> sc = std::make_unique<ThreadSchedulerFIFO>();
    double costs[4] = {0, 1, 2, 3};
    size_t poppedIndices[4] = {0, 1, 2, 3};
    do_test(sc.get(), costs, poppedIndices);
  }

  void test_ThreadSchedulerLIFO() {
    std::unique_ptr<ThreadScheduler> sc = std::make_unique<ThreadSchedulerLIFO>();
    double costs[4] = {0, 1, 2, 3};
    size_t poppedIndices[4] = {3, 2, 1, 0};
    do_test(sc.get(), costs, poppedIndices);
  }

  void test_ThreadSchedulerLargestCost() {
    std::unique_ptr<ThreadScheduler> sc = std::make_unique<ThreadSchedulerLargestCost>();
    double costs[4] = {1, 5, 2, -3};
    size_t poppedIndices[4] = {1, 2, 0, 3};
    do_test(sc.get(), costs, poppedIndices);
  }
};
