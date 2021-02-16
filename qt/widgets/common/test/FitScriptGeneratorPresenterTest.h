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
#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Common/FitScriptGeneratorMockObjects.h"
#include "MantidQtWidgets/Common/FitScriptGeneratorPresenter.h"
#include "MantidQtWidgets/Common/FittingGlobals.h"
#include "MantidQtWidgets/Common/IFitScriptGeneratorModel.h"
#include "MantidQtWidgets/Common/IFitScriptGeneratorView.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include <memory>
#include <vector>

using namespace MantidQt::MantidWidgets;
using namespace testing;
using namespace WorkspaceCreationHelper;

namespace {

Mantid::API::IFunction_sptr createIFunction(std::string const &functionString) {
  return Mantid::API::FunctionFactory::Instance().createInitialized(
      functionString);
}

} // namespace

GNU_DIAG_OFF_SUGGEST_OVERRIDE

MATCHER_P(BoolAttributeValue, value, "") { return arg.asBool() == value; }
MATCHER_P(VectorSize, expectedSize, "") { return arg.size() == expectedSize; }

GNU_DIAG_ON_SUGGEST_OVERRIDE

class FitScriptGeneratorPresenterTest : public CxxTest::TestSuite {

public:
  FitScriptGeneratorPresenterTest() {
    Mantid::API::FrameworkManager::Instance();
  }

  static FitScriptGeneratorPresenterTest *createSuite() {
    return new FitScriptGeneratorPresenterTest;
  }
  static void destroySuite(FitScriptGeneratorPresenterTest *suite) {
    delete suite;
  }

  void setUp() override {
    m_wsName = "Name";
    m_wsIndex = MantidQt::MantidWidgets::WorkspaceIndex(0);
    m_workspace = create2DWorkspace(3, 3);
    m_startX = m_workspace->x(m_wsIndex.value).front();
    m_endX = m_workspace->x(m_wsIndex.value).back();
    m_function = createIFunction("name=FlatBackground");

    Mantid::API::AnalysisDataService::Instance().addOrReplace(m_wsName,
                                                              m_workspace);

    m_view = std::make_unique<MockFitScriptGeneratorView>();
    m_model = std::make_unique<MockFitScriptGeneratorModel>();

    EXPECT_CALL(*m_model, subscribePresenter(_)).Times(1);
    EXPECT_CALL(*m_view, subscribePresenter(_)).Times(1);
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
    auto const globals = std::vector<GlobalParameter>{};

    ON_CALL(*m_view, selectedRows()).WillByDefault(Return(selectedRows));
    ON_CALL(*m_view, workspaceName(selectedRow))
        .WillByDefault(Return(m_wsName));
    ON_CALL(*m_view, workspaceIndex(selectedRow))
        .WillByDefault(Return(m_wsIndex));

    ON_CALL(*m_model, getFittingMode())
        .WillByDefault(Return(FittingMode::SEQUENTIAL));
    ON_CALL(*m_model, getFunction(m_wsName, m_wsIndex))
        .WillByDefault(Return(m_function));

    ON_CALL(*m_model, getGlobalParameters()).WillByDefault(Return(globals));

    EXPECT_CALL(*m_view, selectedRows())
        .Times(2)
        .WillRepeatedly(Return(selectedRows));
    EXPECT_CALL(*m_view, workspaceName(selectedRow))
        .Times(2)
        .WillRepeatedly(Return(m_wsName));
    EXPECT_CALL(*m_view, workspaceIndex(selectedRow))
        .Times(2)
        .WillRepeatedly(Return(m_wsIndex));

    EXPECT_CALL(*m_view, removeWorkspaceDomain(m_wsName, m_wsIndex)).Times(1);
    EXPECT_CALL(*m_model, removeWorkspaceDomain(m_wsName, m_wsIndex)).Times(1);

    EXPECT_CALL(*m_model, getFittingMode()).Times(1);
    EXPECT_CALL(*m_view, setSimultaneousMode(false)).Times(1);

    EXPECT_CALL(*m_model, getFunction(m_wsName, m_wsIndex)).Times(1);
    EXPECT_CALL(*m_view, setFunction(m_function)).Times(1);

    EXPECT_CALL(*m_model, getGlobalParameters()).Times(1);
    EXPECT_CALL(*m_view, setGlobalParameters(VectorSize(0u))).Times(1);

    m_presenter->notifyPresenter(ViewEvent::RemoveClicked);
  }

  void
  test_that_a_add_domain_event_will_attempt_to_add_a_domain_in_the_view_and_model() {
    auto const workspaces =
        std::vector<Mantid::API::MatrixWorkspace_const_sptr>{m_workspace};
    auto const workspaceIndices =
        std::vector<MantidQt::MantidWidgets::WorkspaceIndex>{m_wsIndex};

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
    ON_CALL(*m_model, updateStartX(m_wsName, m_wsIndex, m_startX))
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
    EXPECT_CALL(*m_model, updateStartX(m_wsName, m_wsIndex, m_startX))
        .Times(1)
        .WillOnce(Return(true));

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
    ON_CALL(*m_model, updateStartX(m_wsName, m_wsIndex, m_startX))
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
    EXPECT_CALL(*m_model, updateStartX(m_wsName, m_wsIndex, m_startX))
        .Times(1)
        .WillOnce(Return(false));
    EXPECT_CALL(*m_view, resetSelection()).Times(1);
    EXPECT_CALL(
        *m_view,
        displayWarning("The StartX provided must be within the x limits of "
                       "its workspace, and less than the EndX."))
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
    ON_CALL(*m_model, updateEndX(m_wsName, m_wsIndex, m_endX))
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
    EXPECT_CALL(*m_model, updateEndX(m_wsName, m_wsIndex, m_endX))
        .Times(1)
        .WillOnce(Return(true));

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
    ON_CALL(*m_model, updateEndX(m_wsName, m_wsIndex, m_endX))
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
    EXPECT_CALL(*m_model, updateEndX(m_wsName, m_wsIndex, m_endX))
        .Times(1)
        .WillOnce(Return(false));
    EXPECT_CALL(*m_view, resetSelection()).Times(1);
    EXPECT_CALL(*m_view, displayWarning(
                             "The EndX provided must be within the x limits of "
                             "its workspace, and greater than the StartX."))
        .Times(1);

    m_presenter->notifyPresenter(ViewEvent::EndXChanged);
  }

  void
  test_that_SelectionChanged_will_set_the_function_in_the_view_when_a_row_exists() {
    set_selection_changed_expectations(FitDomainIndex(0));
    m_presenter->notifyPresenter(ViewEvent::SelectionChanged);
  }

  void test_that_SelectionChanged_will_clear_the_function_when_no_rows_exist() {
    set_selection_changed_expectations(FitDomainIndex(0), true);
    m_presenter->notifyPresenter(ViewEvent::SelectionChanged);
  }

  void
  test_that_FunctionRemoved_will_remove_the_function_from_the_relevant_domains() {
    auto const selectedRow = FitDomainIndex(0);
    auto const selectedRows = std::vector<FitDomainIndex>{selectedRow};

    ON_CALL(*m_view, isAddRemoveFunctionForAllChecked())
        .WillByDefault(Return(false));
    ON_CALL(*m_view, selectedRows()).WillByDefault(Return(selectedRows));
    ON_CALL(*m_view, workspaceName(selectedRow))
        .WillByDefault(Return(m_wsName));
    ON_CALL(*m_view, workspaceIndex(selectedRow))
        .WillByDefault(Return(m_wsIndex));

    EXPECT_CALL(*m_view, isAddRemoveFunctionForAllChecked)
        .Times(1)
        .WillOnce(Return(false));
    EXPECT_CALL(*m_view, selectedRows())
        .Times(2)
        .WillRepeatedly(Return(selectedRows));
    EXPECT_CALL(*m_view, workspaceName(selectedRow))
        .Times(2)
        .WillRepeatedly(Return(m_wsName));
    EXPECT_CALL(*m_view, workspaceIndex(selectedRow))
        .Times(2)
        .WillRepeatedly(Return(m_wsIndex));
    EXPECT_CALL(*m_model,
                removeFunction(m_wsName, m_wsIndex, m_function->asString()))
        .Times(1);

    set_selection_changed_expectations(selectedRow, false, true);

    m_presenter->notifyPresenter(ViewEvent::FunctionRemoved,
                                 m_function->asString());
  }

  void
  test_that_FunctionAdded_will_clear_the_function_in_the_view_if_no_data_exists() {
    auto const selectedRows = std::vector<FitDomainIndex>{};

    ON_CALL(*m_view, isAddRemoveFunctionForAllChecked())
        .WillByDefault(Return(false));
    ON_CALL(*m_view, selectedRows()).WillByDefault(Return(selectedRows));

    EXPECT_CALL(*m_view, isAddRemoveFunctionForAllChecked)
        .Times(1)
        .WillOnce(Return(false));
    EXPECT_CALL(*m_view, selectedRows())
        .Times(1)
        .WillOnce(Return(selectedRows));
    EXPECT_CALL(
        *m_view,
        displayWarning("Data needs to be loaded before adding a function."))
        .Times(1);
    EXPECT_CALL(*m_view, clearFunction()).Times(1);

    m_presenter->notifyPresenter(ViewEvent::FunctionAdded,
                                 m_function->asString());
  }

  void
  test_that_FunctionAdded_will_add_the_function_in_the_relevant_domains_when_data_exists() {
    auto const selectedRow = FitDomainIndex(0);
    auto const selectedRows = std::vector<FitDomainIndex>{selectedRow};

    ON_CALL(*m_view, isAddRemoveFunctionForAllChecked())
        .WillByDefault(Return(false));
    ON_CALL(*m_view, selectedRows()).WillByDefault(Return(selectedRows));

    EXPECT_CALL(*m_view, isAddRemoveFunctionForAllChecked)
        .Times(1)
        .WillOnce(Return(false));
    EXPECT_CALL(*m_view, selectedRows())
        .Times(1)
        .WillOnce(Return(selectedRows));
    EXPECT_CALL(*m_view, workspaceName(selectedRow))
        .Times(1)
        .WillOnce(Return(m_wsName));
    EXPECT_CALL(*m_view, workspaceIndex(selectedRow))
        .Times(1)
        .WillOnce(Return(m_wsIndex));
    EXPECT_CALL(*m_model,
                addFunction(m_wsName, m_wsIndex, m_function->asString()))
        .Times(1);

    m_presenter->notifyPresenter(ViewEvent::FunctionAdded,
                                 m_function->asString());
  }

  void
  test_that_FunctionReplaced_will_clear_the_function_in_the_view_if_no_data_exists() {
    auto const selectedRows = std::vector<FitDomainIndex>{};

    ON_CALL(*m_view, isAddRemoveFunctionForAllChecked())
        .WillByDefault(Return(false));
    ON_CALL(*m_view, selectedRows()).WillByDefault(Return(selectedRows));

    EXPECT_CALL(*m_view, isAddRemoveFunctionForAllChecked)
        .Times(1)
        .WillOnce(Return(false));
    EXPECT_CALL(*m_view, selectedRows())
        .Times(1)
        .WillOnce(Return(selectedRows));
    EXPECT_CALL(
        *m_view,
        displayWarning("Data needs to be loaded before adding a function."))
        .Times(1);
    EXPECT_CALL(*m_view, clearFunction()).Times(1);

    m_presenter->notifyPresenter(ViewEvent::FunctionReplaced,
                                 m_function->asString());
  }

  void
  test_that_FunctionReplaced_will_set_the_function_in_the_relevant_domains_when_data_exists() {
    auto const selectedRow = FitDomainIndex(0);
    auto const selectedRows = std::vector<FitDomainIndex>{selectedRow};

    ON_CALL(*m_view, isAddRemoveFunctionForAllChecked())
        .WillByDefault(Return(false));
    ON_CALL(*m_view, selectedRows()).WillByDefault(Return(selectedRows));
    ON_CALL(*m_view, workspaceName(selectedRow))
        .WillByDefault(Return(m_wsName));
    ON_CALL(*m_view, workspaceIndex(selectedRow))
        .WillByDefault(Return(m_wsIndex));

    EXPECT_CALL(*m_view, isAddRemoveFunctionForAllChecked)
        .Times(1)
        .WillOnce(Return(false));
    EXPECT_CALL(*m_view, selectedRows())
        .Times(1)
        .WillOnce(Return(selectedRows));
    EXPECT_CALL(*m_view, workspaceName(selectedRow))
        .Times(1)
        .WillOnce(Return(m_wsName));
    EXPECT_CALL(*m_view, workspaceIndex(selectedRow))
        .Times(1)
        .WillOnce(Return(m_wsIndex));
    EXPECT_CALL(*m_model,
                setFunction(m_wsName, m_wsIndex, m_function->asString()))
        .Times(1);

    m_presenter->notifyPresenter(ViewEvent::FunctionReplaced,
                                 m_function->asString());
  }

  void test_that_ParameterChanged_will_update_the_relevant_parameter_values() {
    std::string const parameter("A0");
    double parameterValue(1.0);
    auto const row = FitDomainIndex(0);
    auto const allRows = std::vector<FitDomainIndex>{row};

    ON_CALL(*m_view, parameterValue(parameter))
        .WillByDefault(Return(parameterValue));
    ON_CALL(*m_view, selectedRows()).WillByDefault(Return(allRows));
    ON_CALL(*m_view, allRows()).WillByDefault(Return(allRows));
    ON_CALL(*m_view, workspaceName(row)).WillByDefault(Return(m_wsName));
    ON_CALL(*m_view, workspaceIndex(row)).WillByDefault(Return(m_wsIndex));
    ON_CALL(*m_model,
            getEquivalentFunctionIndexForDomain(m_wsName, m_wsIndex, parameter))
        .WillByDefault(Return(parameter));

    EXPECT_CALL(*m_view, parameterValue(parameter)).Times(1);
    EXPECT_CALL(*m_view, allRows()).Times(1);
    EXPECT_CALL(*m_view, workspaceName(row))
        .Times(2)
        .WillRepeatedly(Return(m_wsName));
    EXPECT_CALL(*m_view, workspaceIndex(row))
        .Times(2)
        .WillRepeatedly(Return(m_wsIndex));
    EXPECT_CALL(*m_model, getEquivalentFunctionIndexForDomain(
                              m_wsName, m_wsIndex, parameter))
        .Times(1);
    EXPECT_CALL(*m_model, updateParameterValue(m_wsName, m_wsIndex, parameter,
                                               parameterValue))
        .Times(1);
    EXPECT_CALL(*m_view, selectedRows()).Times(1);

    set_selection_changed_expectations(row, false, true);

    m_presenter->notifyPresenter(ViewEvent::ParameterChanged, parameter);
  }

  void test_that_AttributeChanged_will_update_the_relevant_attribute_values() {
    std::string const attribute("NumDeriv");
    Mantid::API::IFunction::Attribute attributeValue(true);
    auto const row = FitDomainIndex(0);
    auto const allRows = std::vector<FitDomainIndex>{row};

    ON_CALL(*m_view, attributeValue(attribute))
        .WillByDefault(Return(attributeValue));
    ON_CALL(*m_view, allRows()).WillByDefault(Return(allRows));
    ON_CALL(*m_view, workspaceName(row)).WillByDefault(Return(m_wsName));
    ON_CALL(*m_view, workspaceIndex(row)).WillByDefault(Return(m_wsIndex));

    EXPECT_CALL(*m_view, attributeValue(attribute)).Times(1);
    EXPECT_CALL(*m_view, allRows()).Times(1);
    EXPECT_CALL(*m_view, workspaceName(row))
        .Times(1)
        .WillOnce(Return(m_wsName));
    EXPECT_CALL(*m_view, workspaceIndex(row))
        .Times(1)
        .WillOnce(Return(m_wsIndex));
    EXPECT_CALL(*m_model, updateAttributeValue(
                              m_wsName, m_wsIndex, attribute,
                              BoolAttributeValue(attributeValue.asBool())))
        .Times(1);

    m_presenter->notifyPresenter(ViewEvent::AttributeChanged, attribute);
  }

  void
  test_that_ParameterTieChanged_will_attempt_to_update_the_ties_in_the_model() {
    std::string const parameter("A0");
    std::string const tie("A1");
    auto const row = FitDomainIndex(0);
    auto const allRows = std::vector<FitDomainIndex>{row};
    auto const globalTies = std::vector<GlobalTie>{};

    ON_CALL(*m_view, allRows()).WillByDefault(Return(allRows));
    ON_CALL(*m_view, selectedRows()).WillByDefault(Return(allRows));
    ON_CALL(*m_model,
            getEquivalentFunctionIndexForDomain(m_wsName, m_wsIndex, parameter))
        .WillByDefault(Return(parameter));
    ON_CALL(*m_model, getEquivalentParameterTieForDomain(m_wsName, m_wsIndex,
                                                         parameter, tie))
        .WillByDefault(Return(tie));
    ON_CALL(*m_model, getGlobalTies()).WillByDefault(Return(globalTies));

    EXPECT_CALL(*m_view, allRows()).Times(1);
    EXPECT_CALL(*m_view, workspaceName(row))
        .Times(2)
        .WillRepeatedly(Return(m_wsName));
    EXPECT_CALL(*m_view, workspaceIndex(row))
        .Times(2)
        .WillRepeatedly(Return(m_wsIndex));
    EXPECT_CALL(*m_model, getEquivalentFunctionIndexForDomain(
                              m_wsName, m_wsIndex, parameter))
        .Times(1);
    EXPECT_CALL(*m_model, getEquivalentParameterTieForDomain(
                              m_wsName, m_wsIndex, parameter, tie))
        .Times(1);
    EXPECT_CALL(*m_model,
                updateParameterTie(m_wsName, m_wsIndex, parameter, tie))
        .Times(1);

    EXPECT_CALL(*m_model, getGlobalTies()).Times(1);
    EXPECT_CALL(*m_view, setGlobalTies(VectorSize(0u))).Times(1);

    EXPECT_CALL(*m_view, selectedRows()).Times(1);

    set_selection_changed_expectations(row, false, true);

    m_presenter->notifyPresenter(ViewEvent::ParameterTieChanged, parameter,
                                 tie);
  }

  void
  test_that_ParameterConstraintRemoved_will_attempt_to_remove_the_constraint_in_the_model() {
    std::string const parameter("A0");
    auto const row = FitDomainIndex(0);
    auto const allRows = std::vector<FitDomainIndex>{row};

    ON_CALL(*m_view, allRows()).WillByDefault(Return(allRows));
    ON_CALL(*m_view, selectedRows()).WillByDefault(Return(allRows));

    EXPECT_CALL(*m_view, allRows()).Times(1);
    EXPECT_CALL(*m_view, workspaceName(row))
        .Times(2)
        .WillRepeatedly(Return(m_wsName));
    EXPECT_CALL(*m_view, workspaceIndex(row))
        .Times(2)
        .WillRepeatedly(Return(m_wsIndex));
    EXPECT_CALL(*m_model,
                removeParameterConstraint(m_wsName, m_wsIndex, parameter))
        .Times(1);
    EXPECT_CALL(*m_view, selectedRows()).Times(1);

    set_selection_changed_expectations(row, false, true);

    m_presenter->notifyPresenter(ViewEvent::ParameterConstraintRemoved,
                                 parameter);
  }

  void
  test_that_ParameterConstraintChanged_will_attempt_to_update_the_ties_in_the_model() {
    std::string const functionIndex("");
    std::string const constraint("0<A0<1");
    auto const row = FitDomainIndex(0);
    auto const allRows = std::vector<FitDomainIndex>{row};

    ON_CALL(*m_view, allRows()).WillByDefault(Return(allRows));
    ON_CALL(*m_view, selectedRows()).WillByDefault(Return(allRows));
    ON_CALL(*m_model, getEquivalentFunctionIndexForDomain(m_wsName, m_wsIndex,
                                                          functionIndex))
        .WillByDefault(Return(functionIndex));

    EXPECT_CALL(*m_view, allRows()).Times(1);
    EXPECT_CALL(*m_view, workspaceName(row))
        .Times(2)
        .WillRepeatedly(Return(m_wsName));
    EXPECT_CALL(*m_view, workspaceIndex(row))
        .Times(2)
        .WillRepeatedly(Return(m_wsIndex));
    EXPECT_CALL(*m_model, getEquivalentFunctionIndexForDomain(
                              m_wsName, m_wsIndex, functionIndex))
        .Times(1);
    EXPECT_CALL(*m_model, updateParameterConstraint(m_wsName, m_wsIndex,
                                                    functionIndex, constraint))
        .Times(1);

    EXPECT_CALL(*m_view, selectedRows()).Times(1);

    set_selection_changed_expectations(row, false, true);

    m_presenter->notifyPresenter(ViewEvent::ParameterConstraintChanged,
                                 functionIndex, constraint);
  }

  void
  test_that_GlobalParametersChanged_updates_the_globals_stored_in_the_model() {
    std::string const globalParameter("A0");
    auto const globalsVector = std::vector<std::string>{globalParameter};
    auto const globals =
        std::vector<GlobalParameter>{GlobalParameter(globalParameter)};

    EXPECT_CALL(*m_model, setGlobalParameters(globalsVector)).Times(1);

    set_selection_changed_expectations(FitDomainIndex(0), false, false,
                                       globals);

    m_presenter->notifyPresenter(ViewEvent::GlobalParametersChanged,
                                 globalsVector);
  }

  void
  test_that_FittingModeChanged_will_update_the_fitting_mode_stored_by_the_model() {
    auto const fittingMode(FittingMode::SIMULTANEOUS);

    EXPECT_CALL(*m_model, setFittingMode(fittingMode)).Times(1);

    set_selection_changed_expectations(FitDomainIndex(0));

    m_presenter->notifyPresenter(ViewEvent::FittingModeChanged, fittingMode);
  }

  void
  test_that_setGlobalTies_will_set_the_global_ties_displayed_by_the_view() {
    auto const globalTies = std::vector<GlobalTie>{GlobalTie("f0.A0", "f1.A0")};

    EXPECT_CALL(*m_view, setGlobalTies(VectorSize(globalTies.size()))).Times(1);

    m_presenter->setGlobalTies(globalTies);
  }

  void
  test_that_setGlobalParameters_will_set_the_global_parameters_displayed_by_the_view() {
    auto const globalParameters =
        std::vector<GlobalParameter>{GlobalParameter("A0")};

    EXPECT_CALL(*m_view,
                setGlobalParameters(VectorSize(globalParameters.size())))
        .Times(1);

    m_presenter->setGlobalParameters(globalParameters);
  }

private:
  void set_selection_changed_expectations(
      FitDomainIndex selectedRow, bool noSelection = false,
      bool ignoreNameIndexRetrieval = false,
      std::vector<GlobalParameter> const &globals =
          std::vector<GlobalParameter>()) {
    auto selectedRows = std::vector<FitDomainIndex>{selectedRow};

    if (noSelection)
      selectedRows = std::vector<FitDomainIndex>{};

    ON_CALL(*m_view, selectedRows()).WillByDefault(Return(selectedRows));

    ON_CALL(*m_model, getFittingMode())
        .WillByDefault(Return(FittingMode::SEQUENTIAL));

    if (!selectedRows.empty()) {
      ON_CALL(*m_view, workspaceName(selectedRow))
          .WillByDefault(Return(m_wsName));
      ON_CALL(*m_view, workspaceIndex(selectedRow))
          .WillByDefault(Return(m_wsIndex));

      ON_CALL(*m_model, getFunction(m_wsName, m_wsIndex))
          .WillByDefault(Return(m_function));

      ON_CALL(*m_model, getGlobalParameters()).WillByDefault(Return(globals));

      if (!ignoreNameIndexRetrieval) {
        EXPECT_CALL(*m_view, selectedRows())
            .Times(1)
            .WillOnce(Return(selectedRows));
        EXPECT_CALL(*m_view, workspaceName(selectedRow))
            .Times(1)
            .WillOnce(Return(m_wsName));
        EXPECT_CALL(*m_view, workspaceIndex(selectedRow))
            .Times(1)
            .WillOnce(Return(m_wsIndex));
      }

      EXPECT_CALL(*m_model, getFittingMode()).Times(1);
      EXPECT_CALL(*m_view, setSimultaneousMode(false)).Times(1);

      EXPECT_CALL(*m_model, getFunction(m_wsName, m_wsIndex)).Times(1);
      EXPECT_CALL(*m_view, setFunction(m_function)).Times(1);

      EXPECT_CALL(*m_model, getGlobalParameters()).Times(1);
      EXPECT_CALL(*m_view, setGlobalParameters(VectorSize(globals.size())))
          .Times(1);
    } else {
      EXPECT_CALL(*m_view, selectedRows())
          .Times(1)
          .WillOnce(Return(selectedRows));

      EXPECT_CALL(*m_model, getFittingMode()).Times(1);
      EXPECT_CALL(*m_view, setSimultaneousMode(false)).Times(1);

      EXPECT_CALL(*m_view, clearFunction()).Times(1);
    }
  }

  std::string m_wsName;
  MantidQt::MantidWidgets::WorkspaceIndex m_wsIndex;
  Mantid::API::MatrixWorkspace_sptr m_workspace;
  double m_startX;
  double m_endX;
  Mantid::API::IFunction_sptr m_function;

  std::unique_ptr<MockFitScriptGeneratorView> m_view;
  std::unique_ptr<MockFitScriptGeneratorModel> m_model;
  std::unique_ptr<FitScriptGeneratorPresenter> m_presenter;
};
