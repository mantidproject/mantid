// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/FreeBlock.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Kernel;

class FreeBlockTest : public CxxTest::TestSuite {
public:
  void test_constructor() {
    FreeBlock b;
    TS_ASSERT_EQUALS(b.getFilePosition(), 0);
    TS_ASSERT_EQUALS(b.getSize(), 0);

    FreeBlock b2(123, 456);
    TS_ASSERT_EQUALS(b2.getFilePosition(), 123);
    TS_ASSERT_EQUALS(b2.getSize(), 456);

    FreeBlock b3(b2);
    TS_ASSERT_EQUALS(b3.getFilePosition(), 123);
    TS_ASSERT_EQUALS(b3.getSize(), 456);
  }

  void test_assignment() {
    FreeBlock b2(123, 456);
    FreeBlock b3;
    b3 = b2;
    TS_ASSERT_EQUALS(b3.getFilePosition(), 123);
    TS_ASSERT_EQUALS(b3.getSize(), 456);
  }

  void test_merge() {
    FreeBlock b1(100, 100);
    FreeBlock b2(200, 500);

    // Merge that succeeds
    TS_ASSERT(FreeBlock::merge(b1, b2));
    TS_ASSERT_EQUALS(b1.getFilePosition(), 100);
    TS_ASSERT_EQUALS(b1.getSize(), 600);

    // Merge that fails
    FreeBlock b3(100, 100);
    FreeBlock b4(201, 100);
    TS_ASSERT(!FreeBlock::merge(b3, b4));
    TS_ASSERT_EQUALS(b3.getFilePosition(), 100);
    TS_ASSERT_EQUALS(b3.getSize(), 100);
  }
};

class FreeBlockTestPerformance : public CxxTest::TestSuite {
public:
  std::vector<FreeBlock> blocks;
  size_t num;
  void setUp() override {
    num = 1000000;
    // Make a list where 1/3 of the blocks are adjacent
    for (size_t i = 0; i < num; i++)
      blocks.emplace_back(FreeBlock(i * 10, (i % 3 == 0) ? 10 : 7));
  }

  void test_merge() {
    // Merge by going backwards through the list.
    for (size_t i = num - 1; i > 0; i--)
      FreeBlock::merge(blocks[i - 1], blocks[i]);
    // The first block is merged into size 17
    TS_ASSERT_EQUALS(blocks[0].getSize(), 17);
  }
};
