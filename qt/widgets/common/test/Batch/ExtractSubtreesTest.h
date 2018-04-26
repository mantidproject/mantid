#ifndef MANTID_MANTIDWIDGETS_EXTRACTSUBTREESTEST_H
#define MANTID_MANTIDWIDGETS_EXTRACTSUBTREESTEST_H

#include "MantidQtWidgets/Common/Batch/ExtractSubtrees.h"
#include "MantidQtWidgets/Common/Batch/RowLocation.h"
#include <algorithm>
#include <cxxtest/TestSuite.h>
#include <gtest/gtest.h>
#include <vector>

using namespace MantidQt::MantidWidgets;
using namespace MantidQt::MantidWidgets::Batch;
using namespace testing;

class ExtractSubtreesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ExtractSubtreesTest *createSuite() { return new ExtractSubtreesTest; }
  static void destroySuite(ExtractSubtreesTest *suite) { delete suite; }

  std::string cell(std::string const &text) const { return text; }

  template <typename... Args> Row cells(Args const &... cellText) const {
    return Row({cell(cellText)...});
  }

  void testForSingleLocation() {
    auto extractSubtrees = ExtractSubtrees();
    auto region = std::vector<RowLocation>({RowLocation({1})});
    auto regionData = std::vector<Row>({cells("Root")});

    auto roots = extractSubtrees(region, regionData).value();

    // clang-format off
    auto expectedSubtrees =
        std::vector<Subtree>({
          Subtree({
            {RowLocation(), cells("Root")}
          })
        });
    // clang-format on

    TS_ASSERT_EQUALS(expectedSubtrees, roots);
  }

  void testTwoSiblingsResultsInTwoRoots() {
    auto extractSubtrees = ExtractSubtrees();
    auto region =
        std::vector<RowLocation>({RowLocation({1}), RowLocation({2})});
    auto regionData = std::vector<Row>({cells("Root 1"), cells("Root 2")});

    // clang-format off
    auto expectedSubtrees =
        std::vector<Subtree>({
          Subtree({
            {RowLocation(), cells("Root 1")}
          }),
          Subtree({
            {RowLocation(), cells("Root 2")}
          })
        });
    // clang-format on

    auto roots = extractSubtrees(region, regionData).value();
    TS_ASSERT_EQUALS(expectedSubtrees, roots);
  }

  void testParentAndChildResultsInParent() {
    auto extractSubtrees = ExtractSubtrees();
    auto region =
        std::vector<RowLocation>({RowLocation({1}), RowLocation({1, 2})});
    auto regionData = std::vector<Row>({cells("Root"), cells("Child")});

    // clang-format off
    auto expectedSubtrees =
        std::vector<Subtree>({
          Subtree({
            {RowLocation(), cells("Root")},
            {RowLocation({2}), cells("Child")}
          })
        });
    // clang-format on

    auto roots = extractSubtrees(region, regionData).value();
    TS_ASSERT_EQUALS(expectedSubtrees, roots);
  }

  void testParentWithChildAndSiblingResultsInParentAndSibling() {
    auto extractSubtrees = ExtractSubtrees();
    // clang-format off
    auto region = std::vector<RowLocation>({
      RowLocation({1}),
      RowLocation({1, 0}),
      RowLocation({2})
    });
    // clang-format on
    auto regionData =
        std::vector<Row>({cells("Root 1"), cells("Child"), cells("Root 2")});

    // clang-format off
    auto expectedSubtrees =
        std::vector<Subtree>({
          Subtree({
            {RowLocation(), cells("Root 1")},
            {RowLocation({0}), cells("Child")},
          }),
          Subtree({
            {RowLocation(), cells("Root 2")},
          })
        });
    // clang-format on

    auto roots = extractSubtrees(region, regionData).value();
    TS_ASSERT_EQUALS(expectedSubtrees, roots);
  }

  void testFindsRootOfNonTrivialTree() {
    auto extractSubtrees = ExtractSubtrees();
    // clang-format off
    auto region = std::vector<RowLocation>({
      RowLocation({1}),
      RowLocation({1, 0}),
      RowLocation({1, 0, 1}),
      RowLocation({1, 1})
    });
    auto regionData = std::vector<Row>({
      cells("Root  1"),
      cells("Child 1, 0"),
      cells("Child 1, 0, 1"),
      cells("Child 1, 1")
    });
    // clang-format on

    // clang-format off
    auto expectedSubtrees =
        std::vector<Subtree>({
          Subtree({
            {RowLocation(),       cells("Root  1")},
            {RowLocation({0}),    cells("Child 1, 0")},
            {RowLocation({0, 1}), cells("Child 1, 0, 1")},
            {RowLocation({1}),    cells("Child 1, 1")},
          }),
        });
    // clang-format on

    auto roots = extractSubtrees(region, regionData).value();
    TS_ASSERT_EQUALS(expectedSubtrees, roots);
  }

  void testFailsForLevelGap() {
    auto extractSubtrees = ExtractSubtrees();
    // clang-format off
    auto region = std::vector<RowLocation>({
      RowLocation({1}),
      RowLocation({1, 0}),
      RowLocation({1, 0, 1, 2})
    });
    auto regionData = std::vector<Row>({
      cells("Root  1"),
      cells("Child 1, 0"),
      cells("Child 1, 0, 1, 2"),
    });
    // clang-format on

    TS_ASSERT(!extractSubtrees(region, regionData).is_initialized());
  }

  void testForRealisticTree() {
    auto extractSubtrees = ExtractSubtrees();
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

    auto regionData = std::vector<Row>({
      cells("Root  0"),
      cells("Child 0, 0"),
      cells("Child 0, 1"),
      cells("Root  1"),
      cells("Child 1, 0"),
      cells("Child 1, 0, 0"),
      cells("Child 1, 0, 0, 0"),
      cells("Child 1, 0, 0, 1"),
      cells("Child 1, 0, 0, 2"),
      cells("Child 1, 2"),
      cells("Root  2"),
      cells("Root  3")
    });
    // clang-format on

    // clang-format off
    auto expectedSubtrees =
        std::vector<Subtree>({
          Subtree({
            {RowLocation(),             cells("Root  0")},
            {RowLocation({0}),          cells("Child 0, 0")},
            {RowLocation({1}),          cells("Child 0, 1")}
          }),
          Subtree({
            {RowLocation(),             cells("Root  1")},
            {RowLocation({0}),          cells("Child 1, 0")},
            {RowLocation({0, 0}),       cells("Child 1, 0, 0")},
            {RowLocation({0, 0, 0}),    cells("Child 1, 0, 0, 0")},
            {RowLocation({0, 0, 1}),    cells("Child 1, 0, 0, 1")},
            {RowLocation({0, 0, 2}),    cells("Child 1, 0, 0, 2")},
            {RowLocation({2}),          cells("Child 1, 2")}
          }),
          Subtree({
            {RowLocation(),             cells("Root  2")}
          }),
          Subtree({
            {RowLocation(),             cells("Root  3")}
          })
        });
    // clang-format on

    auto roots = extractSubtrees(region, regionData).value();
    TS_ASSERT_EQUALS(expectedSubtrees, roots);
  }
};
#endif
