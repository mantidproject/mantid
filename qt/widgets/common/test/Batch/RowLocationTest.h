#ifndef MANTID_MANTIDWIDGETS_ROWLOCATIONTEST_H
#define MANTID_MANTIDWIDGETS_ROWLOCATIONTEST_H

#include <cxxtest/TestSuite.h>
#include <gtest/gtest.h>
#include <vector>
#include <algorithm>
#include "MantidQtWidgets/Common/Batch/RowLocation.h"

using namespace MantidQt::MantidWidgets;
using namespace MantidQt::MantidWidgets::Batch;
using namespace testing;

class RowLocationTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RowLocationTest *createSuite() { return new RowLocationTest(); }
  static void destroySuite(RowLocationTest *suite) { delete suite; }

  void testEqualityIsBasedOnPaths() {
    TS_ASSERT(RowLocation({0, 1, 2}) == RowLocation({0, 1, 2}));
    TS_ASSERT(RowLocation({}) == RowLocation({}));
    TS_ASSERT(!(RowLocation({}) == RowLocation({1})));
    TS_ASSERT(!(RowLocation({1, 2}) == RowLocation({1})));

    TS_ASSERT(RowLocation({0, 0, 2}) != RowLocation({0, 1, 2}));
    TS_ASSERT(RowLocation({0, 2, 2}) != RowLocation({0, 1, 2}));
    TS_ASSERT(!(RowLocation({0, 2, 2}) != RowLocation({0, 2, 2})));
  }

  void testOrderingIsLexographicalBasedOnPath() {
    TS_ASSERT(RowLocation({}) < RowLocation({0}));
    TS_ASSERT(RowLocation({0}) < RowLocation({1}));
    TS_ASSERT(RowLocation({0}) < RowLocation({0, 1}));
    TS_ASSERT(RowLocation({0, 1}) < RowLocation({1, 0}));
    TS_ASSERT(RowLocation({0, 1}) < RowLocation({1}));
    TS_ASSERT(RowLocation({0, 1}) < RowLocation({0, 1, 1}));
  }

  void testOrderingIsLexographicalBasedOnPathSort() {
    // clang-format off
    auto items = std::vector<RowLocation>({
      RowLocation({}),
      RowLocation({1, 0}),
      RowLocation({1, 0, 2}),
      RowLocation({2, 0}),
      RowLocation({1, 2, 1}),
      RowLocation({2, 2}),
      RowLocation({1, 2}),
      RowLocation({1, 2, 0})
    });

    auto expected = std::vector<RowLocation>({
      RowLocation({}),
      RowLocation({1, 0}),
      RowLocation({1, 0, 2}),
      RowLocation({1, 2}),
      RowLocation({1, 2, 0}),
      RowLocation({1, 2, 1}),
      RowLocation({2, 0}),
      RowLocation({2, 2})
    });
    // clang-format on

    std::sort(items.begin(), items.end());

    TS_ASSERT_EQUALS(expected, items);
  }
};

#endif // MANTID_MANTIDWIDGETS_ROWLOCATIONTEST_H
