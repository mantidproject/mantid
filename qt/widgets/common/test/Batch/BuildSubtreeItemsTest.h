#ifndef MANTID_MANTIDWIDGETS_BUILDSUBTREETEST_H
#define MANTID_MANTIDWIDGETS_BUILDSUBTREETEST_H

#include "MantidKernel/make_unique.h"
#include "MantidQtWidgets/Common/Batch/BuildSubtreeItems.h"
#include <algorithm>
#include <cxxtest/TestSuite.h>
#include <gtest/gtest.h>
#include <vector>
#include <QStandardItem>

using namespace MantidQt::MantidWidgets;
using namespace MantidQt::MantidWidgets::Batch;
using namespace testing;

class BuildSubtreeItemsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BuildSubtreeItemsTest *createSuite() { return new BuildSubtreeItemsTest; }
  static void destroySuite(BuildSubtreeItemsTest *suite) { delete suite; }

  std::unique_ptr<QStandardItemModel> emptyModel() const {
    return Mantid::Kernel::make_unique<QStandardItemModel>();
  }

  Cell cell(std::string const &text) const { return Cell(text); }

  template <typename... Args>
  std::vector<Cell> cells(Args const &... cellText) const {
    return std::vector<Cell>({cell(cellText)...});
  }

  void testBuildEmptySubtree() {
    auto build = BuildSubtreeItems();
    auto positionRelativeToMainTree = RowLocation();

    auto subtree = Subtree();
    auto rootItem = Mantid::Kernel::make_unique<QStandardItem>();

    build(rootItem.get(), positionRelativeToMainTree, 0, subtree);

    TS_ASSERT(rootItem->rowCount() == 0);
  }

  void testBuildSubtreeItemsWithRootOnly() {
    auto build = BuildSubtreeItems();
    auto positionRelativeToMainTree = RowLocation();

    auto subtree = Subtree({{RowLocation(), cells("Root")}});
    auto rootItem = Mantid::Kernel::make_unique<QStandardItem>();

    build(rootItem.get(), positionRelativeToMainTree, 0, subtree);

    TS_ASSERT(rootItem->rowCount() == 1);
    TS_ASSERT_EQUALS(rootItem->child(0)->text(), "Root");
  }

  void testBuildSubtreeItemsWithRootAndSingleChild() {
    auto build = BuildSubtreeItems();
    auto positionRelativeToMainTree = RowLocation();

    auto subtree = Subtree({Row(RowLocation(), cells("Root")),
                            Row(RowLocation({0}), cells("Child"))});

    auto invisibleRootItem = Mantid::Kernel::make_unique<QStandardItem>();
    build(invisibleRootItem.get(), positionRelativeToMainTree, 0, subtree);

    TS_ASSERT_EQUALS(invisibleRootItem->rowCount(), 1);
    auto *subtreeRootItem = invisibleRootItem->child(0);
    TS_ASSERT_EQUALS(subtreeRootItem->text(), "Root");

    TS_ASSERT_EQUALS(subtreeRootItem->rowCount(), 1);
    auto *childItem = subtreeRootItem->child(0);
    TS_ASSERT_EQUALS(childItem->text(), "Child");
  }

  void testBuildSubtreeItemsWithRootAndTwoChildren() {
    auto build = BuildSubtreeItems();
    auto positionRelativeToMainTree = RowLocation();

    auto subtree = Subtree({Row(RowLocation(), cells("Root")),
                            Row(RowLocation({0}), cells("Child 1")),
                            Row(RowLocation({1}), cells("Child 2"))});

    auto invisibleRootItem = Mantid::Kernel::make_unique<QStandardItem>();
    build(invisibleRootItem.get(), positionRelativeToMainTree, 0, subtree);

    TS_ASSERT_EQUALS(invisibleRootItem->rowCount(), 1);
    auto *subtreeRootItem = invisibleRootItem->child(0);
    TS_ASSERT_EQUALS(subtreeRootItem->text(), "Root");

    TS_ASSERT_EQUALS(subtreeRootItem->rowCount(), 2);

    auto *firstChildItem = subtreeRootItem->child(0);
    TS_ASSERT_EQUALS(firstChildItem->text(), "Child 1");

    auto *secondChildItem = subtreeRootItem->child(1);
    TS_ASSERT_EQUALS(secondChildItem->text(), "Child 2");
  }
};
#endif
