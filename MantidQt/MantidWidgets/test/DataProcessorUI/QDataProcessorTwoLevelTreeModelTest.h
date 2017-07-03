#ifndef MANTID_MANTIDWIDGETS_QDATAPROCESSORTWOLEVELTREEMODELTEST_H
#define MANTID_MANTIDWIDGETS_QDATAPROCESSORTWOLEVELTREEMODELTEST_H

#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidQtMantidWidgets/DataProcessorUI/QDataProcessorTwoLevelTreeModel.h"
#include <cxxtest/TestSuite.h>

using namespace MantidQt::MantidWidgets;
using namespace Mantid::API;

class QDataProcessorTwoLevelTreeModelTest : public CxxTest::TestSuite {

public:
  // Constructor (initializes whitelist)
  QDataProcessorTwoLevelTreeModelTest() {
    m_whitelist.addElement("Column1", "Property1", "Description1");
    m_whitelist.addElement("Column2", "Property2", "Description2");
  }

  ITableWorkspace_sptr oneRowTable() {
    ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();
    ws->addColumn("str", "Group");
    ws->addColumn("str", "Column1");
    ws->addColumn("str", "Column2");

    TableRow row = ws->appendRow();
    row << "group_0"
        << "row_00"
        << "row_01";

    return ws;
  }

  ITableWorkspace_sptr fourRowTable() {
    ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();
    ws->addColumn("str", "Group");
    ws->addColumn("str", "Column1");
    ws->addColumn("str", "Column2");

    TableRow row = ws->appendRow();
    row << "group0"
        << "group0_row0_col0"
        << "group0_row0_col1";
    row = ws->appendRow();
    row << "group0"
        << "group0_row1_col0"
        << "group0_row1_col1";
    row = ws->appendRow();
    row << "group1"
        << "group1_row0_col0"
        << "group1_row0_col1";
    row = ws->appendRow();
    row << "group1"
        << "group1_row1_col0"
        << "group1_row1_col1";
    return ws;
  }

  ITableWorkspace_sptr unsortedFourRowTable() {
    // A table workspace where rows belonging to the same group are
    // non-consecutive
    ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();
    ws->addColumn("str", "Group");
    ws->addColumn("str", "Column1");
    ws->addColumn("str", "Column2");

    TableRow row = ws->appendRow();
    row << "group0"
        << "group0_row0_col0"
        << "group0_row0_col1";
    row = ws->appendRow();
    row << "group1"
        << "group1_row0_col0"
        << "group1_row0_col1";
    row = ws->appendRow();
    row << "group0"
        << "group0_row1_col0"
        << "group0_row1_col1";
    row = ws->appendRow();
    row << "group1"
        << "group1_row1_col0"
        << "group1_row1_col1";
    return ws;
  }

  void testBadTableWorkspace() {
    auto ws = oneRowTable();

    ws->removeColumn("Group");
    TS_ASSERT_THROWS(QDataProcessorTwoLevelTreeModel(ws, m_whitelist),
                     std::invalid_argument);

    ws->addColumn("str", "Group1");
    ws->addColumn("str", "Group2");
    TS_ASSERT_THROWS(QDataProcessorTwoLevelTreeModel(ws, m_whitelist),
                     std::invalid_argument);
  }

  void testConstructorOneRowTable() {
    auto ws = oneRowTable();
    QDataProcessorTwoLevelTreeModel model(ws, m_whitelist);

    // One group
    TS_ASSERT_EQUALS(model.rowCount(), 1);
    // One row
    TS_ASSERT_EQUALS(model.rowCount(model.index(0, 0)), 1);

    // Test data
    // Group name
    TS_ASSERT_EQUALS(model.data(model.index(0, 0)).toString().toStdString(),
                     "group_0");
    // Data in row
    TS_ASSERT_EQUALS(model.data(model.index(0, 0, model.index(0, 0)))
                         .toString()
                         .toStdString(),
                     "row_00");
    TS_ASSERT_EQUALS(model.data(model.index(0, 1, model.index(0, 0)))
                         .toString()
                         .toStdString(),
                     "row_01");

    // Header data
    TS_ASSERT_EQUALS(model.headerData(0, Qt::Horizontal, Qt::DisplayRole),
                     "Column1");
    TS_ASSERT_EQUALS(model.headerData(1, Qt::Horizontal, Qt::DisplayRole),
                     "Column2");
    TS_ASSERT_EQUALS(model.headerData(0, Qt::Horizontal, Qt::WhatsThisRole),
                     "Description1");
    TS_ASSERT_EQUALS(model.headerData(1, Qt::Horizontal, Qt::WhatsThisRole),
                     "Description2");
  }

  void testConstructorFourRowTable() {
    auto ws = fourRowTable();
    QDataProcessorTwoLevelTreeModel model(ws, m_whitelist);

    // TWo groups
    TS_ASSERT_EQUALS(model.rowCount(), 2);
    // Two rows each
    TS_ASSERT_EQUALS(model.rowCount(model.index(0, 0)), 2);
    TS_ASSERT_EQUALS(model.rowCount(model.index(1, 0)), 2);

    // Test data
    // Group names
    TS_ASSERT_EQUALS(model.data(model.index(0, 0)).toString().toStdString(),
                     "group0");
    TS_ASSERT_EQUALS(model.data(model.index(1, 0)).toString().toStdString(),
                     "group1");
    // Data in rows
    TS_ASSERT_EQUALS(model.data(model.index(0, 0, model.index(0, 0)))
                         .toString()
                         .toStdString(),
                     "group0_row0_col0");
    TS_ASSERT_EQUALS(model.data(model.index(0, 1, model.index(0, 0)))
                         .toString()
                         .toStdString(),
                     "group0_row0_col1");
    TS_ASSERT_EQUALS(model.data(model.index(1, 0, model.index(0, 0)))
                         .toString()
                         .toStdString(),
                     "group0_row1_col0");
    TS_ASSERT_EQUALS(model.data(model.index(1, 1, model.index(0, 0)))
                         .toString()
                         .toStdString(),
                     "group0_row1_col1");
    TS_ASSERT_EQUALS(model.data(model.index(0, 0, model.index(1, 0)))
                         .toString()
                         .toStdString(),
                     "group1_row0_col0");
    TS_ASSERT_EQUALS(model.data(model.index(0, 1, model.index(1, 0)))
                         .toString()
                         .toStdString(),
                     "group1_row0_col1");
    TS_ASSERT_EQUALS(model.data(model.index(1, 0, model.index(1, 0)))
                         .toString()
                         .toStdString(),
                     "group1_row1_col0");
    TS_ASSERT_EQUALS(model.data(model.index(1, 1, model.index(1, 0)))
                         .toString()
                         .toStdString(),
                     "group1_row1_col1");
  }

  void testColumnCount() {
    auto ws = oneRowTable();
    QDataProcessorTwoLevelTreeModel model(ws, m_whitelist);
    TS_ASSERT_EQUALS(model.columnCount(), m_whitelist.size());
  }

  void testIndex() {
    auto ws = fourRowTable();
    QDataProcessorTwoLevelTreeModel model(ws, m_whitelist);

    // Group indices
    TS_ASSERT_EQUALS(model.index(0, 0).row(), 0);
    TS_ASSERT_EQUALS(model.index(1, 0).row(), 1);

    // Row indices
    TS_ASSERT_EQUALS(model.index(0, 0, model.index(0, 0)).row(), 0);
    TS_ASSERT_EQUALS(model.index(1, 0, model.index(0, 0)).row(), 1);
    TS_ASSERT_EQUALS(model.index(0, 0, model.index(1, 0)).row(), 0);
    TS_ASSERT_EQUALS(model.index(1, 0, model.index(1, 0)).row(), 1);
  }

  void testParent() {
    auto ws = fourRowTable();
    QDataProcessorTwoLevelTreeModel model(ws, m_whitelist);

    // Group parent
    TS_ASSERT_EQUALS(model.parent(model.index(0, 0)), QModelIndex());
    TS_ASSERT_EQUALS(model.parent(model.index(1, 0)), QModelIndex());

    // Row parent
    TS_ASSERT_EQUALS(model.parent(model.index(0, 0, model.index(0, 0))),
                     model.index(0, 0));
    TS_ASSERT_EQUALS(model.parent(model.index(1, 0, model.index(0, 0))),
                     model.index(0, 0));
    TS_ASSERT_EQUALS(model.parent(model.index(0, 0, model.index(1, 0))),
                     model.index(1, 0));
    TS_ASSERT_EQUALS(model.parent(model.index(1, 0, model.index(1, 0))),
                     model.index(1, 0));
  }

  void testSetData() {
    auto ws = fourRowTable();
    QDataProcessorTwoLevelTreeModel model(ws, m_whitelist);

    // Rename groups
    model.setData(model.index(0, 0), "new_group_0");
    model.setData(model.index(1, 0), "new_group_1");
    TS_ASSERT_EQUALS(model.data(model.index(0, 0)).toString().toStdString(),
                     "new_group_0");
    TS_ASSERT_EQUALS(model.data(model.index(1, 0)).toString().toStdString(),
                     "new_group_1");

    // Update some cells with new data
    model.setData(model.index(0, 0, model.index(0, 0)), "new_value1");
    model.setData(model.index(1, 1, model.index(0, 0)), "new_value2");
    model.setData(model.index(1, 1, model.index(1, 0)), "new_value3");

    // Test all data in model
    // First group
    TS_ASSERT_EQUALS(model.data(model.index(0, 0, model.index(0, 0)))
                         .toString()
                         .toStdString(),
                     "new_value1");
    TS_ASSERT_EQUALS(model.data(model.index(0, 1, model.index(0, 0)))
                         .toString()
                         .toStdString(),
                     "group0_row0_col1");
    TS_ASSERT_EQUALS(model.data(model.index(1, 0, model.index(0, 0)))
                         .toString()
                         .toStdString(),
                     "group0_row1_col0");
    TS_ASSERT_EQUALS(model.data(model.index(1, 1, model.index(0, 0)))
                         .toString()
                         .toStdString(),
                     "new_value2");
    // Second group
    TS_ASSERT_EQUALS(model.data(model.index(0, 0, model.index(1, 0)))
                         .toString()
                         .toStdString(),
                     "group1_row0_col0");
    TS_ASSERT_EQUALS(model.data(model.index(0, 1, model.index(1, 0)))
                         .toString()
                         .toStdString(),
                     "group1_row0_col1");
    TS_ASSERT_EQUALS(model.data(model.index(1, 0, model.index(1, 0)))
                         .toString()
                         .toStdString(),
                     "group1_row1_col0");
    TS_ASSERT_EQUALS(model.data(model.index(1, 1, model.index(1, 0)))
                         .toString()
                         .toStdString(),
                     "new_value3");
  }

  void testInsertRowsOneRowTable() {
    auto ws = oneRowTable();
    QDataProcessorTwoLevelTreeModel model(ws, m_whitelist);

    // Insert rows

    // Invalid position
    TS_ASSERT_EQUALS(model.insertRows(2, 1, model.index(0, 0)), false);
    // Tree dimensions didn't change
    TS_ASSERT_EQUALS(model.rowCount(model.index(0, 0)), 1);

    // Insert after existing row
    TS_ASSERT_EQUALS(model.insertRows(1, 1, model.index(0, 0)), true);
    // There's one extra row
    TS_ASSERT_EQUALS(model.rowCount(model.index(0, 0)), 2);

    // Insert two rows at the beginning of the group
    TS_ASSERT_EQUALS(model.insertRows(0, 2, model.index(0, 0)), true);
    // There are two extra row
    TS_ASSERT_EQUALS(model.rowCount(model.index(0, 0)), 4);
  }

  void testInsertGroupsOneRowTable() {
    auto ws = oneRowTable();
    QDataProcessorTwoLevelTreeModel model(ws, m_whitelist);

    // Insert groups

    // Invalid position
    TS_ASSERT_EQUALS(model.insertRows(20, 1), false);
    // Tree dimensions didn't change
    TS_ASSERT_EQUALS(model.rowCount(), 1);

    // Insert group after existing group
    TS_ASSERT_EQUALS(model.insertRows(1, 1), true);
    // Tree dimensions changed
    // There are two groups
    TS_ASSERT_EQUALS(model.rowCount(), 2);
    // First group didn't change
    TS_ASSERT_EQUALS(model.rowCount(model.index(0, 0)), 1);
    // New group has one row
    TS_ASSERT_EQUALS(model.rowCount(model.index(1, 0)), 1);

    // Insert three groups at the begining
    TS_ASSERT_EQUALS(model.insertRows(0, 3), true);
    // Tree dimensions changed
    // There are two groups
    TS_ASSERT_EQUALS(model.rowCount(), 5);
    // First three groups have one row
    TS_ASSERT_EQUALS(model.rowCount(model.index(0, 0)), 1);
    TS_ASSERT_EQUALS(model.rowCount(model.index(1, 0)), 1);
    TS_ASSERT_EQUALS(model.rowCount(model.index(2, 0)), 1);
    // Fourth group has four rows
    TS_ASSERT_EQUALS(model.rowCount(model.index(3, 0)), 1);
    // Fifth group has one row
    TS_ASSERT_EQUALS(model.rowCount(model.index(4, 0)), 1);
  }

  void testRemoveRowsOneRowTable() {
    auto ws = oneRowTable();
    QDataProcessorTwoLevelTreeModel model(ws, m_whitelist);

    // Remove the only row, this should remove the group
    TS_ASSERT_EQUALS(model.removeRows(0, 1, model.index(0, 0)), true);
    TS_ASSERT_EQUALS(model.rowCount(), 0);

    // We should be able to add new groups back
    TS_ASSERT_EQUALS(model.insertRows(0, 1), true);
    TS_ASSERT_EQUALS(model.rowCount(), 1);
  }

  void testRemoveGroupsFourRowTable() {
    auto ws = fourRowTable();
    QDataProcessorTwoLevelTreeModel model(ws, m_whitelist);

    // Non-existing group
    TS_ASSERT_EQUALS(model.removeRows(10, 1), false);
    TS_ASSERT_EQUALS(model.rowCount(), 2);
    TS_ASSERT_EQUALS(model.rowCount(model.index(0, 0)), 2);
    TS_ASSERT_EQUALS(model.rowCount(model.index(1, 0)), 2);

    // More groups than current number of groups
    TS_ASSERT_EQUALS(model.removeRows(1, 5), false);
    TS_ASSERT_EQUALS(model.rowCount(), 2);
    TS_ASSERT_EQUALS(model.rowCount(model.index(0, 0)), 2);
    TS_ASSERT_EQUALS(model.rowCount(model.index(1, 0)), 2);

    // Remove last group
    TS_ASSERT_EQUALS(model.removeRows(1, 1), true);
    TS_ASSERT_EQUALS(model.rowCount(), 1);
    TS_ASSERT_EQUALS(model.rowCount(model.index(0, 0)), 2);
  }

  void testRemoveRowsFourRowTable() {
    auto ws = fourRowTable();
    QDataProcessorTwoLevelTreeModel model(ws, m_whitelist);

    // Non-existing row in first group
    TS_ASSERT_EQUALS(model.removeRows(10, 1, model.index(0, 1)), false);
    TS_ASSERT_EQUALS(model.removeRows(-1, 1, model.index(0, 1)), false);
    TS_ASSERT_EQUALS(model.rowCount(), 2);
    TS_ASSERT_EQUALS(model.rowCount(model.index(0, 0)), 2);
    TS_ASSERT_EQUALS(model.rowCount(model.index(1, 0)), 2);

    // More rows than current number of rows
    TS_ASSERT_EQUALS(model.removeRows(1, 50, model.index(1, 0)), false);
    TS_ASSERT_EQUALS(model.rowCount(), 2);
    TS_ASSERT_EQUALS(model.rowCount(model.index(0, 0)), 2);
    TS_ASSERT_EQUALS(model.rowCount(model.index(1, 0)), 2);

    // Remove last row in second group
    TS_ASSERT_EQUALS(model.removeRows(1, 1, model.index(1, 0)), true);
    TS_ASSERT_EQUALS(model.rowCount(), 2);
    TS_ASSERT_EQUALS(model.rowCount(model.index(0, 0)), 2);
    TS_ASSERT_EQUALS(model.rowCount(model.index(1, 0)), 1);

    // Test tree data
    // Groups
    TS_ASSERT_EQUALS(model.data(model.index(0, 0)).toString().toStdString(),
                     "group0");
    TS_ASSERT_EQUALS(model.data(model.index(1, 0)).toString().toStdString(),
                     "group1");
    // Rows in first group
    TS_ASSERT_EQUALS(model.data(model.index(0, 0, model.index(0, 0)))
                         .toString()
                         .toStdString(),
                     "group0_row0_col0");
    TS_ASSERT_EQUALS(model.data(model.index(0, 1, model.index(0, 0)))
                         .toString()
                         .toStdString(),
                     "group0_row0_col1");
    TS_ASSERT_EQUALS(model.data(model.index(1, 0, model.index(0, 0)))
                         .toString()
                         .toStdString(),
                     "group0_row1_col0");
    TS_ASSERT_EQUALS(model.data(model.index(1, 1, model.index(0, 0)))
                         .toString()
                         .toStdString(),
                     "group0_row1_col1");
    // Rows in second group
    TS_ASSERT_EQUALS(model.data(model.index(0, 0, model.index(1, 0)))
                         .toString()
                         .toStdString(),
                     "group1_row0_col0");
    TS_ASSERT_EQUALS(model.data(model.index(0, 1, model.index(1, 0)))
                         .toString()
                         .toStdString(),
                     "group1_row0_col1");
  }

  void testRemoveRowsFourRowTableTwoGroups() {

    // Create the table ws
    ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();
    ws->addColumn("str", "Group");
    ws->addColumn("str", "Column1");
    ws->addColumn("str", "Column2");

    TableRow row = ws->appendRow();
    row << "1"
        << "13462"
        << "2.3";
    row = ws->appendRow();
    row << "2"
        << "13470"
        << "2.3";
    row = ws->appendRow();
    row << "3"
        << "13460"
        << "0.7";
    row = ws->appendRow();
    row << "3"
        << "13469"
        << "0.7";

    QDataProcessorTwoLevelTreeModel model(ws, m_whitelist);

    // Delete second row
    TS_ASSERT_EQUALS(model.removeRows(0, 1, model.index(1, 0)), true);

    // Test tree data
    // Groups
    TS_ASSERT_EQUALS(model.rowCount(), 2);
    TS_ASSERT_EQUALS(model.rowCount(model.index(0, 0)), 1);
    TS_ASSERT_EQUALS(model.rowCount(model.index(1, 0)), 2);

    TS_ASSERT_EQUALS(model.data(model.index(0, 0, model.index(0, 0)))
                         .toString()
                         .toStdString(),
                     "13462");
    TS_ASSERT_EQUALS(model.data(model.index(0, 1, model.index(0, 0)))
                         .toString()
                         .toStdString(),
                     "2.3");
    TS_ASSERT_EQUALS(model.data(model.index(0, 0, model.index(1, 0)))
                         .toString()
                         .toStdString(),
                     "13460");
    TS_ASSERT_EQUALS(model.data(model.index(0, 1, model.index(1, 0)))
                         .toString()
                         .toStdString(),
                     "0.7");
    TS_ASSERT_EQUALS(model.data(model.index(1, 0, model.index(1, 0)))
                         .toString()
                         .toStdString(),
                     "13469");
    TS_ASSERT_EQUALS(model.data(model.index(1, 1, model.index(1, 0)))
                         .toString()
                         .toStdString(),
                     "0.7");
  }

  void testRemoveRowUnsortedTable() {
    // Create a table ws
    ITableWorkspace_sptr ws = unsortedFourRowTable();
    QDataProcessorTwoLevelTreeModel model(ws, m_whitelist);

    // Delete second row
    TS_ASSERT_EQUALS(model.removeRows(1, 1, model.index(0, 0)), true);
    TS_ASSERT_THROWS_NOTHING(model.data(model.index(1, 0, model.index(1, 0))));

    // Test remaining values
    TS_ASSERT_EQUALS(model.data(model.index(0, 0)), "group0");
    TS_ASSERT_EQUALS(model.data(model.index(1, 0)), "group1");
    TS_ASSERT_EQUALS(model.data(model.index(0, 0, model.index(0, 0))),
                     "group0_row0_col0");
    TS_ASSERT_EQUALS(model.data(model.index(0, 0, model.index(1, 0))),
                     "group1_row0_col0");
    TS_ASSERT_EQUALS(model.data(model.index(1, 0, model.index(1, 0))),
                     "group1_row1_col0");
  }

  void testRemoveRowsUnsortedTable() {
    // Create a table ws
    ITableWorkspace_sptr ws = unsortedFourRowTable();
    QDataProcessorTwoLevelTreeModel model(ws, m_whitelist);

    // Delete two consecutive rows belonging to second group
    TS_ASSERT_EQUALS(model.removeRows(0, 2, model.index(1, 0)), true);
    TS_ASSERT_THROWS_NOTHING(model.data(model.index(0, 0, model.index(0, 0))));
    TS_ASSERT_THROWS_NOTHING(model.data(model.index(1, 0, model.index(0, 0))));

    // Test remaining values
    TS_ASSERT_EQUALS(model.data(model.index(0, 0)), "group0");
    TS_ASSERT_EQUALS(model.data(model.index(0, 0, model.index(0, 0))),
                     "group0_row0_col0");
    TS_ASSERT_EQUALS(model.data(model.index(1, 0, model.index(0, 0))),
                     "group0_row1_col0");
  }

  void testRemoveGroupUnsortedTable() {
    // Create a table ws
    ITableWorkspace_sptr ws = unsortedFourRowTable();
    QDataProcessorTwoLevelTreeModel model(ws, m_whitelist);

    // Delete second group
    TS_ASSERT_EQUALS(model.removeRows(1, 1), true);
    TS_ASSERT_THROWS_NOTHING(model.data(model.index(1, 0, model.index(0, 0))));

    // Test remaining values
    TS_ASSERT_EQUALS(model.rowCount(), 1);
    TS_ASSERT_EQUALS(model.data(model.index(0, 0)), "group0");
    TS_ASSERT_EQUALS(model.data(model.index(0, 0, model.index(0, 0))),
                     "group0_row0_col0");
    TS_ASSERT_EQUALS(model.data(model.index(1, 0, model.index(0, 0))),
                     "group0_row1_col0");
  }

  void testRemoveGroupsUnsortedTable() {
    // Create a table ws
    ITableWorkspace_sptr ws = unsortedFourRowTable();
    // Add an extra group
    TableRow row = ws->appendRow();
    row << "group2"
        << "group2_row0_col0"
        << "group2_row0_col1";
    row = ws->appendRow();
    row << "group2"
        << "group2_row1_col0"
        << "group2_row1_col1";

    QDataProcessorTwoLevelTreeModel model(ws, m_whitelist);

    // Delete second and third groups
    TS_ASSERT_EQUALS(model.removeRows(1, 2), true);
    TS_ASSERT_THROWS_NOTHING(model.data(model.index(0, 0, model.index(0, 0))));
    TS_ASSERT_THROWS_NOTHING(model.data(model.index(1, 0, model.index(0, 0))));

    // Test remaining values
    TS_ASSERT_EQUALS(model.rowCount(), 1);
    TS_ASSERT_EQUALS(model.data(model.index(0, 0)), "group0");
    TS_ASSERT_EQUALS(model.data(model.index(0, 0, model.index(0, 0))),
                     "group0_row0_col0");
    TS_ASSERT_EQUALS(model.data(model.index(1, 0, model.index(0, 0))),
                     "group0_row1_col0");
  }

  void testUnsortedTableGetsSorted() {

    // Create an unsorted table ws
    ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();
    ws->addColumn("str", "Group");
    ws->addColumn("str", "Column1");
    ws->addColumn("str", "Column2");

    TableRow row = ws->appendRow();
    row << "3"
        << "13462"
        << "2.3";
    row = ws->appendRow();
    row << "2"
        << "13470"
        << "2.3";
    row = ws->appendRow();
    row << "0"
        << "13463"
        << "0.7";
    row = ws->appendRow();
    row << "4"
        << "13469"
        << "0.7";
    row = ws->appendRow();
    row << "0"
        << "13460"
        << "0.7";

    QDataProcessorTwoLevelTreeModel model(ws, m_whitelist);
    ITableWorkspace_sptr ws_model = model.getTableWorkspace();

    TS_ASSERT_EQUALS(ws_model->rowCount(), 5);
    TS_ASSERT_EQUALS(ws_model->String(0, 0), "0");
    TS_ASSERT_EQUALS(ws_model->String(1, 0), "0");
    TS_ASSERT_EQUALS(ws_model->String(2, 0), "2");
    TS_ASSERT_EQUALS(ws_model->String(3, 0), "3");
    TS_ASSERT_EQUALS(ws_model->String(4, 0), "4");
    TS_ASSERT_EQUALS(ws_model->String(0, 1), "13463");
    TS_ASSERT_EQUALS(ws_model->String(1, 1), "13460");
    TS_ASSERT_EQUALS(ws_model->String(2, 1), "13470");
    TS_ASSERT_EQUALS(ws_model->String(3, 1), "13462");
    TS_ASSERT_EQUALS(ws_model->String(4, 1), "13469");
  }

  void testCountRowsOfNonexistentGroup() {

    QDataProcessorTwoLevelTreeModel model(oneRowTable(), m_whitelist);

    TS_ASSERT_THROWS_NOTHING(model.rowCount(model.index(1, 0)));
  }

private:
  DataProcessorWhiteList m_whitelist;
};

#endif /* MANTID_MANTIDWIDGETS_QDATAPROCESSORTWOLEVELTREEMODELTEST_H */
