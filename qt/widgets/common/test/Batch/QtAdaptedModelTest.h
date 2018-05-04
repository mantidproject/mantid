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

  QtStandardItemMutableTreeAdapter adapt(QStandardItemModel *model) {
    return QtStandardItemMutableTreeAdapter(*model);
  }

  std::unique_ptr<QStandardItemModel> emptyModel() const {
    return Mantid::Kernel::make_unique<QStandardItemModel>();
  }

  void testInvalidIndexIsRoot() {
    auto model = emptyModel();
    auto adaptedModel = adapt(model.get());

    TS_ASSERT_EQUALS(adaptedModel.rootModelIndex(), QModelIndex());
  }

  void testCanGetRootItemFromRootIndex() {
    auto model = emptyModel();
    auto adaptedModel = adapt(model.get());

    auto rootIndex = QModelIndex();
    TS_ASSERT_EQUALS(adaptedModel.modelItemFromIndex(rootIndex),
                     model->invisibleRootItem());
  }

  void testAppendChildNode() {
    auto model = emptyModel();
    auto adaptedModel = adapt(model.get());

    auto *expectedChildItem = new QStandardItem("Some Dummy Text");
    adaptedModel.appendChildRow(QModelIndex(), {expectedChildItem});

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

    adaptedModel.insertChildRow(QModelIndex(), 1, {newSibling});
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

    auto rootIndex = QModelIndex();
    auto sibling0Index = model->index(/*row=*/0, /*column=*/0, rootIndex);
    auto *newSibling = new QStandardItem("Some Dummy Text");

    adaptedModel.appendSiblingRow(sibling0Index, {newSibling});
    TS_ASSERT_EQUALS(newSibling, model->invisibleRootItem()->child(2));
  }

  void testCanRowTextCorrectForAppendedRow() {
    auto model = emptyModel();
    auto adaptedModel = adapt(model.get());

    auto const firstCellText = QString("First Cell");
    auto *firstCell = new QStandardItem(firstCellText);
    auto const secondCellText = QString("Second Cell");
    auto *secondCell = new QStandardItem(secondCellText);

    auto *rootItem = model->invisibleRootItem();
    rootItem->appendRow({firstCell, secondCell});

    auto rootIndex = QModelIndex();
    auto childRowIndex = model->index(/*row=*/0, /*column=*/0, rootIndex);

    auto rowText = adaptedModel.rowTextFromRow(childRowIndex);

    TS_ASSERT_EQUALS(rowText.size(), 2u);
    TS_ASSERT_EQUALS(firstCellText, QString::fromStdString(rowText[0]));
    TS_ASSERT_EQUALS(secondCellText, QString::fromStdString(rowText[1]));
  }

  void testCanCreateCellsFromStringVector() {
    auto model = emptyModel();
    auto adaptedModel = adapt(model.get());

    auto const rowText =
        std::vector<std::string>({"First Cell", "Second Cell"});
    auto const row = adaptedModel.rowFromRowText(rowText);

    TS_ASSERT_EQUALS(row.size(), 2u);
    TS_ASSERT_EQUALS(rowText[0], row[0]->text().toStdString());
    TS_ASSERT_EQUALS(rowText[1], row[1]->text().toStdString());
  }
};

#endif // MANTID_MANTIDWIDGETS_QTADAPTEDMODELTEST_H
