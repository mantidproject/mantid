// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/Common/FitScriptGeneratorDataTable.h"
#include "MantidQtWidgets/Common/FitScriptGeneratorMockObjects.h"
#include "MantidQtWidgets/Common/FitScriptGeneratorView.h"
#include "MantidQtWidgets/Common/IFitScriptGeneratorPresenter.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include <memory>
#include <string>
#include <vector>

#include <QApplication>
#include <QListView>
#include <QTableWidget>
#include <QtTest>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;
using namespace testing;
using namespace WorkspaceCreationHelper;

namespace {

IFunction_sptr createIFunction(std::string const &functionString) {
  return FunctionFactory::Instance().createInitialized(functionString);
}

CompositeFunction_sptr toComposite(IFunction_sptr function) {
  return std::dynamic_pointer_cast<CompositeFunction>(function);
}

CompositeFunction_sptr createEmptyComposite() {
  return toComposite(createIFunction("name=CompositeFunction"));
}

} // namespace

class FitScriptGeneratorViewTest : public CxxTest::TestSuite {

public:
  FitScriptGeneratorViewTest() { FrameworkManager::Instance(); }

  static FitScriptGeneratorViewTest *createSuite() {
    return new FitScriptGeneratorViewTest;
  }
  static void destroySuite(FitScriptGeneratorViewTest *suite) { delete suite; }

  void setUp() override {
    assertNoTopLevelWidgets();

    m_wsName = "Name";
    m_wsIndex = MantidQt::MantidWidgets::WorkspaceIndex(0);
    m_workspace = create2DWorkspace(3, 3);
    m_workspaceGroup = createWorkspaceGroup(3, 3, 3, "GroupName");

    m_function = createEmptyComposite();
    m_function->addFunction(createIFunction("name=FlatBackground"));
    m_function->addFunction(createIFunction("name=ExpDecay"));

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

    EXPECT_CALL(*m_presenter,
                notifyPresenterImpl(ViewEvent::RemoveClicked, "", ""))
        .Times(1);

    QTest::mouseClick(m_view->removeButton(), Qt::LeftButton);
    QApplication::sendPostedEvents();
  }

  void test_that_clicking_the_add_workspace_button_will_notify_the_presenter() {
    openFitScriptGeneratorWidget();

    EXPECT_CALL(*m_presenter,
                notifyPresenterImpl(ViewEvent::AddClicked, "", ""))
        .Times(1);

    QTest::mouseClick(m_view->addWorkspaceButton(), Qt::LeftButton);
    QApplication::sendPostedEvents();
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

    EXPECT_CALL(*m_presenter,
                notifyPresenterImpl(ViewEvent::StartXChanged, "", ""))
        .Times(1);

    changeValueInTableCell(0, ColumnIndex::StartX);
  }

  void test_that_modifying_the_endX_in_the_table_will_notify_the_presenter() {
    openFitScriptGeneratorWidget();
    m_view->addWorkspaceDomain(m_wsName, m_wsIndex, 0.0, 2.0);

    EXPECT_CALL(*m_presenter,
                notifyPresenterImpl(ViewEvent::EndXChanged, "", ""))
        .Times(1);

    changeValueInTableCell(0, ColumnIndex::EndX);
  }

  void test_that_allRows_will_return_all_of_the_existing_row_indices() {
    openFitScriptGeneratorWidget();
    m_view->addWorkspaceDomain("Name", m_wsIndex, 0.0, 2.0);
    m_view->addWorkspaceDomain("Name2", m_wsIndex, 0.0, 2.0);
    m_view->addWorkspaceDomain("Name3", m_wsIndex, 0.0, 2.0);

    auto const allIndices = m_view->allRows();
    auto const expectedIndices = std::vector<FitDomainIndex>{2, 1, 0};
    TS_ASSERT_EQUALS(allIndices, expectedIndices);
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

  void test_that_selectedRows_will_return_the_first_row_index_by_default() {
    openFitScriptGeneratorWidget();
    m_view->addWorkspaceDomain(m_wsName, m_wsIndex, 0.0, 2.0);
    m_view->addWorkspaceDomain("Name2", m_wsIndex, 0.0, 2.0);

    auto const selectedIndices = m_view->selectedRows();
    TS_ASSERT_EQUALS(selectedIndices.size(), 1);
    TS_ASSERT_EQUALS(selectedIndices[0], FitDomainIndex(0));
  }

  void
  test_that_parameterValue_will_return_the_correct_value_of_the_specified_parameter() {
    openFitScriptGeneratorWidget();
    m_view->addWorkspaceDomain(m_wsName, m_wsIndex, 0.0, 2.0);
    m_function->addFunction(createIFunction("name=LinearBackground"));
    m_view->setFunction(m_function);

    TS_ASSERT_EQUALS(m_view->parameterValue("f0.A0"), 0.0);
    TS_ASSERT_EQUALS(m_view->parameterValue("f1.Height"), 1.0);
    TS_ASSERT_EQUALS(m_view->parameterValue("f2.A0"), 0.0);
    TS_ASSERT_EQUALS(m_view->parameterValue("f2.A1"), 0.0);
  }

  void
  test_that_attributeValue_will_return_the_correct_value_of_the_specified_attribute() {
    openFitScriptGeneratorWidget();
    m_view->addWorkspaceDomain(m_wsName, m_wsIndex, 0.0, 2.0);
    m_view->setFunction(m_function);

    TS_ASSERT(!m_view->attributeValue("NumDeriv").asBool());
  }

  void
  test_that_getDialogWorkspaces_returns_the_expected_workspace_selected_in_the_AddWorkspaceDialog() {
    openFitScriptGeneratorWidget();

    auto dialog = m_view->addWorkspaceDialog();
    auto combobox = dialog->workspaceNameComboBox();
    auto lineedit = dialog->workspaceIndiceLineEdit();
    dialog->show();

    combobox->setCurrentIndex(4);
    lineedit->setText("0-2");
    dialog->accept();

    auto const workspaces = m_view->getDialogWorkspaces();
    TS_ASSERT_EQUALS(workspaces.size(), 1);
    TS_ASSERT_EQUALS(workspaces[0]->getNumberHistograms(), 3);
    TS_ASSERT_EQUALS(workspaces[0]->getName(), m_wsName);
  }

  void
  test_that_getDialogWorkspaces_returns_the_expected_workspaces_when_a_workspace_group_is_selected_in_the_AddWorkspaceDialog() {
    openFitScriptGeneratorWidget();

    auto dialog = m_view->addWorkspaceDialog();
    auto combobox = dialog->workspaceNameComboBox();
    auto lineedit = dialog->workspaceIndiceLineEdit();
    dialog->show();

    combobox->setCurrentIndex(0);
    lineedit->setText("0-2");
    dialog->accept();

    auto const workspaces = m_view->getDialogWorkspaces();
    TS_ASSERT_EQUALS(workspaces.size(), 3);
    for (auto i = 0u; i < workspaces.size(); ++i) {
      TS_ASSERT_EQUALS(workspaces[i]->getNumberHistograms(), 3);
      TS_ASSERT_EQUALS(workspaces[i]->getName(),
                       "GroupName_" + std::to_string(i));
    }
  }

  void
  test_that_getDialogWorkspaceIndices_returns_the_expected_workspace_index_selected_in_the_AddWorkspaceDialog() {
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
  test_that_getDialogWorkspaceIndices_returns_the_expected_range_of_workspace_indices_selected_in_the_AddWorkspaceDialog() {
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

    EXPECT_CALL(*m_presenter,
                notifyPresenterImpl(ViewEvent::SelectionChanged, "", ""))
        .Times(1);
    EXPECT_CALL(*m_presenter,
                notifyPresenterImpl(ViewEvent::StartXChanged, "", ""))
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
    QApplication::sendPostedEvents();
  }

  void selectRowInTable(int row) {
    // Retrieve the pixel position of a cell in the specified row
    int xPos = m_view->tableWidget()->columnViewportPosition(0) + 5;
    int yPos = m_view->tableWidget()->rowViewportPosition(row) + 10;

    // Click the table cell, thereby selecting the row
    QWidget *pViewport = m_view->tableWidget()->viewport();
    QTest::mouseClick(pViewport, Qt::LeftButton, NULL, QPoint(xPos, yPos));
    QApplication::sendPostedEvents();
  }

  std::string m_wsName;
  MantidQt::MantidWidgets::WorkspaceIndex m_wsIndex;
  Mantid::API::MatrixWorkspace_sptr m_workspace;
  Mantid::API::WorkspaceGroup_sptr m_workspaceGroup;
  Mantid::API::CompositeFunction_sptr m_function;
  std::unique_ptr<FitScriptGeneratorView> m_view;
  std::unique_ptr<MockFitScriptGeneratorPresenter> m_presenter;
};
