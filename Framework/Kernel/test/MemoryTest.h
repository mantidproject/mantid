// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/FunctionTask.h"
#include "MantidKernel/Memory.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/ThreadPool.h"

#include <limits>
#include <string>

using namespace Mantid::Kernel;

void MemoryTest_myTaskFunction() {
  MemoryStats mem;
  mem.update();
  mem.getFreeRatio();
}

class MemoryTest : public CxxTest::TestSuite {
public:
  void setUp() override {
    m_memoryCheck = MemoryStats::memoryCheckEnabled() ? "On" : "Off";
    ConfigService::Instance().setString("algorithms.memorycheck", "On");
  }

  void tearDown() override { ConfigService::Instance().setString("algorithms.memorycheck", m_memoryCheck); }

  void test_checkAvailableMemory() {
    MemoryStats mem;
    TS_ASSERT(mem.checkAvailableMemory(1).empty());
    TS_ASSERT(!mem.checkAvailableMemory(std::numeric_limits<std::size_t>::max()).empty());
  }

  void test_checkAvailableMemory_is_skipped_when_turned_off() {
    MemoryStats mem;
    ConfigService::Instance().setString("algorithms.memorycheck", "Off");
    TS_ASSERT(mem.checkAvailableMemory(std::numeric_limits<std::size_t>::max()).empty());
  }

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

private:
  std::string m_memoryCheck;
};
