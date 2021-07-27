// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/FitScriptGeneratorDataTable.h"

#include <cxxtest/TestSuite.h>

#include <memory>

#include <QApplication>
#include <QtTest>

using namespace MantidQt::MantidWidgets;

class FitScriptGeneratorDataTableTest : public CxxTest::TestSuite {

public:
  static FitScriptGeneratorDataTableTest *createSuite() { return new FitScriptGeneratorDataTableTest; }
  static void destroySuite(FitScriptGeneratorDataTableTest *suite) { delete suite; }

  void setUp() override {
    assertNoTopLevelWidgets();
    m_dataTable = std::make_unique<FitScriptGeneratorDataTable>();
  }

  void tearDown() override {
    TS_ASSERT(m_dataTable->close());
    m_dataTable.reset();
  }

  void test_opening_the_data_table_will_create_a_top_level_widget() {
    m_dataTable->show();
    assertWidgetCreated();
  }

  void test_that_adding_a_domain_to_the_data_table_will_change_the_number_of_table_rows() {
    m_dataTable->show();
    TS_ASSERT_EQUALS(m_dataTable->rowCount(), 0);

    m_dataTable->addDomain("Name", WorkspaceIndex(0), 0.0, 2.0);

    TS_ASSERT_EQUALS(m_dataTable->rowCount(), 1);
  }

  void test_that_adding_a_domain_to_the_data_table_will_show_the_correct_data_in_the_table() {
    m_dataTable->show();
    m_dataTable->addDomain("Name", WorkspaceIndex(0), 0.0, 2.0);

    TS_ASSERT_EQUALS(m_dataTable->workspaceName(0), "Name");
    TS_ASSERT_EQUALS(m_dataTable->workspaceIndex(0), WorkspaceIndex(0));
    TS_ASSERT_EQUALS(m_dataTable->startX(0), 0.0);
    TS_ASSERT_EQUALS(m_dataTable->endX(0), 2.0);
  }

  void test_that_removing_a_domain_in_the_data_table_will_change_the_number_of_table_rows() {
    m_dataTable->show();
    m_dataTable->addDomain("Name", WorkspaceIndex(0), 0.0, 2.0);

    TS_ASSERT_EQUALS(m_dataTable->rowCount(), 1);
    m_dataTable->removeDomain(FitDomainIndex{0});

    TS_ASSERT_EQUALS(m_dataTable->rowCount(), 0);
  }

  void test_that_renameWorkspace_will_rename_the_all_rows_containing_that_workspace() {
    QString const newName("NewName");

    m_dataTable->show();
    m_dataTable->addDomain("Name", WorkspaceIndex(0), 0.0, 2.0);
    m_dataTable->addDomain("Name1", WorkspaceIndex(0), 0.0, 2.0);
    m_dataTable->addDomain("Name", WorkspaceIndex(1), 0.0, 2.0);
    m_dataTable->addDomain("Name2", WorkspaceIndex(0), 0.0, 2.0);
    m_dataTable->addDomain("Name", WorkspaceIndex(2), 0.0, 2.0);

    m_dataTable->renameWorkspace("Name", newName);

    TS_ASSERT_EQUALS(m_dataTable->workspaceName(0), newName.toStdString());
    TS_ASSERT_EQUALS(m_dataTable->workspaceName(1), "Name1");
    TS_ASSERT_EQUALS(m_dataTable->workspaceName(2), newName.toStdString());
    TS_ASSERT_EQUALS(m_dataTable->workspaceName(3), "Name2");
    TS_ASSERT_EQUALS(m_dataTable->workspaceName(4), newName.toStdString());
  }

  void test_that_renameWorkspace_will_not_cause_an_exception_if_a_workspace_name_does_not_exist() {
    m_dataTable->show();
    m_dataTable->addDomain("Name", WorkspaceIndex(0), 0.0, 2.0);

    m_dataTable->renameWorkspace("NonExistingName", "NewName");

    TS_ASSERT_EQUALS(m_dataTable->workspaceName(0), "Name");
  }

  void test_that_allRows_will_return_all_of_the_existing_row_indices() {
    m_dataTable->show();
    m_dataTable->addDomain("Name", WorkspaceIndex(0), 0.0, 2.0);
    m_dataTable->addDomain("Name2", WorkspaceIndex(0), 0.0, 2.0);
    m_dataTable->addDomain("Name3", WorkspaceIndex(0), 0.0, 2.0);

    auto const allIndices = m_dataTable->allRows();
    auto const expectedIndices = std::vector<FitDomainIndex>{2, 1, 0};
    TS_ASSERT_EQUALS(allIndices, expectedIndices);
  }

  void test_that_selectedRows_will_return_the_currently_selected_row() {
    int rowIndex(1);

    m_dataTable->show();
    m_dataTable->addDomain("Name", WorkspaceIndex(0), 0.0, 2.0);
    m_dataTable->addDomain("Name2", WorkspaceIndex(0), 0.0, 2.0);
    m_dataTable->addDomain("Name3", WorkspaceIndex(0), 0.0, 2.0);

    selectRowInTable(rowIndex);

    auto const selectedIndices = m_dataTable->selectedRows();
    TS_ASSERT_EQUALS(selectedIndices.size(), 1);
    TS_ASSERT_EQUALS(selectedIndices[0].value, rowIndex);
  }

  void test_that_selectedDomainFunctionPrefix_will_return_the_currently_selected_function_index() {
    int rowIndex(1);

    m_dataTable->show();
    m_dataTable->addDomain("Name", WorkspaceIndex(0), 0.0, 2.0);
    m_dataTable->addDomain("Name2", WorkspaceIndex(0), 0.0, 2.0);
    m_dataTable->addDomain("Name3", WorkspaceIndex(0), 0.0, 2.0);

    selectRowInTable(rowIndex);

    auto const selectedPrefix = m_dataTable->selectedDomainFunctionPrefix();
    TS_ASSERT_EQUALS(selectedPrefix, "f1.");
  }

private:
  void assertWidgetCreated() { TS_ASSERT_LESS_THAN(0, QApplication::topLevelWidgets().size()); }

  void assertNoTopLevelWidgets() { TS_ASSERT_EQUALS(0, QApplication::topLevelWidgets().size()); }

  void selectRowInTable(int row) {
    // Retrieve the pixel position of the first column cell at rowIndex
    int xPos = m_dataTable->columnViewportPosition(0) + 5;
    int yPos = m_dataTable->rowViewportPosition(row) + 10;

    // Click the table cell, thereby selecting a row
    QWidget *pViewport = m_dataTable->viewport();
    QTest::mouseClick(pViewport, Qt::LeftButton, NULL, QPoint(xPos, yPos));
    QApplication::sendPostedEvents();
  }

  std::unique_ptr<FitScriptGeneratorDataTable> m_dataTable;
};
