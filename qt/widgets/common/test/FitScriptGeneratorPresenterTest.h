// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtWidgets/Common/FitScriptGeneratorPresenter.h"
#include "MantidQtWidgets/Common/IFitScriptGeneratorModel.h"
#include "MantidQtWidgets/Common/IFitScriptGeneratorView.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include <memory>

using namespace MantidQt::MantidWidgets;
using namespace testing;
using namespace WorkspaceCreationHelper;

namespace {

class MockFitScriptGeneratorView : public IFitScriptGeneratorView {

public:
  MOCK_METHOD1(subscribePresenter,
               void(IFitScriptGeneratorPresenter *presenter));

  MOCK_CONST_METHOD1(workspaceName, std::string(FitDomainIndex index));
  MOCK_CONST_METHOD1(workspaceIndex, WorkspaceIndex(FitDomainIndex index));
  MOCK_CONST_METHOD1(startX, double(FitDomainIndex index));
  MOCK_CONST_METHOD1(endX, double(FitDomainIndex index));

  MOCK_CONST_METHOD0(selectedRows, std::vector<FitDomainIndex>());

  MOCK_METHOD2(removeWorkspaceDomain, void(std::string const &workspaceName,
                                           WorkspaceIndex workspaceIndex));
  MOCK_METHOD4(addWorkspaceDomain,
               void(std::string const &workspaceName,
                    WorkspaceIndex workspaceIndex, double startX, double endX));

  MOCK_METHOD0(openAddWorkspaceDialog, bool());
  MOCK_METHOD0(getDialogWorkspaces,
               std::vector<Mantid::API::MatrixWorkspace_const_sptr>());
  MOCK_CONST_METHOD0(getDialogWorkspaceIndices, std::vector<WorkspaceIndex>());

  MOCK_METHOD0(resetSelection, void());

  MOCK_METHOD1(displayWarning, void(std::string const &message));

  MOCK_CONST_METHOD0(tableWidget, FitScriptGeneratorDataTable *());
  MOCK_CONST_METHOD0(removeButton, QPushButton *());
  MOCK_CONST_METHOD0(addWorkspaceButton, QPushButton *());
};

class MockFitScriptGeneratorModel : public IFitScriptGeneratorModel {

public:
  MOCK_METHOD2(removeWorkspaceDomain, void(std::string const &workspaceName,
                                           WorkspaceIndex workspaceIndex));
  MOCK_METHOD4(addWorkspaceDomain,
               void(std::string const &workspaceName,
                    WorkspaceIndex workspaceIndex, double startX, double endX));

  MOCK_CONST_METHOD3(isXValid,
                     bool(std::string const &workspaceName,
                          WorkspaceIndex workspaceIndex, double xValue));

  MOCK_METHOD3(updateStartX,
               void(std::string const &workspaceName,
                    WorkspaceIndex workspaceIndex, double startX));
  MOCK_METHOD3(updateEndX, void(std::string const &workspaceName,
                                WorkspaceIndex workspaceIndex, double endX));
};

} // namespace

class FitScriptGeneratorPresenterTest : public CxxTest::TestSuite {

public:
  static FitScriptGeneratorPresenterTest *createSuite() {
    return new FitScriptGeneratorPresenterTest;
  }
  static void destroySuite(FitScriptGeneratorPresenterTest *suite) {
    delete suite;
  }

  void setUp() override {
    m_wsName = "Name";
    m_wsIndex = WorkspaceIndex(0);
    m_workspace = create2DWorkspace(3, 3);
    m_startX = m_workspace->x(m_wsIndex.value).front();
    m_endX = m_workspace->x(m_wsIndex.value).back();

    Mantid::API::AnalysisDataService::Instance().addOrReplace(m_wsName,
                                                              m_workspace);

    m_view = std::make_unique<MockFitScriptGeneratorView>();
    m_model = std::make_unique<MockFitScriptGeneratorModel>();
    m_presenter = std::make_unique<FitScriptGeneratorPresenter>(m_view.get(),
                                                                m_model.get());
  }

  void tearDown() override {
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_view.get()));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_model.get()));

    m_presenter.reset();
    m_view.reset();
    m_model.reset();

    Mantid::API::AnalysisDataService::Instance().clear();
  }

  void
  test_that_a_remove_domain_event_will_attempt_to_remove_a_domain_in_the_view_and_model() {
    auto const selectedRow = FitDomainIndex(0);
    auto const selectedRows = std::vector<FitDomainIndex>{selectedRow};

    ON_CALL(*m_view, selectedRows()).WillByDefault(Return(selectedRows));
    ON_CALL(*m_view, workspaceName(selectedRow))
        .WillByDefault(Return(m_wsName));
    ON_CALL(*m_view, workspaceIndex(selectedRow))
        .WillByDefault(Return(m_wsIndex));

    EXPECT_CALL(*m_view, selectedRows())
        .Times(1)
        .WillOnce(Return(selectedRows));
    EXPECT_CALL(*m_view, workspaceName(selectedRow))
        .Times(1)
        .WillOnce(Return(m_wsName));
    EXPECT_CALL(*m_view, workspaceIndex(selectedRow))
        .Times(1)
        .WillOnce(Return(m_wsIndex));

    EXPECT_CALL(*m_view, removeWorkspaceDomain(m_wsName, m_wsIndex)).Times(1);
    EXPECT_CALL(*m_model, removeWorkspaceDomain(m_wsName, m_wsIndex)).Times(1);

    m_presenter->notifyPresenter(ViewEvent::RemoveClicked);
  }

  void
  test_that_a_add_domain_event_will_attempt_to_add_a_domain_in_the_view_and_model() {
    auto const workspaces =
        std::vector<Mantid::API::MatrixWorkspace_const_sptr>{m_workspace};
    auto const workspaceIndices = std::vector<WorkspaceIndex>{m_wsIndex};

    ON_CALL(*m_view, openAddWorkspaceDialog()).WillByDefault(Return(true));
    ON_CALL(*m_view, getDialogWorkspaces()).WillByDefault(Return(workspaces));
    ON_CALL(*m_view, getDialogWorkspaceIndices())
        .WillByDefault(Return(workspaceIndices));

    EXPECT_CALL(*m_view, openAddWorkspaceDialog())
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(*m_view, getDialogWorkspaces())
        .Times(1)
        .WillOnce(Return(workspaces));
    EXPECT_CALL(*m_view, getDialogWorkspaceIndices())
        .Times(1)
        .WillOnce(Return(workspaceIndices));

    EXPECT_CALL(*m_view,
                addWorkspaceDomain(m_wsName, m_wsIndex, m_startX, m_endX))
        .Times(1);
    EXPECT_CALL(*m_model,
                addWorkspaceDomain(m_wsName, m_wsIndex, m_startX, m_endX))
        .Times(1);

    m_presenter->notifyPresenter(ViewEvent::AddClicked);
  }

  void
  test_that_changing_a_start_x_will_update_its_value_in_the_model_when_the_x_value_is_valid() {
    auto const selectedRow = FitDomainIndex(0);
    auto const selectedRows = std::vector<FitDomainIndex>{selectedRow};

    ON_CALL(*m_view, selectedRows()).WillByDefault(Return(selectedRows));
    ON_CALL(*m_view, workspaceName(selectedRow))
        .WillByDefault(Return(m_wsName));
    ON_CALL(*m_view, workspaceIndex(selectedRow))
        .WillByDefault(Return(m_wsIndex));
    ON_CALL(*m_view, startX(selectedRow)).WillByDefault(Return(m_startX));
    ON_CALL(*m_model, isXValid(m_wsName, m_wsIndex, m_startX))
        .WillByDefault(Return(true));

    EXPECT_CALL(*m_view, selectedRows())
        .Times(1)
        .WillOnce(Return(selectedRows));
    EXPECT_CALL(*m_view, workspaceName(selectedRow))
        .Times(1)
        .WillOnce(Return(m_wsName));
    EXPECT_CALL(*m_view, workspaceIndex(selectedRow))
        .Times(1)
        .WillOnce(Return(m_wsIndex));
    EXPECT_CALL(*m_view, startX(selectedRow))
        .Times(1)
        .WillOnce(Return(m_startX));
    EXPECT_CALL(*m_model, isXValid(m_wsName, m_wsIndex, m_startX))
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(*m_model, updateStartX(m_wsName, m_wsIndex, m_startX)).Times(1);

    m_presenter->notifyPresenter(ViewEvent::StartXChanged);
  }

  void
  test_that_changing_a_start_x_will_reset_the_view_if_its_new_value_is_invalid() {
    auto const selectedRow = FitDomainIndex(0);
    auto const selectedRows = std::vector<FitDomainIndex>{selectedRow};

    ON_CALL(*m_view, selectedRows()).WillByDefault(Return(selectedRows));
    ON_CALL(*m_view, workspaceName(selectedRow))
        .WillByDefault(Return(m_wsName));
    ON_CALL(*m_view, workspaceIndex(selectedRow))
        .WillByDefault(Return(m_wsIndex));
    ON_CALL(*m_view, startX(selectedRow)).WillByDefault(Return(m_startX));
    ON_CALL(*m_model, isXValid(m_wsName, m_wsIndex, m_startX))
        .WillByDefault(Return(false));

    EXPECT_CALL(*m_view, selectedRows())
        .Times(1)
        .WillOnce(Return(selectedRows));
    EXPECT_CALL(*m_view, workspaceName(selectedRow))
        .Times(1)
        .WillOnce(Return(m_wsName));
    EXPECT_CALL(*m_view, workspaceIndex(selectedRow))
        .Times(1)
        .WillOnce(Return(m_wsIndex));
    EXPECT_CALL(*m_view, startX(selectedRow))
        .Times(1)
        .WillOnce(Return(m_startX));
    EXPECT_CALL(*m_model, isXValid(m_wsName, m_wsIndex, m_startX))
        .Times(1)
        .WillOnce(Return(false));
    EXPECT_CALL(*m_view, resetSelection()).Times(1);
    EXPECT_CALL(*m_view, displayWarning("The StartX provided must be within "
                                        "the x limits of its workspace."))
        .Times(1);

    m_presenter->notifyPresenter(ViewEvent::StartXChanged);
  }

  void
  test_that_changing_a_end_x_will_update_its_value_in_the_model_when_the_x_value_is_valid() {
    auto const selectedRow = FitDomainIndex(0);
    auto const selectedRows = std::vector<FitDomainIndex>{selectedRow};

    ON_CALL(*m_view, selectedRows()).WillByDefault(Return(selectedRows));
    ON_CALL(*m_view, workspaceName(selectedRow))
        .WillByDefault(Return(m_wsName));
    ON_CALL(*m_view, workspaceIndex(selectedRow))
        .WillByDefault(Return(m_wsIndex));
    ON_CALL(*m_view, endX(selectedRow)).WillByDefault(Return(m_endX));
    ON_CALL(*m_model, isXValid(m_wsName, m_wsIndex, m_endX))
        .WillByDefault(Return(true));

    EXPECT_CALL(*m_view, selectedRows())
        .Times(1)
        .WillOnce(Return(selectedRows));
    EXPECT_CALL(*m_view, workspaceName(selectedRow))
        .Times(1)
        .WillOnce(Return(m_wsName));
    EXPECT_CALL(*m_view, workspaceIndex(selectedRow))
        .Times(1)
        .WillOnce(Return(m_wsIndex));
    EXPECT_CALL(*m_view, endX(selectedRow)).Times(1).WillOnce(Return(m_endX));
    EXPECT_CALL(*m_model, isXValid(m_wsName, m_wsIndex, m_endX))
        .Times(1)
        .WillOnce(Return(true));
    EXPECT_CALL(*m_model, updateEndX(m_wsName, m_wsIndex, m_endX)).Times(1);

    m_presenter->notifyPresenter(ViewEvent::EndXChanged);
  }

  void
  test_that_changing_a_end_x_will_reset_the_view_if_its_new_value_is_invalid() {
    auto const selectedRow = FitDomainIndex(0);
    auto const selectedRows = std::vector<FitDomainIndex>{selectedRow};

    ON_CALL(*m_view, selectedRows()).WillByDefault(Return(selectedRows));
    ON_CALL(*m_view, workspaceName(selectedRow))
        .WillByDefault(Return(m_wsName));
    ON_CALL(*m_view, workspaceIndex(selectedRow))
        .WillByDefault(Return(m_wsIndex));
    ON_CALL(*m_view, endX(selectedRow)).WillByDefault(Return(m_endX));
    ON_CALL(*m_model, isXValid(m_wsName, m_wsIndex, m_endX))
        .WillByDefault(Return(false));

    EXPECT_CALL(*m_view, selectedRows())
        .Times(1)
        .WillOnce(Return(selectedRows));
    EXPECT_CALL(*m_view, workspaceName(selectedRow))
        .Times(1)
        .WillOnce(Return(m_wsName));
    EXPECT_CALL(*m_view, workspaceIndex(selectedRow))
        .Times(1)
        .WillOnce(Return(m_wsIndex));
    EXPECT_CALL(*m_view, endX(selectedRow)).Times(1).WillOnce(Return(m_endX));
    EXPECT_CALL(*m_model, isXValid(m_wsName, m_wsIndex, m_endX))
        .Times(1)
        .WillOnce(Return(false));
    EXPECT_CALL(*m_view, resetSelection()).Times(1);
    EXPECT_CALL(*m_view, displayWarning("The EndX provided must be within "
                                        "the x limits of its workspace."))
        .Times(1);

    m_presenter->notifyPresenter(ViewEvent::EndXChanged);
  }

private:
  std::string m_wsName;
  WorkspaceIndex m_wsIndex;
  Mantid::API::MatrixWorkspace_sptr m_workspace;
  double m_startX;
  double m_endX;

  std::unique_ptr<MockFitScriptGeneratorView> m_view;
  std::unique_ptr<MockFitScriptGeneratorModel> m_model;
  std::unique_ptr<FitScriptGeneratorPresenter> m_presenter;
};
