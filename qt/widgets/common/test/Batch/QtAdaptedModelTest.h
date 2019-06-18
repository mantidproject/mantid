// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MANTIDWIDGETS_QTADAPTEDMODELTEST_H
#define MANTID_MANTIDWIDGETS_QTADAPTEDMODELTEST_H

#include "MantidQtWidgets/Common/Batch/QtStandardItemTreeAdapter.h"
#include <QModelIndex>
#include <QStandardItemModel>
#include <cxxtest/TestSuite.h>
#include <gtest/gtest.h>

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
    return std::make_unique<QStandardItemModel>();
  }

  void testInvalidIndexIsRoot() {
    auto model = emptyModel();
    auto adaptedModel = adapt(model.get());

    TS_ASSERT_EQUALS(adaptedModel.rootIndex().untyped(), QModelIndex());
  }

  void testAppendChildNode() {
    auto model = emptyModel();
    auto adaptedModel = adapt(model.get());

    auto expectedChildCell = Cell("Some Dummy Text");
    adaptedModel.appendChildRow(QModelIndexForMainModel(), {expectedChildCell});

    TS_ASSERT_EQUALS(QString::fromStdString(expectedChildCell.contentText()),
                     model->invisibleRootItem()->child(0)->text());
  }

  void testInsertChildNodeBetweenTwoSiblings() {
    auto model = emptyModel();
    auto adaptedModel = adapt(model.get());

    auto *sibling0 = new QStandardItem("Sibling 0");
    auto *sibling1 = new QStandardItem("Sibling 1");

    auto *rootItem = model->invisibleRootItem();
    rootItem->appendRow(sibling0);
    rootItem->appendRow(sibling1);

    auto newSiblingCell = Cell("Some Dummy Text");

    adaptedModel.insertChildRow(QModelIndexForMainModel(), 1, {newSiblingCell});
    TS_ASSERT_EQUALS(QString::fromStdString(newSiblingCell.contentText()),
                     model->invisibleRootItem()->child(1)->text());
  }

  void testAppendSiblingNodeAfterSiblings() {
    auto model = emptyModel();
    auto adaptedModel = adapt(model.get());

    auto *sibling0 = new QStandardItem("Sibling 0");
    auto *sibling1 = new QStandardItem("Sibling 1");

    auto *rootItem = model->invisibleRootItem();
    rootItem->appendRow(sibling0);
    rootItem->appendRow(sibling1);

    auto rootIndex = QModelIndexForMainModel();
    auto sibling0Index = fromMainModel(
        model->index(/*row=*/0, /*column=*/0, rootIndex.untyped()), *model);
    auto newSiblingCell = Cell("Some Text");

    adaptedModel.appendSiblingRow(sibling0Index, {newSiblingCell});
    TS_ASSERT_EQUALS(QString::fromStdString(newSiblingCell.contentText()),
                     model->invisibleRootItem()->child(2)->text());
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
};

#endif // MANTID_MANTIDWIDGETS_QTADAPTEDMODELTEST_H
