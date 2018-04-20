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

  void testDefaultIsRoot() { TS_ASSERT(RowLocation().isRoot()); }

  void testEqualityIsBasedOnPaths() {
    TS_ASSERT(RowLocation({0, 1, 2}) == RowLocation({0, 1, 2}));
    TS_ASSERT(RowLocation() == RowLocation());
    TS_ASSERT(!(RowLocation() == RowLocation({1})));
    TS_ASSERT(!(RowLocation({1, 2}) == RowLocation({1})));

    TS_ASSERT(RowLocation({0, 0, 2}) != RowLocation({0, 1, 2}));
    TS_ASSERT(RowLocation({0, 2, 2}) != RowLocation({0, 1, 2}));
    TS_ASSERT(!(RowLocation({0, 2, 2}) != RowLocation({0, 2, 2})));
  }

  void testOrderingIsLexographicalBasedOnPath() {
    TS_ASSERT(RowLocation() < RowLocation({0}));
    TS_ASSERT(RowLocation({0}) < RowLocation({1}));
    TS_ASSERT(RowLocation({0}) < RowLocation({0, 1}));
    TS_ASSERT(RowLocation({0, 1}) < RowLocation({1, 0}));
    TS_ASSERT(RowLocation({0, 1}) < RowLocation({1}));
    TS_ASSERT(RowLocation({0, 1}) < RowLocation({0, 1, 1}));
  }

  void testOrderingIsLexographicalBasedOnPathSort() {
    // clang-format off
    auto items = std::vector<RowLocation>({
      RowLocation(),
      RowLocation({1, 0}),
      RowLocation({1, 0, 2}),
      RowLocation({2, 0}),
      RowLocation({1, 2, 1}),
      RowLocation({2, 2}),
      RowLocation({1, 2}),
      RowLocation({1, 2, 0})
    });

    auto expected = std::vector<RowLocation>({
      RowLocation(),
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

  void sortBoth(std::vector<RowLocation> &lhs, std::vector<RowLocation> &rhs) {
    std::sort(lhs.begin(), lhs.end());
    std::sort(rhs.begin(), rhs.end());
  }

  void testRootIsParentOfDirectDescendant() {
    auto root = RowLocation();
    auto directRootDescendant = RowLocation({3});
    TS_ASSERT(directRootDescendant.isChildOf(root));
  }

  void testRootIsNotParentOfIndirectDescendant() {
    auto root = RowLocation();
    auto indirectRootDescendant = RowLocation({2, 1});
    TS_ASSERT(!indirectRootDescendant.isChildOf(root));
  }

  void testRootIsNotChildOfAnything() {
    auto root = RowLocation();
    TS_ASSERT(!root.isChildOf(RowLocation()));
    TS_ASSERT(!root.isChildOf(RowLocation({1})));
    TS_ASSERT(!root.isChildOf(RowLocation({0, 0})));
  }

  void testDirectDescentantOfNonRootNodeIsChildOfNonRootNode() {
    auto nonRootNode = RowLocation({2});
    auto childOfNonRootNode = RowLocation({2, 3});
    TS_ASSERT(childOfNonRootNode.isChildOf(nonRootNode));

    auto deepNonRootNode = RowLocation({2, 3, 4, 5, 6, 10});
    auto deepNonRootNodeChild = RowLocation({2, 3, 4, 5, 6, 10, 1});
    TS_ASSERT(deepNonRootNodeChild.isChildOf(deepNonRootNode));
  }

  void testIndirectDescendantIsNotChild() {
    auto ancestor = RowLocation({1, 2});
    auto indirectDescendant = RowLocation({1, 2, 3, 4});
    TS_ASSERT(!ancestor.isChildOf(indirectDescendant));
  }

  void testNonDescendantIsNotChild() {
    auto child = RowLocation({1, 2, 4});

    auto parent = RowLocation({1, 2});
    TS_ASSERT(!parent.isChildOf(child));

    auto sibling = RowLocation({1, 2, 3});
    TS_ASSERT(!sibling.isChildOf(child));
  }

  void testPositionRelativeToRootIsEqualToSelf() {
    auto node = RowLocation({0, 1, 2});
    TS_ASSERT_EQUALS(node, node.relativeTo(RowLocation()));
  }

  void testPositionRelativeToParent() {
    auto node = RowLocation({0, 1, 2});
    auto parent = RowLocation({0, 1});
    TS_ASSERT_EQUALS(RowLocation({2}), node.relativeTo(parent));
  }

  void testPositionRelativeNonParentAncestor() {
    auto node = RowLocation({0, 1, 2, 3, 4, 10});
    auto ancestor = RowLocation({0, 1});
    TS_ASSERT_EQUALS(RowLocation({2, 3, 4, 10}), node.relativeTo(ancestor));
  }

  void testFindRootsForSingleLocation() {
    auto region = std::vector<RowLocation>{{RowLocation({1})}};
    auto roots = findRootNodes(region).value();
    TS_ASSERT_EQUALS(1, roots.size());
    TS_ASSERT_EQUALS(region[0], roots[0]);
  }

  void testFindRootsForTwoSiblings() {
    auto region = std::vector<RowLocation>{{
        RowLocation({1}), RowLocation({2}), }};
    auto roots = findRootNodes(region).value();
    sortBoth(region, roots);
    TS_ASSERT_EQUALS(region, roots);
  }

  void testFindRootsForParentAndChild() {
    auto region = std::vector<RowLocation>({
        RowLocation({1}), RowLocation({1, 2}),
    });

    auto expectedRoots = std::vector<RowLocation>({RowLocation({1})});

    auto roots = findRootNodes(region).value();
    sortBoth(expectedRoots, roots);
    TS_ASSERT_EQUALS(expectedRoots, roots);
  }

  void testFindRootsForParentWithChildAndSibling() {
    auto region = std::vector<RowLocation>({
        RowLocation({1}), RowLocation({1, 0}), RowLocation({2}),
    });

    auto expectedRoots =
        std::vector<RowLocation>({RowLocation({1}), RowLocation({2})});

    auto roots = findRootNodes(region).value();
    sortBoth(expectedRoots, roots);
    TS_ASSERT_EQUALS(expectedRoots, roots);
  }

  void testFindRootsForNonTrivialSibling() {
    auto region = std::vector<RowLocation>({
        RowLocation({1}), RowLocation({1, 0}), RowLocation({1, 0, 1}),
        RowLocation({1, 1}),
    });

    auto expectedRoots = std::vector<RowLocation>({
        RowLocation({1}),
    });

    auto roots = findRootNodes(region).value();
    sortBoth(expectedRoots, roots);
    TS_ASSERT_EQUALS(expectedRoots, roots);
  }

  void testFindRootsFailsForNonChildDescendantGap() {
    auto region = std::vector<RowLocation>({
        RowLocation({1}), RowLocation({1, 0}), RowLocation({1, 0, 1, 2}),
    });

    TS_ASSERT(!findRootNodes(region).is_initialized());
  }


  void testFindRootsForBigTree() {
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

    auto expectedRoots = std::vector<RowLocation>({
        RowLocation({0}),
        RowLocation({1}),
        RowLocation({2}),
        RowLocation({3})
    });

    auto roots = findRootNodes(region).value();
    sortBoth(expectedRoots, roots);
    TS_ASSERT_EQUALS(expectedRoots, roots);
  }
};

#endif // MANTID_MANTIDWIDGETS_ROWLOCATIONTEST_H
