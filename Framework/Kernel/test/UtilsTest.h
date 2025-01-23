// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include "MantidKernel/Utils.h"

using namespace Mantid::Kernel;

class UtilsTest : public CxxTest::TestSuite {
public:
  /** Set up counter array */
  void test_nestedForLoopSetUp() {
    size_t counters[3];
    Utils::NestedForLoop::SetUp(3, counters, 123);
    for (size_t i = 0; i < 3; i++)
      TS_ASSERT_EQUALS(counters[i], 123);
  }

  /** Set up an index maker*/
  void test_nestedForLoopSetUpIndexMaker() {
    size_t index_max[4];
    Utils::NestedForLoop::SetUp(4, index_max);
    index_max[0] = 10;
    index_max[1] = 5;
    index_max[2] = 2;
    index_max[3] = 8;

    size_t index_maker[4];
    Utils::NestedForLoop::SetUpIndexMaker(4, index_maker, index_max);
    TS_ASSERT_EQUALS(index_maker[0], 1);
    TS_ASSERT_EQUALS(index_maker[1], 10);
    TS_ASSERT_EQUALS(index_maker[2], 50);
    TS_ASSERT_EQUALS(index_maker[3], 100);
  }

  /** Use the index_maker */
  void test_nestedForLoopGetLinearIndex() {
    size_t index_max[4];
    Utils::NestedForLoop::SetUp(4, index_max);
    index_max[0] = 10;
    index_max[1] = 5;
    index_max[2] = 2;
    index_max[3] = 8;
    size_t index_maker[4];
    Utils::NestedForLoop::SetUpIndexMaker(4, index_maker, index_max);

    size_t index[4] = {1, 1, 1, 1};
    TS_ASSERT_EQUALS(Utils::NestedForLoop::GetLinearIndex(4, index, index_maker), 1 + 10 + 50 + 100);
    size_t index2[4] = {3, 2, 1, 0};
    TS_ASSERT_EQUALS(Utils::NestedForLoop::GetLinearIndex(4, index2, index_maker), 3 + 20 + 50);
  }

  /** Back-conversion from linear index */
  void test_nestedForLoopGetIndicesFromLinearIndex() {
    size_t index_max[4];
    Utils::NestedForLoop::SetUp(4, index_max);
    index_max[0] = 10;
    index_max[1] = 5;
    index_max[2] = 2;
    index_max[3] = 8;
    size_t index_maker[4];
    Utils::NestedForLoop::SetUpIndexMaker(4, index_maker, index_max);

    size_t indices[4] = {0, 0, 0, 0};
    size_t out_indices[4];
    bool allDone = false;
    while (!allDone) {
      // Convert to linear index
      size_t linear_index = Utils::NestedForLoop::GetLinearIndex(4, indices, index_maker);

      // Back-convert
      Utils::NestedForLoop::GetIndicesFromLinearIndex(4, linear_index, index_maker, index_max, out_indices);
      for (size_t d = 0; d < 4; d++) {
        TS_ASSERT_EQUALS(out_indices[d], indices[d]);
      }

      // Keep going
      allDone = Utils::NestedForLoop::Increment(4, indices, index_max);
    }
  }

  /** Make a nested loop with each counter resetting at 0 */
  void test_nestedForLoopIncrement() {
    size_t counters[3];
    Utils::NestedForLoop::SetUp(3, counters);
    size_t counters_max[3];
    Utils::NestedForLoop::SetUp(3, counters_max, 10);

    // The data
    size_t data[10][10][10];
    memset(data, 0, sizeof(data));

    bool allDone = false;
    while (!allDone) {
      data[counters[0]][counters[1]][counters[2]] = counters[0] * 10000 + counters[1] * 100 + counters[2];
      allDone = Utils::NestedForLoop::Increment(3, counters, counters_max);
    }

    for (size_t x = 0; x < 10; x++)
      for (size_t y = 0; y < 10; y++)
        for (size_t z = 0; z < 10; z++) {
          TS_ASSERT_EQUALS(data[x][y][z], x * 10000 + y * 100 + z);
        }
  }

  /** Make a nested loop but use a non-zero starting index for each counter */
  void test_nestedForLoopIncrement_nonZeroMinimum() {
    size_t counters[3];
    Utils::NestedForLoop::SetUp(3, counters, 4);
    size_t counters_min[3];
    Utils::NestedForLoop::SetUp(3, counters_min, 4);
    size_t counters_max[3];
    Utils::NestedForLoop::SetUp(3, counters_max, 8);

    // The data
    size_t data[10][10][10];
    memset(data, 0, sizeof(data));

    bool allDone = false;
    while (!allDone) {
      data[counters[0]][counters[1]][counters[2]] = counters[0] * 10000 + counters[1] * 100 + counters[2];
      allDone = Utils::NestedForLoop::Increment(3, counters, counters_max, counters_min);
    }

    for (size_t x = 0; x < 10; x++)
      for (size_t y = 0; y < 10; y++)
        for (size_t z = 0; z < 10; z++) {
          if ((x < 4 || y < 4 || z < 4) || (x >= 8 || y >= 8 || z >= 8)) {
            TS_ASSERT_EQUALS(data[x][y][z], 0);
          } else {
            TS_ASSERT_EQUALS(data[x][y][z], x * 10000 + y * 100 + z);
          }
        }
  }

  void test_nestedLinearIndexes() {
    std::vector<size_t> numBins(3, 10);
    numBins[1] = 20;
    numBins[2] = 5;

    size_t ic(0);

    for (size_t k = 0; k < numBins[2]; k++) {
      for (size_t j = 0; j < numBins[1]; j++) {
        for (size_t i = 0; i < numBins[0]; i++) {
          auto indexes = Utils::getIndicesFromLinearIndex(ic, numBins);
          ic++;

          TS_ASSERT_EQUALS(indexes[0], i);
          TS_ASSERT_EQUALS(indexes[1], j);
          TS_ASSERT_EQUALS(indexes[2], k);
        }
      }
    }
  }

  void test_nestedLinearIndexesWith0() {
    std::vector<size_t> numBins(3, 10);
    numBins[1] = 1; // there can not be 0, shluld be at least 1
    numBins[2] = 5;

    size_t ic(0);

    for (size_t k = 0; k < numBins[2]; k++) {
      for (size_t j = 0; j < numBins[1]; j++) {
        for (size_t i = 0; i < numBins[0]; i++) {
          auto indexes = Utils::getIndicesFromLinearIndex(ic, numBins);
          ic++;

          TS_ASSERT_EQUALS(indexes[0], i);
          TS_ASSERT_EQUALS(indexes[1], j);
          TS_ASSERT_EQUALS(indexes[2], k);
        }
      }
    }
  }
};
