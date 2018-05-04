#ifndef MANTID_MANTIDWIDGETS_QTADAPTEDMODELTEST_H
#define MANTID_MANTIDWIDGETS_QTADAPTEDMODELTEST_H

#include <cxxtest/TestSuite.h>
#include <gtest/gtest.h>
#include <QModelIndex>
#include <QStandardItemModel>
#include "MantidKernel/make_unique.h"
#include "MantidQtWidgets/Common/Batch/QtStandardItemTreeAdapter.h"

using namespace MantidQt::MantidWidgets;
using namespace MantidQt::MantidWidgets::Batch;
using namespace testing;

class QtAdaptedModelTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static QtAdaptedModelTest *createSuite() { return new QtAdaptedModelTest(); }
  static void destroySuite(QtAdaptedModelTest *suite) { delete suite; }

  QtStandardItemTreeModelAdapter adapt(QStandardItemModel *model) {
    return QtStandardItemTreeModelAdapter(*model, Cell(""));
  }

  std::unique_ptr<QStandardItemModel> emptyModel() const {
    return Mantid::Kernel::make_unique<QStandardItemModel>();
  }

  void testInvalidIndexIsRoot() {
    auto model = emptyModel();
    auto adaptedModel = adapt(model.get());

    TS_ASSERT_EQUALS(rootModelIndex(*model).untyped(), QModelIndex());
  }

  void testCanGetRootItemFromRootIndex() {
    auto model = emptyModel();
    auto adaptedModel = adapt(model.get());

    auto rootIndex = QModelIndexForMainModel();
    TS_ASSERT_EQUALS(modelItemFromIndex(*model, rootIndex),
                     model->invisibleRootItem());
  }

  void testAppendChildNode() {
    auto model = emptyModel();
    auto adaptedModel = adapt(model.get());

    auto *expectedChildItem = new QStandardItem("Some Dummy Text");
    adaptedModel.appendChildRow(QModelIndexForMainModel(), {expectedChildItem});

    TS_ASSERT_EQUALS(expectedChildItem, model->invisibleRootItem()->child(0));
  }

  void testInsertChildNodeBetweenTwoSiblings() {
    auto model = emptyModel();
    auto adaptedModel = adapt(model.get());

    auto *sibling0 = new QStandardItem("Sibling 0");
    auto *sibling1 = new QStandardItem("Sibling 1");

    auto *rootItem = model->invisibleRootItem();
    rootItem->appendRow({sibling0});
    rootItem->appendRow({sibling1});

    auto *newSibling = new QStandardItem("Some Dummy Text");

    adaptedModel.insertChildRow(QModelIndexForMainModel(), 1, {newSibling});
    TS_ASSERT_EQUALS(newSibling, model->invisibleRootItem()->child(1));
  }

  void testAppendSiblingNodeAfterSiblings() {
    auto model = emptyModel();
    auto adaptedModel = adapt(model.get());

    auto *sibling0 = new QStandardItem("Sibling 0");
    auto *sibling1 = new QStandardItem("Sibling 1");

    auto *rootItem = model->invisibleRootItem();
    rootItem->appendRow({sibling0});
    rootItem->appendRow({sibling1});

    auto rootIndex = QModelIndexForMainModel();
    auto sibling0Index = fromMainModel(
        model->index(/*row=*/0, /*column=*/0, rootIndex.untyped()), *model);
    auto *newSibling = new QStandardItem("Some Dummy Text");

    adaptedModel.appendSiblingRow(sibling0Index, {newSibling});
    TS_ASSERT_EQUALS(newSibling, model->invisibleRootItem()->child(2));
  }

  void testCellTextCorrectForAppendedRow() {
    auto model = emptyModel();
    auto adaptedModel = adapt(model.get());

    auto const firstCellText = QString("First Cell");
    auto *firstCell = new QStandardItem(firstCellText);
    auto const secondCellText = QString("Second Cell");
    auto *secondCell = new QStandardItem(secondCellText);

    auto *rootItem = model->invisibleRootItem();
    rootItem->appendRow({firstCell, secondCell});

    auto rootIndex = QModelIndexForMainModel();
    auto childRowIndex = fromMainModel(
        model->index(/*row=*/0, /*column=*/0, rootIndex.untyped()), *model);

    auto cells = adaptedModel.cellsAtRow(childRowIndex);

    TS_ASSERT_EQUALS(cells.size(), 2u);
    TS_ASSERT_EQUALS(firstCellText,
                     QString::fromStdString(cells[0].contentText()));
    TS_ASSERT_EQUALS(secondCellText,
                     QString::fromStdString(cells[1].contentText()));
  }

  void testCanCreateRowFromStringVector() {
    auto model = emptyModel();
    auto adaptedModel = adapt(model.get());

    auto const cells =
        std::vector<Cell>({Cell("First Cell"), Cell("Second Cell")});
    auto const cellItems = rowFromCells(cells);

    TS_ASSERT_EQUALS(cellItems.size(), 2u);
    TS_ASSERT_EQUALS(cells[0].contentText(),
                     cellItems[0]->text().toStdString());
    TS_ASSERT_EQUALS(cells[1].contentText(),
                     cellItems[1]->text().toStdString());
  }
};

#endif // MANTID_MANTIDWIDGETS_QTADAPTEDMODELTEST_H
