// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Common/FitScriptGeneratorDataTable.h"
#include "MantidQtWidgets/Common/FitScriptGeneratorView.h"
#include "MantidQtWidgets/Common/IFitScriptGeneratorPresenter.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include <memory>

#include <QApplication>
#include <QListView>
#include <QTableWidget>
#include <QtTest>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;
using namespace testing;
using namespace WorkspaceCreationHelper;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockFitScriptGeneratorPresenter : public IFitScriptGeneratorPresenter {

public:
  MockFitScriptGeneratorPresenter(FitScriptGeneratorView *view) {
    m_view = view;
    m_view->subscribePresenter(this);
  }

  MOCK_METHOD1(notifyPresenter, void(ViewEvent const &ev));
  MOCK_METHOD0(openFitScriptGenerator, void());

private:
  FitScriptGeneratorView *m_view;
};

GNU_DIAG_ON_SUGGEST_OVERRIDE

class FitScriptGeneratorViewTest : public CxxTest::TestSuite {

public:
  static FitScriptGeneratorViewTest *createSuite() {
    return new FitScriptGeneratorViewTest;
  }
  static void destroySuite(FitScriptGeneratorViewTest *suite) { delete suite; }

  void setUp() override {
    assertNoTopLevelWidgets();

    m_wsName = "Name";
    m_wsIndex = MantidQt::MantidWidgets::WorkspaceIndex(0);
    m_workspace = create2DWorkspace(3, 3);

    AnalysisDataService::Instance().addOrReplace(m_wsName, m_workspace);

    m_view = std::make_unique<FitScriptGeneratorView>();
    m_presenter =
        std::make_unique<MockFitScriptGeneratorPresenter>(m_view.get());
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();

    TS_ASSERT(m_view->close());

    TS_ASSERT(Mock::VerifyAndClearExpectations(m_presenter.get()));
    m_presenter.reset();
    m_view.reset();
  }

  void test_opening_the_view_will_create_a_top_level_widget() {
    openFitScriptGeneratorWidget();
    assertWidgetCreated();
  }

  void test_that_clicking_the_remove_button_will_notify_the_presenter() {
    openFitScriptGeneratorWidget();

    EXPECT_CALL(*m_presenter, notifyPresenter(ViewEvent::RemoveClicked))
        .Times(1);

    QTest::mouseClick(m_view->removeButton(), Qt::LeftButton);
    QTest::qWait(500);
  }

  void test_that_clicking_the_add_workspace_button_will_notify_the_presenter() {
    openFitScriptGeneratorWidget();

    EXPECT_CALL(*m_presenter, notifyPresenter(ViewEvent::AddClicked)).Times(1);

    QTest::mouseClick(m_view->addWorkspaceButton(), Qt::LeftButton);
    QTest::qWait(500);
  }

  void
  test_that_adding_a_domain_to_the_view_will_change_the_number_of_table_rows() {
    openFitScriptGeneratorWidget();
    TS_ASSERT_EQUALS(m_view->tableWidget()->rowCount(), 0);

    m_view->addWorkspaceDomain(m_wsName, m_wsIndex, 0.0, 2.0);

    TS_ASSERT_EQUALS(m_view->tableWidget()->rowCount(), 1);
  }

  void
  test_that_adding_a_domain_to_the_view_will_show_the_correct_data_in_the_table() {
    openFitScriptGeneratorWidget();
    m_view->addWorkspaceDomain(m_wsName, m_wsIndex, 0.0, 2.0);

    TS_ASSERT_EQUALS(m_view->workspaceName(0), m_wsName);
    TS_ASSERT_EQUALS(m_view->workspaceIndex(0), m_wsIndex);
    TS_ASSERT_EQUALS(m_view->startX(0), 0.0);
    TS_ASSERT_EQUALS(m_view->endX(0), 2.0);
  }

  void
  test_that_removing_a_domain_in_the_view_will_change_the_number_of_table_rows() {
    openFitScriptGeneratorWidget();
    m_view->addWorkspaceDomain(m_wsName, m_wsIndex, 0.0, 2.0);

    TS_ASSERT_EQUALS(m_view->tableWidget()->rowCount(), 1);
    m_view->removeWorkspaceDomain(m_wsName, m_wsIndex);

    TS_ASSERT_EQUALS(m_view->tableWidget()->rowCount(), 0);
  }

  void test_that_modifying_the_startX_in_the_table_will_notify_the_presenter() {
    openFitScriptGeneratorWidget();
    m_view->addWorkspaceDomain(m_wsName, m_wsIndex, 0.0, 2.0);

    EXPECT_CALL(*m_presenter, notifyPresenter(ViewEvent::StartXChanged))
        .Times(1);

    changeValueInTableCell(0, ColumnIndex::StartX);
  }

  void test_that_modifying_the_endX_in_the_table_will_notify_the_presenter() {
    openFitScriptGeneratorWidget();
    m_view->addWorkspaceDomain(m_wsName, m_wsIndex, 0.0, 2.0);

    EXPECT_CALL(*m_presenter, notifyPresenter(ViewEvent::EndXChanged)).Times(1);

    changeValueInTableCell(0, ColumnIndex::EndX);
  }

  void test_that_selectedRows_will_return_the_currently_selected_row() {
    int rowIndex(1);

    openFitScriptGeneratorWidget();
    m_view->addWorkspaceDomain("Name", m_wsIndex, 0.0, 2.0);
    m_view->addWorkspaceDomain("Name2", m_wsIndex, 0.0, 2.0);
    m_view->addWorkspaceDomain("Name3", m_wsIndex, 0.0, 2.0);

    selectRowInTable(rowIndex);

    auto const selectedIndices = m_view->selectedRows();
    TS_ASSERT_EQUALS(selectedIndices.size(), 1);
    TS_ASSERT_EQUALS(selectedIndices[0].value, rowIndex);
  }

  void
  test_that_selectedRows_will_return_an_empty_vector_if_there_are_no_currently_selected_rows() {
    openFitScriptGeneratorWidget();
    m_view->addWorkspaceDomain(m_wsName, m_wsIndex, 0.0, 2.0);

    auto const selectedIndices = m_view->selectedRows();
    TS_ASSERT_EQUALS(selectedIndices.size(), 0);
  }

  void
  test_that_getDialogWorkspaces_returns_the_expected_workspace_selected_in_the_AddWorkspaceDialog() {
    openFitScriptGeneratorWidget();

    auto dialog = m_view->addWorkspaceDialog();
    auto combobox = dialog->workspaceNameComboBox();
    auto lineedit = dialog->workspaceIndiceLineEdit();
    dialog->show();

    combobox->setCurrentIndex(0);
    lineedit->setText("0-2");
    dialog->accept();

    auto const workspaces = m_view->getDialogWorkspaces();
    TS_ASSERT_EQUALS(workspaces.size(), 1);
    TS_ASSERT_EQUALS(workspaces[0]->getNumberHistograms(), 3);
    TS_ASSERT_EQUALS(workspaces[0]->getName(), m_wsName);
  }

  void
  test_that_getDialogWorkspaces_returns_the_expected_workspace_index_selected_in_the_AddWorkspaceDialog() {
    openFitScriptGeneratorWidget();

    auto dialog = m_view->addWorkspaceDialog();
    auto combobox = dialog->workspaceNameComboBox();
    auto lineedit = dialog->workspaceIndiceLineEdit();
    dialog->show();

    combobox->setCurrentIndex(0);
    lineedit->setText("1");
    dialog->accept();

    auto const workspaceIndices = m_view->getDialogWorkspaceIndices();
    TS_ASSERT_EQUALS(workspaceIndices.size(), 1);
    TS_ASSERT_EQUALS(workspaceIndices[0],
                     MantidQt::MantidWidgets::WorkspaceIndex(1));
  }

  void
  test_that_getDialogWorkspaces_returns_the_expected_range_of_workspace_indices_selected_in_the_AddWorkspaceDialog() {
    openFitScriptGeneratorWidget();

    auto dialog = m_view->addWorkspaceDialog();
    auto combobox = dialog->workspaceNameComboBox();
    auto lineedit = dialog->workspaceIndiceLineEdit();
    dialog->show();

    combobox->setCurrentIndex(0);
    lineedit->setText("0-2");
    dialog->accept();

    auto const workspaceIndices = m_view->getDialogWorkspaceIndices();
    TS_ASSERT_EQUALS(workspaceIndices.size(), 3);
    for (auto i = 0u; i < workspaceIndices.size(); ++i)
      TS_ASSERT_EQUALS(workspaceIndices[i],
                       MantidQt::MantidWidgets::WorkspaceIndex(i));
  }

  void
  test_that_resetSelection_will_reset_the_selected_rows_value_to_its_previous_value() {
    int rowIndex(1);
    openFitScriptGeneratorWidget();
    m_view->addWorkspaceDomain(m_wsName, m_wsIndex, 0.0, 2.0);
    m_view->addWorkspaceDomain("Name2", m_wsIndex, 0.0, 2.0);

    EXPECT_CALL(*m_presenter, notifyPresenter(ViewEvent::StartXChanged))
        .Times(1);

    // Change the value of StartX to 5.0
    changeValueInTableCell(rowIndex, ColumnIndex::StartX);

    // Reset selected row to the previous value of 0.0
    m_view->resetSelection();

    auto const startX = m_view->startX(FitDomainIndex(rowIndex));
    TS_ASSERT_EQUALS(startX, 0.0);
  }

private:
  void openFitScriptGeneratorWidget() {
    EXPECT_CALL(*m_presenter, openFitScriptGenerator()).Times(1);
    m_presenter->openFitScriptGenerator();
  }

  void assertWidgetCreated() {
    TS_ASSERT_LESS_THAN(0, QApplication::topLevelWidgets().size());
  }

  void assertNoTopLevelWidgets() {
    TS_ASSERT_EQUALS(0, QApplication::topLevelWidgets().size());
  }

  void changeValueInTableCell(int row, int column) {
    // Retrieve the pixel position of a StartX cell
    int xPos = m_view->tableWidget()->columnViewportPosition(column) + 5;
    int yPos = m_view->tableWidget()->rowViewportPosition(row) + 10;

    // Double click the table cell and change its value
    QWidget *pViewport = m_view->tableWidget()->viewport();
    QTest::mouseClick(pViewport, Qt::LeftButton, NULL, QPoint(xPos, yPos));
    QTest::mouseDClick(pViewport, Qt::LeftButton, NULL, QPoint(xPos, yPos));
    QTest::keyClick(pViewport->focusWidget(), Qt::Key_5);
    QTest::keyClick(pViewport->focusWidget(), Qt::Key_Enter);
    QTest::qWait(500);
  }

  void selectRowInTable(int row) {
    // Retrieve the pixel position of a cell in the specified row
    int xPos = m_view->tableWidget()->columnViewportPosition(0) + 5;
    int yPos = m_view->tableWidget()->rowViewportPosition(row) + 10;

    // Click the table cell, thereby selecting the row
    QWidget *pViewport = m_view->tableWidget()->viewport();
    QTest::mouseClick(pViewport, Qt::LeftButton, NULL, QPoint(xPos, yPos));
    QTest::qWait(500);
  }

  std::string m_wsName;
  MantidQt::MantidWidgets::WorkspaceIndex m_wsIndex;
  Mantid::API::MatrixWorkspace_sptr m_workspace;
  std::unique_ptr<FitScriptGeneratorView> m_view;
  std::unique_ptr<MockFitScriptGeneratorPresenter> m_presenter;
};
