// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include "MantidKernel/FunctionTask.h"
#include "MantidKernel/Memory.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/ThreadPool.h"

using namespace Mantid::Kernel;

void MemoryTest_myTaskFunction() {
  MemoryStats mem;
  mem.update();
  mem.getFreeRatio();
}

class MemoryTest : public CxxTest::TestSuite {
public:
  void test_update() {
    MemoryStats mem;
    TS_ASSERT_THROWS_NOTHING(mem.update());
    TS_ASSERT_LESS_THAN_EQUALS(0, mem.availMem());
    TS_ASSERT_DIFFERS(mem.availMemStr(), "");
    TS_ASSERT_LESS_THAN_EQUALS(0, mem.reservedMem());
    TS_ASSERT_LESS_THAN(0, mem.totalMem()); // The machine must have some memory
    TS_ASSERT_DIFFERS(mem.totalMemStr(), "");

    // Current process stats
    TS_ASSERT_LESS_THAN(0, mem.residentMem()); // Current process must use something
    TS_ASSERT_DIFFERS(mem.resUsageStr(), "");
    TS_ASSERT_LESS_THAN_EQUALS(0, mem.virtualMem());
    TS_ASSERT_DIFFERS(mem.vmUsageStr(), "");
  }

  /// Update in parallel to test thread safety
  void test_parallel() {
    PARALLEL_FOR_NO_WSP_CHECK()
    for (int i = 0; i < 500; i++) {
      MemoryStats mem;
      mem.update();
      mem.getFreeRatio();
    }
  }

  void test_parallel_threadpool() {
    ThreadPool pool;
    for (int i = 0; i < 500; i++) {
      pool.schedule(std::make_shared<FunctionTask>(&MemoryTest_myTaskFunction, 1.0));
    }
    pool.joinAll();
  }
};
