// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/Batch/BuildSubtreeItems.h"
#include "MantidQtWidgets/Common/Batch/QtStandardItemTreeAdapter.h"
#include "MantidQtWidgets/Common/Batch/RowLocationAdapter.h"
#include <QStandardItem>
#include <algorithm>
#include <cxxtest/TestSuite.h>
#include <gtest/gtest.h>
#include <vector>

using namespace MantidQt::MantidWidgets;
using namespace MantidQt::MantidWidgets::Batch;
using namespace testing;

class BuildSubtreeItemsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BuildSubtreeItemsTest *createSuite() { return new BuildSubtreeItemsTest; }
  static void destroySuite(BuildSubtreeItemsTest *suite) { delete suite; }

  std::unique_ptr<QStandardItemModel> emptyModel() const { return std::make_unique<QStandardItemModel>(); }

  QtStandardItemTreeModelAdapter adapt(QStandardItemModel *model) {
    return QtStandardItemTreeModelAdapter(*model, Cell(""));
  }

  Cell cell(std::string const &text) const { return Cell(text); }

  template <typename... Args> std::vector<Cell> cells(Args const &...cellText) const {
    return std::vector<Cell>({cell(cellText)...});
  }

  void testBuildEmptySubtree() {
    auto model = emptyModel();
    auto adaptedModel = adapt(model.get());
    auto build = BuildSubtreeItems(adaptedModel, RowLocationAdapter(*model));
    auto positionRelativeToMainTree = RowLocation();

    auto subtree = Subtree();
    auto rootItem = std::make_unique<QStandardItem>();

    build(positionRelativeToMainTree, 0, subtree);

    TS_ASSERT(rootItem->rowCount() == 0);
  }

  void testBuildSubtreeItemsWithRootOnly() {
    auto model = emptyModel();
    auto adaptedModel = adapt(model.get());
    auto build = BuildSubtreeItems(adaptedModel, RowLocationAdapter(*model));

    auto positionRelativeToMainTree = RowLocation();
    auto subtree = Subtree({{RowLocation(), cells("Root")}});
    build(positionRelativeToMainTree, 0, subtree);

    auto *rootItem = model->invisibleRootItem();
    TS_ASSERT(rootItem->rowCount() == 1);
    TS_ASSERT_EQUALS(rootItem->child(0)->text(), "Root");
  }

  void testBuildSubtreeItemsWithRootAndSingleChild() {
    auto model = emptyModel();
    auto adaptedModel = adapt(model.get());
    auto build = BuildSubtreeItems(adaptedModel, RowLocationAdapter(*model));
    auto positionRelativeToMainTree = RowLocation();

    auto subtree = Subtree({Row(RowLocation(), cells("Root")), Row(RowLocation({0}), cells("Child"))});

    build(positionRelativeToMainTree, 0, subtree);

    auto *invisibleRootItem = model->invisibleRootItem();
    TS_ASSERT_EQUALS(invisibleRootItem->rowCount(), 1);
    auto *subtreeRootItem = invisibleRootItem->child(0);
    TS_ASSERT_EQUALS(subtreeRootItem->text(), "Root");

    TS_ASSERT_EQUALS(subtreeRootItem->rowCount(), 1);
    auto *childItem = subtreeRootItem->child(0);
    TS_ASSERT_EQUALS(childItem->text(), "Child");
  }

  void testBuildSubtreeItemsWithRootAndTwoChildren() {
    auto model = emptyModel();
    auto adaptedModel = adapt(model.get());
    auto build = BuildSubtreeItems(adaptedModel, RowLocationAdapter(*model));
    auto positionRelativeToMainTree = RowLocation();

    auto subtree = Subtree({Row(RowLocation(), cells("Root")), Row(RowLocation({0}), cells("Child 1")),
                            Row(RowLocation({1}), cells("Child 2"))});

    build(positionRelativeToMainTree, 0, subtree);

    auto *invisibleRootItem = model->invisibleRootItem();
    TS_ASSERT_EQUALS(invisibleRootItem->rowCount(), 1);
    auto *subtreeRootItem = invisibleRootItem->child(0);
    TS_ASSERT_EQUALS(subtreeRootItem->text(), "Root");

    TS_ASSERT_EQUALS(subtreeRootItem->rowCount(), 2);

    auto *firstChildItem = subtreeRootItem->child(0);
    TS_ASSERT_EQUALS(firstChildItem->text(), "Child 1");

    auto *secondChildItem = subtreeRootItem->child(1);
    TS_ASSERT_EQUALS(secondChildItem->text(), "Child 2");
  }

  void testBuildSubtreeItemsWithRootAndTwoChildrenWithAChildEach() {
    auto model = emptyModel();
    auto adaptedModel = adapt(model.get());
    auto build = BuildSubtreeItems(adaptedModel, RowLocationAdapter(*model));
    auto positionRelativeToMainTree = RowLocation();

    auto subtree =
        Subtree({Row(RowLocation(), cells("Root")), Row(RowLocation({0}), cells("1st Child")),
                 Row(RowLocation({0, 0}), cells("Child of 1st Child")), Row(RowLocation({1}), cells("2nd Child")),
                 Row(RowLocation({1, 0}), cells("Child of 2nd Child"))});

    build(positionRelativeToMainTree, 0, subtree);

    auto *invisibleRootItem = model->invisibleRootItem();
    TS_ASSERT_EQUALS(invisibleRootItem->rowCount(), 1);
    auto *subtreeRootItem = invisibleRootItem->child(0);
    TS_ASSERT_EQUALS(subtreeRootItem->text(), "Root");
    TS_ASSERT_EQUALS(subtreeRootItem->rowCount(), 2);

    {
      auto *firstChildItem = subtreeRootItem->child(0);
      TS_ASSERT_EQUALS(firstChildItem->text(), "1st Child");
      TS_ASSERT_EQUALS(firstChildItem->rowCount(), 1);

      auto *childOfChildItem = firstChildItem->child(0);
      TS_ASSERT_EQUALS(childOfChildItem->text(), "Child of 1st Child");
      TS_ASSERT_EQUALS(childOfChildItem->rowCount(), 0);
    }

    {
      auto *secondChildItem = subtreeRootItem->child(1);
      TS_ASSERT_EQUALS(secondChildItem->text(), "2nd Child");
      TS_ASSERT_EQUALS(secondChildItem->rowCount(), 1);

      auto *childOfChildItem = secondChildItem->child(0);
      TS_ASSERT_EQUALS(childOfChildItem->text(), "Child of 2nd Child");
      TS_ASSERT_EQUALS(childOfChildItem->rowCount(), 0);
    }
  }
};
