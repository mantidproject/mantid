// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>
#include <limits>
#include <stdexcept>

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

  void test_checkAvailableMemory() {
    MemoryStats mem;

    // A trivially small request always fits, whether compared to available or total memory
    TS_ASSERT(mem.checkAvailableMemory(1).empty());
    TS_ASSERT(mem.checkAvailableMemory(1, /*compareToTotalMemory=*/true).empty());

    // A request larger than the whole machine never fits, on either basis
    constexpr std::size_t tooMuch = std::numeric_limits<std::size_t>::max();
    TS_ASSERT(!mem.checkAvailableMemory(tooMuch).empty());
    TS_ASSERT(!mem.checkAvailableMemory(tooMuch, /*compareToTotalMemory=*/true).empty());

    // The fraction scales the usable limit when comparing to total memory
    const std::size_t totalBytes = mem.totalMem() * 1024;
    const auto threeQuarters = static_cast<std::size_t>(0.75 * static_cast<double>(totalBytes));
    // 0.75 * total exceeds a limit of half the total memory ...
    TS_ASSERT(!mem.checkAvailableMemory(threeQuarters, /*compareToTotalMemory=*/true, 0.5).empty());
    // ... but fits within the full total memory
    TS_ASSERT(mem.checkAvailableMemory(threeQuarters, /*compareToTotalMemory=*/true, 1.0).empty());

    // The boundary values of the fraction are accepted
    TS_ASSERT_THROWS_NOTHING(mem.checkAvailableMemory(1, /*compareToTotalMemory=*/true, 1.0));
    TS_ASSERT_THROWS_NOTHING(mem.checkAvailableMemory(1, /*compareToTotalMemory=*/true, 0.001));
    // A fraction outside (0, 1] is a caller error and must throw before any size_t conversion
    TS_ASSERT_THROWS(mem.checkAvailableMemory(1, true, 0.0), const std::invalid_argument &);
    TS_ASSERT_THROWS(mem.checkAvailableMemory(1, true, -0.5), const std::invalid_argument &);
    TS_ASSERT_THROWS(mem.checkAvailableMemory(1, true, 1.5), const std::invalid_argument &);
    TS_ASSERT_THROWS(mem.checkAvailableMemory(1, true, std::numeric_limits<double>::quiet_NaN()),
                     const std::invalid_argument &);
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
