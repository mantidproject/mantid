// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/Batch/FindSubtreeRoots.h"
#include "MantidQtWidgets/Common/Batch/RowLocation.h"
#include <algorithm>
#include <cxxtest/TestSuite.h>
#include <gtest/gtest.h>
#include <vector>

using namespace MantidQt::MantidWidgets;
using namespace MantidQt::MantidWidgets::Batch;
using namespace testing;

class FindSubtreeRootsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FindSubtreeRootsTest *createSuite() { return new FindSubtreeRootsTest(); }
  static void destroySuite(FindSubtreeRootsTest *suite) { delete suite; }

  void testForSingleLocation() {
    auto findSubtreeRoots = FindSubtreeRoots();
    auto region = std::vector<RowLocation>{{RowLocation({1})}};

    auto roots = findSubtreeRoots(region);

    auto expectedRoots = std::vector<RowLocation>({RowLocation({1})});

    TS_ASSERT(roots.has_value());
    TS_ASSERT_EQUALS(expectedRoots, roots.value());
  }

  void testTwoSiblingsResultsInTwoRoots() {
    auto findSubtreeRoots = FindSubtreeRoots();
    auto region = std::vector<RowLocation>({RowLocation({1}), RowLocation({2})});

    // clang-format off
    auto expectedRoots = std::vector<RowLocation>({
      RowLocation({1}),
      RowLocation({2})
    });
    // clang-format on

    auto roots = findSubtreeRoots(region);
    TS_ASSERT(roots.has_value());
    TS_ASSERT_EQUALS(expectedRoots, roots.value());
  }

  void testParentAndChildResultsInParent() {
    auto findSubtreeRoots = FindSubtreeRoots();
    auto region = std::vector<RowLocation>({RowLocation({1}), RowLocation({1, 2})});

    auto expectedRoots = std::vector<RowLocation>({RowLocation({1})});

    auto roots = findSubtreeRoots(region);
    TS_ASSERT(roots.has_value());
    TS_ASSERT_EQUALS(expectedRoots, roots.value());
  }

  void testParentWithChildAndSiblingResultsInParentAndSibling() {
    auto findSubtreeRoots = FindSubtreeRoots();
    // clang-format off
    auto region = std::vector<RowLocation>({
      RowLocation({1}),
      RowLocation({1, 0}),
      RowLocation({2})
    });
    // clang-format on

    auto expectedRoots = std::vector<RowLocation>({RowLocation({1}), RowLocation({2})});

    auto roots = findSubtreeRoots(region);
    TS_ASSERT(roots.has_value());
    TS_ASSERT_EQUALS(expectedRoots, roots.value());
  }

  void testFindsRootOfNonTrivialTree() {
    auto findSubtreeRoots = FindSubtreeRoots();
    // clang-format off
    auto region = std::vector<RowLocation>({
          RowLocation({1}),
          RowLocation({1, 0}),
          RowLocation({1, 0, 1}),
          RowLocation({1, 1})
    });
    // clang-format on

    auto expectedRoots = std::vector<RowLocation>({RowLocation({1})});

    auto roots = findSubtreeRoots(region);
    TS_ASSERT(roots.has_value());
    TS_ASSERT_EQUALS(expectedRoots, roots.value());
  }

  void testFailsForLevelGap() {
    auto findSubtreeRoots = FindSubtreeRoots();
    // clang-format off
    auto region = std::vector<RowLocation>({
      RowLocation({1}),
      RowLocation({1, 0}),
      RowLocation({1, 0, 1, 2})
    });
    // clang-format on

    TS_ASSERT(!findSubtreeRoots(region).has_value());
  }

  void testForRealisticTree() {
    auto findSubtreeRoots = FindSubtreeRoots();
    // clang-format off
    auto region = std::vector<RowLocation>({
        RowLocation({0}),
        RowLocation({0, 0}),
        RowLocation({0, 1}),
        RowLocation({1}),
        RowLocation({1, 0}),
        RowLocation({1, 0, 0}),
        RowLocation({1, 0, 0, 0}),
        RowLocation({1, 0, 0, 1}),
        RowLocation({1, 0, 0, 2}),
        RowLocation({1, 2}),
        RowLocation({2}),
        RowLocation({3})
    });
    // clang-format on

    // clang-format off
    auto expectedRoots = std::vector<RowLocation>({
      RowLocation({0}),
      RowLocation({1}),
      RowLocation({2}),
      RowLocation({3})
    });
    // clang-format on

    auto roots = findSubtreeRoots(region);
    TS_ASSERT(roots.has_value());
    TS_ASSERT_EQUALS(expectedRoots, roots.value());
  }

  void testFailsForShallowRoot() {
    auto findSubtreeRoots = FindSubtreeRoots();
    // clang-format off
    auto region = std::vector<RowLocation>({
        RowLocation({0, 0}),
        RowLocation({0, 0, 0}),
        RowLocation({0, 0, 1}),
        RowLocation({1}),
        RowLocation({1, 0}),
        RowLocation({1, 1}),
        RowLocation({1, 2})
    });
    // clang-format on

    auto roots = findSubtreeRoots(region);
    TS_ASSERT(!roots.has_value())
  }

  void testFailsForDeepRoot() {
    auto findSubtreeRoots = FindSubtreeRoots();
    // clang-format off
    auto region = std::vector<RowLocation>({
      RowLocation({0}),
      RowLocation({0, 0}),
      RowLocation({0, 1}),
      RowLocation({0, 2}),
      RowLocation({1, 0}),
      RowLocation({1, 0, 0}),
      RowLocation({1, 0, 1})
    });
    // clang-format on

    auto roots = findSubtreeRoots(region);
    TS_ASSERT(!roots.has_value())
  }

  void testFailsForDeepRootImmediatelyAfterFirstRoot() {
    auto findSubtreeRoots = FindSubtreeRoots();
    // clang-format off
    auto region = std::vector<RowLocation>({
      RowLocation({0}),
      RowLocation({1, 0})
    });
    // clang-format on

    auto roots = findSubtreeRoots(region);
    TS_ASSERT(!roots.has_value())
  }

  void testFailsForShallowRootImmediatelyAfterFirstRoot() {
    auto findSubtreeRoots = FindSubtreeRoots();
    // clang-format off
    auto region = std::vector<RowLocation>({
      RowLocation({0, 0}),
      RowLocation({1}),
    });
    // clang-format on

    auto roots = findSubtreeRoots(region);
    TS_ASSERT(!roots.has_value())
  }

  void testFailsForDisconnectedRoots() {
    auto findSubtreeRoots = FindSubtreeRoots();
    // clang-format off
    auto region = std::vector<RowLocation>({
      RowLocation({0, 0}),
      RowLocation({1, 0}),
    });
    // clang-format on

    auto roots = findSubtreeRoots(region);
    TS_ASSERT(!roots.has_value())
  }

  void testForDocumentationFailTree() {
    auto findSubtreeRoots = FindSubtreeRoots();
    // clang-format off
    auto region = std::vector<RowLocation>({
        RowLocation({0, 0}),
        RowLocation({0, 0, 0}),
        RowLocation({0, 0, 1}),
        RowLocation({1}),
        RowLocation({1, 0}),
        RowLocation({1, 1}),
        RowLocation({1, 2})
    });
    // clang-format on
    auto roots = findSubtreeRoots(region);
    TS_ASSERT(!roots.has_value())
  }
};
