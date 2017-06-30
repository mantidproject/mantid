#ifndef MANTID_MANTIDWIDGETS_QDATAPROCESSORONELEVELTREEMODELTEST_H
#define MANTID_MANTIDWIDGETS_QDATAPROCESSORONELEVELTREEMODELTEST_H

#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidQtMantidWidgets/DataProcessorUI/QDataProcessorOneLevelTreeModel.h"
#include <cxxtest/TestSuite.h>

using namespace MantidQt::MantidWidgets;
using namespace Mantid::API;

class QDataProcessorOneLevelTreeModelTest : public CxxTest::TestSuite {

public:
  // Create a white list
  QDataProcessorOneLevelTreeModelTest() {
    m_whitelist.addElement("Column1", "Property1", "Description1");
    m_whitelist.addElement("Column2", "Property2", "Description2");
  }

  // Create a table ws with one row
  ITableWorkspace_sptr oneRowTable() {
    ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();
    ws->addColumn("str", "Column1");
    ws->addColumn("str", "Column2");

    TableRow row = ws->appendRow();
    row << "row0_col0"
        << "row0_col1";

    return ws;
  }

  // Create a table ws with four rows
  ITableWorkspace_sptr fourRowTable() {
    ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();
    ws->addColumn("str", "Column1");
    ws->addColumn("str", "Column2");

    TableRow row = ws->appendRow();
    row << "row0_col0"
        << "row0_col1";
    row = ws->appendRow();
    row << "row1_col0"
        << "row1_col1";
    row = ws->appendRow();
    row << "row2_col0"
        << "row2_col1";
    row = ws->appendRow();
    row << "row3_col0"
        << "row3_col1";
    return ws;
  }

  void testBadTableWorkspace() {
    auto ws = oneRowTable();

    ws->addColumn("str", "Group");
    TS_ASSERT_THROWS(QDataProcessorOneLevelTreeModel(ws, m_whitelist),
                     std::invalid_argument);

    ws->addColumn("str", "Group1");
    ws->addColumn("str", "Group2");
    TS_ASSERT_THROWS(QDataProcessorOneLevelTreeModel(ws, m_whitelist),
                     std::invalid_argument);
  }

  void testConstructorOneRowTable() {
    auto ws = oneRowTable();
    QDataProcessorOneLevelTreeModel model(ws, m_whitelist);

    // One row
    TS_ASSERT_EQUALS(model.rowCount(), 1);
    // Two columns
    TS_ASSERT_EQUALS(model.columnCount(), 2);

    // Test data
    TS_ASSERT_EQUALS(model.data(model.index(0, 0)).toString().toStdString(),
                     "row0_col0");
    TS_ASSERT_EQUALS(model.data(model.index(0, 1)).toString().toStdString(),
                     "row0_col1");
    // Header data
    TS_ASSERT_EQUALS(model.headerData(0, Qt::Horizontal, Qt::DisplayRole),
                     "Column1");
    TS_ASSERT_EQUALS(model.headerData(1, Qt::Horizontal, Qt::DisplayRole),
                     "Column2");
  }

  void testConstructorFourRowTable() {
    auto ws = fourRowTable();
    QDataProcessorOneLevelTreeModel model(ws, m_whitelist);

    // Four rows
    TS_ASSERT_EQUALS(model.rowCount(), 4);
    // Two columns
    TS_ASSERT_EQUALS(model.columnCount(), 2);

    // Test data
    TS_ASSERT_EQUALS(model.data(model.index(2, 0)).toString().toStdString(),
                     "row2_col0");
    TS_ASSERT_EQUALS(model.data(model.index(2, 1)).toString().toStdString(),
                     "row2_col1");
    TS_ASSERT_EQUALS(model.data(model.index(3, 0)).toString().toStdString(),
                     "row3_col0");
    TS_ASSERT_EQUALS(model.data(model.index(3, 1)).toString().toStdString(),
                     "row3_col1");
    // Header data
    TS_ASSERT_EQUALS(model.headerData(0, Qt::Horizontal, Qt::DisplayRole),
                     "Column1");
    TS_ASSERT_EQUALS(model.headerData(1, Qt::Horizontal, Qt::DisplayRole),
                     "Column2");
  }

  void testColumnCount() {
    auto ws = oneRowTable();
    QDataProcessorOneLevelTreeModel model(ws, m_whitelist);
    TS_ASSERT_EQUALS(model.columnCount(), m_whitelist.size());
  }

  void testIndex() {
    auto ws = fourRowTable();
    QDataProcessorOneLevelTreeModel model(ws, m_whitelist);

    TS_ASSERT_EQUALS(model.index(0, 0).row(), 0);
    TS_ASSERT_EQUALS(model.index(1, 0).row(), 1);
    TS_ASSERT_EQUALS(model.index(2, 0).row(), 2);
    TS_ASSERT_EQUALS(model.index(3, 0).row(), 3);
  }

  void testParent() {
    auto ws = fourRowTable();
    QDataProcessorOneLevelTreeModel model(ws, m_whitelist);

    TS_ASSERT_EQUALS(model.parent(model.index(0, 0)), QModelIndex());
    TS_ASSERT_EQUALS(model.parent(model.index(1, 0)), QModelIndex());
    TS_ASSERT_EQUALS(model.parent(model.index(2, 0)), QModelIndex());
    TS_ASSERT_EQUALS(model.parent(model.index(3, 0)), QModelIndex());
  }

  void testSetData() {
    auto ws = fourRowTable();
    QDataProcessorOneLevelTreeModel model(ws, m_whitelist);

    // Update some cells with new data
    model.setData(model.index(0, 0), "new_value1");
    model.setData(model.index(1, 1), "new_value2");
    model.setData(model.index(2, 1), "new_value3");

    // Test data
    TS_ASSERT_EQUALS(model.data(model.index(0, 0)).toString().toStdString(),
                     "new_value1");
    TS_ASSERT_EQUALS(model.data(model.index(0, 1)).toString().toStdString(),
                     "row0_col1");
    TS_ASSERT_EQUALS(model.data(model.index(1, 0)).toString().toStdString(),
                     "row1_col0");
    TS_ASSERT_EQUALS(model.data(model.index(1, 1)).toString().toStdString(),
                     "new_value2");
    TS_ASSERT_EQUALS(model.data(model.index(2, 0)).toString().toStdString(),
                     "row2_col0");
    TS_ASSERT_EQUALS(model.data(model.index(2, 1)).toString().toStdString(),
                     "new_value3");
    TS_ASSERT_EQUALS(model.data(model.index(3, 0)).toString().toStdString(),
                     "row3_col0");
    TS_ASSERT_EQUALS(model.data(model.index(3, 1)).toString().toStdString(),
                     "row3_col1");
  }

  void testInsertRowsOneRowTable() {
    auto ws = oneRowTable();
    QDataProcessorOneLevelTreeModel model(ws, m_whitelist);

    // Insert rows

    // Invalid position
    TS_ASSERT_EQUALS(model.insertRows(2, 1), false);
    // Tree dimensions didn't change
    TS_ASSERT_EQUALS(model.rowCount(), 1);

    // Insert after existing row
    TS_ASSERT_EQUALS(model.insertRows(1, 1), true);
    // There's one extra row
    TS_ASSERT_EQUALS(model.rowCount(), 2);

    // Insert two rows at the beginning
    TS_ASSERT_EQUALS(model.insertRows(0, 2), true);
    // There are two extra row
    TS_ASSERT_EQUALS(model.rowCount(), 4);
  }

  void testRemoveRowsOneRowTable() {
    auto ws = oneRowTable();
    QDataProcessorOneLevelTreeModel model(ws, m_whitelist);

    // Remove the only row
    TS_ASSERT_EQUALS(model.removeRows(0, 1), true);
    TS_ASSERT_EQUALS(model.rowCount(), 0);

    // We should be able to add new rows back
    TS_ASSERT_EQUALS(model.insertRows(0, 1), true);
    TS_ASSERT_EQUALS(model.rowCount(), 1);
  }

  void testRemoveRowsFourRowTable() {
    auto ws = fourRowTable();
    QDataProcessorOneLevelTreeModel model(ws, m_whitelist);

    // Non-existing row
    TS_ASSERT_EQUALS(model.removeRows(10, 1), false);
    TS_ASSERT_EQUALS(model.removeRows(-1, 1), false);
    TS_ASSERT_EQUALS(model.rowCount(), 4);

    // More rows than current number of rows
    TS_ASSERT_EQUALS(model.removeRows(1, 50), false);
    TS_ASSERT_EQUALS(model.rowCount(), 4);

    // Remove last row
    TS_ASSERT_EQUALS(model.removeRows(3, 1), true);
    TS_ASSERT_EQUALS(model.rowCount(), 3);
  }

  void testHighlightFourRowTable() {
    auto ws = fourRowTable();
    QDataProcessorOneLevelTreeModel model(ws, m_whitelist);

    // Non-existent row
    TS_ASSERT_EQUALS(model.addHighlighted(10), false);
    TS_ASSERT_EQUALS(model.addHighlighted(-1), false);

    // Highlight the 1st and 3rd rows
    TS_ASSERT_EQUALS(model.addHighlighted(0), true);
    TS_ASSERT_EQUALS(model.addHighlighted(2), true);

    // Only 1st and 3rd rows are highlighted
    TS_ASSERT_EQUALS(model.data(model.index(0, 0), Qt::BackgroundRole)
                         .toString()
                         .toStdString(),
                     "#00b300");
    TS_ASSERT_EQUALS(model.data(model.index(1, 0), Qt::BackgroundRole)
                         .toString()
                         .toStdString(),
                     "");
    TS_ASSERT_EQUALS(model.data(model.index(2, 0), Qt::BackgroundRole)
                         .toString()
                         .toStdString(),
                     "#00b300");
    TS_ASSERT_EQUALS(model.data(model.index(3, 0), Qt::BackgroundRole)
                         .toString()
                         .toStdString(),
                     "");
  }

  void testClearHighlightFourRowTable() {
    auto ws = fourRowTable();
    QDataProcessorOneLevelTreeModel model(ws, m_whitelist);

    // Highlight the 1st and 3rd rows
    TS_ASSERT_EQUALS(model.addHighlighted(0), true);
    TS_ASSERT_EQUALS(model.addHighlighted(2), true);

    // Clear all highlighted rows
    model.clearHighlighted();

    // No rows should be highlighted
    TS_ASSERT_EQUALS(model.data(model.index(0, 0), Qt::BackgroundRole)
                         .toString()
                         .toStdString(),
                     "");
    TS_ASSERT_EQUALS(model.data(model.index(1, 0), Qt::BackgroundRole)
                         .toString()
                         .toStdString(),
                     "");
    TS_ASSERT_EQUALS(model.data(model.index(2, 0), Qt::BackgroundRole)
                         .toString()
                         .toStdString(),
                     "");
    TS_ASSERT_EQUALS(model.data(model.index(3, 0), Qt::BackgroundRole)
                         .toString()
                         .toStdString(),
                     "");
  }

private:
  DataProcessorWhiteList m_whitelist;
};

#endif /* MANTID_MANTIDWIDGETS_QDATAPROCESSORONELEVELTREEMODELTEST_H */
