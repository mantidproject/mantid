// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/Common/AddWorkspaceDialog.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>

#include <memory>

#include <QApplication>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;
using namespace WorkspaceCreationHelper;

class AddWorkspaceDialogTest : public CxxTest::TestSuite {

public:
  static AddWorkspaceDialogTest *createSuite() { return new AddWorkspaceDialogTest; }
  static void destroySuite(AddWorkspaceDialogTest *suite) { delete suite; }

  void setUp() override {
    assertNoTopLevelWidgets();

    m_wsName = "Name";
    m_workspace = create2DWorkspace(3, 3);
    m_workspaceGroup = createWorkspaceGroup(3, 3, 3, "GroupName");

    AnalysisDataService::Instance().addOrReplace(m_wsName, m_workspace);

    m_dialog = std::make_unique<AddWorkspaceDialog>();
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();

    TS_ASSERT(m_dialog->close());

    m_dialog.reset();
  }

  void test_opening_the_dialog_will_create_a_top_level_widget() {
    m_dialog->show();
    assertWidgetCreated();
  }

  void test_that_getWorkspaces_returns_an_empty_vector_when_the_workspaces_do_not_exist_anymore() {
    m_dialog->show();

    auto combobox = m_dialog->workspaceNameComboBox();
    auto lineedit = m_dialog->workspaceIndiceLineEdit();

    combobox->setCurrentIndex(4);
    lineedit->setText("0-2");
    m_dialog->accept();

    AnalysisDataService::Instance().clear();

    auto const workspaces = m_dialog->getWorkspaces();
    TS_ASSERT(workspaces.empty());
  }

  void test_that_getWorkspaces_returns_the_expected_workspace_selected_in_the_AddWorkspaceDialog() {
    m_dialog->show();

    auto combobox = m_dialog->workspaceNameComboBox();
    auto lineedit = m_dialog->workspaceIndiceLineEdit();

    combobox->setCurrentIndex(4);
    lineedit->setText("0-2");
    m_dialog->accept();

    auto const workspaces = m_dialog->getWorkspaces();
    TS_ASSERT_EQUALS(workspaces.size(), 1);
    TS_ASSERT_EQUALS(workspaces[0]->getNumberHistograms(), 3);
    TS_ASSERT_EQUALS(workspaces[0]->getName(), m_wsName);
  }

  void
  test_that_getWorkspaces_returns_the_expected_workspaces_when_a_workspace_group_is_selected_in_the_AddWorkspaceDialog() {
    m_dialog->show();

    auto combobox = m_dialog->workspaceNameComboBox();
    auto lineedit = m_dialog->workspaceIndiceLineEdit();

    combobox->setCurrentIndex(0);
    lineedit->setText("0-2");
    m_dialog->accept();

    auto const workspaces = m_dialog->getWorkspaces();
    TS_ASSERT_EQUALS(workspaces.size(), 3);
    for (auto i = 0u; i < workspaces.size(); ++i) {
      TS_ASSERT_EQUALS(workspaces[i]->getNumberHistograms(), 3);
      TS_ASSERT_EQUALS(workspaces[i]->getName(), "GroupName_" + std::to_string(i));
    }
  }

  void test_that_workspaceIndices_returns_the_expected_workspaces_indices_from_the_AddWorkspaceDialog() {
    m_dialog->show();

    auto combobox = m_dialog->workspaceNameComboBox();
    auto lineedit = m_dialog->workspaceIndiceLineEdit();

    combobox->setCurrentIndex(0);
    lineedit->setText("0-2");
    m_dialog->accept();

    auto const expectedIndices = std::vector<int>{0, 1, 2};
    TS_ASSERT_EQUALS(m_dialog->workspaceIndices(), expectedIndices);
  }

private:
  void assertWidgetCreated() { TS_ASSERT_LESS_THAN(0, QApplication::topLevelWidgets().size()); }

  void assertNoTopLevelWidgets() { TS_ASSERT_EQUALS(0, QApplication::topLevelWidgets().size()); }

  std::string m_wsName;
  Mantid::API::MatrixWorkspace_sptr m_workspace;
  Mantid::API::WorkspaceGroup_sptr m_workspaceGroup;
  std::unique_ptr<AddWorkspaceDialog> m_dialog;
};
