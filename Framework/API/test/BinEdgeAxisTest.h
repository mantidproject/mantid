// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_BINEDGEAXISTEST_H_
#define MANTID_API_BINEDGEAXISTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/BinEdgeAxis.h"

using Mantid::API::BinEdgeAxis;

class BinEdgeAxisTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BinEdgeAxisTest *createSuite() { return new BinEdgeAxisTest(); }
  static void destroySuite(BinEdgeAxisTest *suite) { delete suite; }

  void test_Clone_With_Only_Workspace_Returns_BinEdgeAxis_() {
    BinEdgeAxis ax1(10);
    Mantid::API::Axis *copy = ax1.clone(nullptr);
    auto *typedCopy = dynamic_cast<BinEdgeAxis *>(copy);

    TS_ASSERT(typedCopy);

    delete copy;
  }

  void
  test_Clone_With_Only_Length_And_Workspace_Returns_BinEdgeAxis_With_New_Length() {
    BinEdgeAxis ax1(10);
    Mantid::API::Axis *copy = ax1.clone(20, nullptr);
    auto *typedCopy = dynamic_cast<BinEdgeAxis *>(copy);

    TS_ASSERT(typedCopy);
    TS_ASSERT_EQUALS(20, typedCopy->length());

    delete copy;
  }

  void test_indexOfValue_Returns_Index_Of_Bin_For_Valid_Value() {
    const size_t length(10); // 9 bin centres
    BinEdgeAxis ax1(length);
    for (size_t i = 0; i < length; ++i) {
      ax1.setValue(i, static_cast<double>(i + 1));
    }

    TS_ASSERT_EQUALS(0, ax1.indexOfValue(1.1)); // start
    TS_ASSERT_EQUALS(4, ax1.indexOfValue(5.4)); // ~middle
    TS_ASSERT_EQUALS(8, ax1.indexOfValue(9.9)); // end
  }

  void test_CreateBinBoundaries_Simply_Returns_Same_Values() {
    const size_t length(10); // 9 bin centres
    BinEdgeAxis ax1(length);
    for (size_t i = 0; i < length; ++i) {
      ax1.setValue(i, static_cast<double>(i + 1));
    }

    auto edges = ax1.createBinBoundaries();
    TS_ASSERT_EQUALS(length, edges.size());
    for (size_t i = 0; i < length; ++i) {
      TS_ASSERT_EQUALS(ax1.getValue(i), edges[i]);
    }
  }

  // ------------------------- Failure cases -----------------------------------

  void test_indexOfValue_Throws_OutOfRange_For_Invalid_Value() {
    const size_t length(10); // 9 bin centres
    BinEdgeAxis ax1(length);
    for (size_t i = 0; i < length; ++i) {
      ax1.setValue(i, static_cast<double>(i + 1));
    }

    TS_ASSERT_THROWS(ax1.indexOfValue(0.9), const std::out_of_range &); // start
    TS_ASSERT_THROWS(ax1.indexOfValue(10.1), const std::out_of_range &); // end
  }
};

#endif /* MANTID_API_BINEDGEAXISTEST_H_ */
