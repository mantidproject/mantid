// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MANTIDWIDGETS_QDATAPROCESSORTWOLEVELTREEMODELTEST_H
#define MANTID_MANTIDWIDGETS_QDATAPROCESSORTWOLEVELTREEMODELTEST_H

#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidQtWidgets/Common/DataProcessorUI/QTwoLevelTreeModel.h"
#include <cxxtest/TestSuite.h>

using namespace MantidQt::MantidWidgets;
using namespace MantidQt::MantidWidgets::DataProcessor;
using namespace Mantid::API;

class QTwoLevelTreeModelTest : public CxxTest::TestSuite {
public:
  // This means the constructor isn't called when running other tests
  static QTwoLevelTreeModelTest *createSuite() {
    return new QTwoLevelTreeModelTest();
  }
  static void destroySuite(QTwoLevelTreeModelTest *suite) { delete suite; }

  // Constructor (initializes whitelist)
  QTwoLevelTreeModelTest() {
    m_whitelist.addElement("Column1", "Property1", "Description1");
    m_whitelist.addElement("Column2", "Property2", "Description2");
  }

  WhiteList whitelistWithKeyColumn() {
    WhiteList whitelist;
    whitelist.addElement("Column1", "Property1", "Description1", false, "",
                         true); // key column
    whitelist.addElement("Column2", "Property2", "Description2");
    return whitelist;
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
    TS_ASSERT_THROWS(QTwoLevelTreeModel(ws, m_whitelist),
                     const std::invalid_argument &);

    ws->addColumn("str", "Group1");
    ws->addColumn("str", "Group2");
    TS_ASSERT_THROWS(QTwoLevelTreeModel(ws, m_whitelist),
                     const std::invalid_argument &);
  }

  void testConstructorOneRowTable() {
    auto ws = oneRowTable();
    QTwoLevelTreeModel model(ws, m_whitelist);

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
    QTwoLevelTreeModel model(ws, m_whitelist);

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
    QTwoLevelTreeModel model(ws, m_whitelist);
    TS_ASSERT_EQUALS(model.columnCount(), m_whitelist.size());
  }

  void testIndex() {
    auto ws = fourRowTable();
    QTwoLevelTreeModel model(ws, m_whitelist);

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
    QTwoLevelTreeModel model(ws, m_whitelist);

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
    QTwoLevelTreeModel model(ws, m_whitelist);

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
    QTwoLevelTreeModel model(ws, m_whitelist);

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
    QTwoLevelTreeModel model(ws, m_whitelist);

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
    QTwoLevelTreeModel model(ws, m_whitelist);

    // Remove the only row, this should remove the group
    TS_ASSERT_EQUALS(model.removeRows(0, 1, model.index(0, 0)), true);
    TS_ASSERT_EQUALS(model.rowCount(), 0);

    // We should be able to add new groups back
    TS_ASSERT_EQUALS(model.insertRows(0, 1), true);
    TS_ASSERT_EQUALS(model.rowCount(), 1);
  }

  void testRemoveGroupsFourRowTable() {
    auto ws = fourRowTable();
    QTwoLevelTreeModel model(ws, m_whitelist);

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
    QTwoLevelTreeModel model(ws, m_whitelist);

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

    QTwoLevelTreeModel model(ws, m_whitelist);

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
    QTwoLevelTreeModel model(ws, m_whitelist);

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
    QTwoLevelTreeModel model(ws, m_whitelist);

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
    QTwoLevelTreeModel model(ws, m_whitelist);

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

    QTwoLevelTreeModel model(ws, m_whitelist);

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

    QTwoLevelTreeModel model(ws, m_whitelist);
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

    QTwoLevelTreeModel model(oneRowTable(), m_whitelist);

    TS_ASSERT_THROWS_NOTHING(model.rowCount(model.index(1, 0)));
  }

  void testHighlightTable() {
    auto ws = fourRowTable();
    QTwoLevelTreeModel model(ws, m_whitelist);

    // Non-existent row
    TS_ASSERT_EQUALS(model.setProcessed(true, 10, model.index(0, 0)), false);
    TS_ASSERT_EQUALS(model.setProcessed(true, -1, model.index(0, 0)), false);

    // Non-existent group
    TS_ASSERT_EQUALS(model.setProcessed(true, 10), false);
    TS_ASSERT_EQUALS(model.setProcessed(true, -1), false);

    // Set 1st row of 1st group and 2nd group processed
    TS_ASSERT_EQUALS(model.setProcessed(true, 0, model.index(0, 0)), true);
    TS_ASSERT_EQUALS(model.setProcessed(true, 1), true);

    // Only the 1st row of 1st group and 2nd group should be highlighted
    TS_ASSERT_EQUALS(model.data(model.index(0, 0), Qt::BackgroundRole)
                         .toString()
                         .toStdString(),
                     "");
    TS_ASSERT_EQUALS(
        model.data(model.index(0, 0, model.index(0, 0)), Qt::BackgroundRole)
            .toString()
            .toStdString(),
        Colour::SUCCESS);
    TS_ASSERT_EQUALS(
        model.data(model.index(1, 0, model.index(0, 0)), Qt::BackgroundRole)
            .toString()
            .toStdString(),
        "");
    TS_ASSERT_EQUALS(model.data(model.index(1, 0), Qt::BackgroundRole)
                         .toString()
                         .toStdString(),
                     Colour::SUCCESS);
    TS_ASSERT_EQUALS(
        model.data(model.index(0, 0, model.index(1, 0)), Qt::BackgroundRole)
            .toString()
            .toStdString(),
        "");
    TS_ASSERT_EQUALS(
        model.data(model.index(1, 0, model.index(1, 0)), Qt::BackgroundRole)
            .toString()
            .toStdString(),
        "");
  }

  void testIsProcessed() {
    auto ws = fourRowTable();
    QTwoLevelTreeModel model(ws, m_whitelist);

    // Set 1st row of 1st group and 2nd group processed
    model.setProcessed(true, 0, model.index(0, 0));
    model.setProcessed(true, 1);

    // Non-existent row
    TS_ASSERT_THROWS(model.isProcessed(10, model.index(0, 0)),
                     const std::invalid_argument &);
    TS_ASSERT_THROWS(model.isProcessed(-1, model.index(0, 0)),
                     const std::invalid_argument &);

    // Non-existent group
    TS_ASSERT_THROWS(model.isProcessed(10), const std::invalid_argument &);
    TS_ASSERT_THROWS(model.isProcessed(-1), const std::invalid_argument &);

    // Only the 1st row of 1st group and 2nd group should be highlighted
    TS_ASSERT_EQUALS(model.isProcessed(model.index(0, 0).row(), QModelIndex()),
                     false);
    TS_ASSERT_EQUALS(model.isProcessed(0, model.index(0, 0)), true);
    TS_ASSERT_EQUALS(model.isProcessed(1, model.index(0, 0)), false);
    TS_ASSERT_EQUALS(model.isProcessed(model.index(1, 0).row(), QModelIndex()),
                     true);
    TS_ASSERT_EQUALS(model.isProcessed(0, model.index(1, 0)), false);
    TS_ASSERT_EQUALS(model.isProcessed(1, model.index(1, 0)), false);
  }

  void testTransferThrowsIfNoGroupSpecified() {
    auto ws = oneRowTable();
    QTwoLevelTreeModel model(ws, m_whitelist);

    auto rowValues = std::map<QString, QString>{{"Column1", "row_10"},
                                                {"Column2", "row_11"}};
    auto rowsToTransfer = std::vector<std::map<QString, QString>>{{rowValues}};
    TS_ASSERT_THROWS(model.transfer(rowsToTransfer),
                     const std::invalid_argument &);
  }

  void testTransferToExistingGroup() {
    auto ws = oneRowTable();
    QTwoLevelTreeModel model(ws, m_whitelist);

    constexpr int group = 0;
    auto rowValues = std::map<QString, QString>{
        {"Group", "group_0"}, {"Column1", "row_10"}, {"Column2", "row_11"}};
    auto rowsToTransfer = std::vector<std::map<QString, QString>>{{rowValues}};
    model.transfer(rowsToTransfer);

    // One group with two rows
    TS_ASSERT_EQUALS(model.rowCount(model.index(0, 0)), 2);
    TS_ASSERT_EQUALS(model.rowCount(), 1);
    // New row inserted at end of group
    TS_ASSERT_EQUALS(model.cellValue(group, 0, 0), "row_00");
    TS_ASSERT_EQUALS(model.cellValue(group, 0, 1), "row_01");
    TS_ASSERT_EQUALS(model.cellValue(group, 1, 0), "row_10");
    TS_ASSERT_EQUALS(model.cellValue(group, 1, 1), "row_11");
  }

  void testTransferToExistingSortedGroupBeforeCurrentRow() {
    auto ws = oneRowTable();
    QTwoLevelTreeModel model(ws, whitelistWithKeyColumn());

    constexpr int group = 0;
    auto rowValues = std::map<QString, QString>{
        {"Group", "group_0"}, {"Column1", "arow_10"}, {"Column2", "arow_11"}};
    auto rowsToTransfer = std::vector<std::map<QString, QString>>{{rowValues}};
    model.transfer(rowsToTransfer);

    // One group with two rows
    TS_ASSERT_EQUALS(model.rowCount(model.index(0, 0)), 2);
    TS_ASSERT_EQUALS(model.rowCount(), 1);
    // The new row should be sorted first
    TS_ASSERT_EQUALS(model.cellValue(group, 0, 0), "arow_10");
    TS_ASSERT_EQUALS(model.cellValue(group, 0, 1), "arow_11");
    TS_ASSERT_EQUALS(model.cellValue(group, 1, 0), "row_00");
    TS_ASSERT_EQUALS(model.cellValue(group, 1, 1), "row_01");
  }

  void testTransferToExistingSortedGroupAfterCurrentRow() {
    auto ws = oneRowTable();
    QTwoLevelTreeModel model(ws, whitelistWithKeyColumn());

    constexpr int group = 0;
    auto rowValues = std::map<QString, QString>{
        {"Group", "group_0"}, {"Column1", "zrow_10"}, {"Column2", "zrow_11"}};
    auto rowsToTransfer = std::vector<std::map<QString, QString>>{{rowValues}};
    model.transfer(rowsToTransfer);

    // One group with two rows
    TS_ASSERT_EQUALS(model.rowCount(), 1);
    TS_ASSERT_EQUALS(model.rowCount(model.index(0, 0)), 2);
    // The new row should be sorted last
    TS_ASSERT_EQUALS(model.cellValue(group, 0, 0), "row_00");
    TS_ASSERT_EQUALS(model.cellValue(group, 0, 1), "row_01");
    TS_ASSERT_EQUALS(model.cellValue(group, 1, 0), "zrow_10");
    TS_ASSERT_EQUALS(model.cellValue(group, 1, 1), "zrow_11");
  }

  void testTransferDuplicateRow() {
    auto ws = oneRowTable();
    QTwoLevelTreeModel model(ws, m_whitelist);

    // If the whole row is a duplicate nothing will be added
    constexpr int group = 0;
    auto rowValues = std::map<QString, QString>{
        {"Group", "group_0"}, {"Column1", "row_00"}, {"Column2", "row_01"}};
    auto rowsToTransfer = std::vector<std::map<QString, QString>>{{rowValues}};
    model.transfer(rowsToTransfer);

    // Should just have original group with one row and original values
    TS_ASSERT_EQUALS(model.rowCount(), 1);
    TS_ASSERT_EQUALS(model.rowCount(model.index(0, 0)), 1);
    TS_ASSERT_EQUALS(model.cellValue(group, 0, 0), "row_00");
    TS_ASSERT_EQUALS(model.cellValue(group, 0, 1), "row_01");
  }

  void testTransferOverwritesRow() {
    auto ws = oneRowTable();
    QTwoLevelTreeModel model(ws, whitelistWithKeyColumn());

    // If the group and key column matches, the existing row will be
    // overwritten
    constexpr int group = 0;
    auto rowValues = std::map<QString, QString>{
        {"Group", "group_0"}, {"Column1", "row_00"}, {"Column2", "new_row_01"}};
    auto rowsToTransfer = std::vector<std::map<QString, QString>>{{rowValues}};
    model.transfer(rowsToTransfer);

    // Still just one group with one row but containing new values
    TS_ASSERT_EQUALS(model.rowCount(), 1);
    TS_ASSERT_EQUALS(model.rowCount(model.index(0, 0)), 1);
    TS_ASSERT_EQUALS(model.cellValue(group, 0, 0), "row_00");
    TS_ASSERT_EQUALS(model.cellValue(group, 0, 1), "new_row_01");
  }

  void testTransferToNewGroup() {
    auto ws = oneRowTable();
    QTwoLevelTreeModel model(ws, m_whitelist);

    constexpr int group = 0;
    auto rowValues = std::map<QString, QString>{
        {"Group", "group_1"}, {"Column1", "row_10"}, {"Column2", "row_11"}};
    auto rowsToTransfer = std::vector<std::map<QString, QString>>{{rowValues}};
    model.transfer(rowsToTransfer);

    // The new row should be ordered first
    TS_ASSERT_EQUALS(model.rowCount(), 2);
    TS_ASSERT_EQUALS(model.cellValue(group, 0, 0), "row_00");
    TS_ASSERT_EQUALS(model.cellValue(group, 0, 1), "row_01");
    TS_ASSERT_EQUALS(model.cellValue(group + 1, 0, 0), "row_10");
    TS_ASSERT_EQUALS(model.cellValue(group + 1, 0, 1), "row_11");
  }

private:
  WhiteList m_whitelist;
};

#endif /* MANTID_MANTIDWIDGETS_QDATAPROCESSORTWOLEVELTREEMODELTEST_H */
