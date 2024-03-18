// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "Analysis/DataAnalysisTab.h"
#include "Analysis/FitOutputOptionsPresenter.h"
#include "Analysis/FitOutputOptionsView.h"
#include "Analysis/IFitOutputOptionsModel.h"
#include "Analysis/IFitOutputOptionsView.h"
#include "MockObjects.h"

#include "MantidFrameworkTestHelpers/FitDataCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::FitDataCreationHelper;
using namespace MantidQt::CustomInterfaces::IDA;
using namespace testing;

namespace {

std::vector<std::string> getThreeParameters() { return {"Amplitude", "HWHM", "PeakCentre"}; }

} // namespace

class FitOutputOptionsPresenterTest : public CxxTest::TestSuite {
public:
  static FitOutputOptionsPresenterTest *createSuite() { return new FitOutputOptionsPresenterTest(); }

  static void destroySuite(FitOutputOptionsPresenterTest *suite) { delete suite; }

  void setUp() override {
    m_tab = std::make_unique<NiceMock<MockDataAnalysisTab>>();
    m_view = std::make_unique<NiceMock<MockFitOutputOptionsView>>();
    auto model = std::make_unique<NiceMock<MockFitOutputOptionsModel>>();
    m_model = model.get();

    m_presenter = std::make_unique<FitOutputOptionsPresenter>(m_tab.get(), m_view.get(), std::move(model));
  }

  void tearDown() override {
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_view.get()));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_model));

    m_view.reset();
    m_presenter.reset(); /// The model is destructed by the presenter
  }

  ///----------------------------------------------------------------------
  /// Unit tests to check for successful presenter instantiation
  ///----------------------------------------------------------------------

  void test_that_the_presenter_has_been_instantiated() {
    TS_ASSERT(m_view);
    TS_ASSERT(m_model);
    TS_ASSERT(m_presenter);
  }

  void test_that_calling_a_presenter_method_will_invoke_the_relevant_model_and_view_methods() {
    std::string const selectedGroup("Result Group");

    EXPECT_CALL(*m_view, clearPlotTypes()).Times(1);
    EXPECT_CALL(*m_model, getWorkspaceParameters(selectedGroup)).Times(1);

    m_presenter->setPlotTypes(selectedGroup);
  }

  ///----------------------------------------------------------------------
  /// Unit Tests that test the views signals invoke the correct methods
  ///----------------------------------------------------------------------

  void
  test_that_handleGroupWorkspaceChanged_will_check_the_group_selected_before_setting_the_workspace_combobox_visibility() {
    std::string const selectedGroup("Result Group");
    auto const isResultGroup(true);

    ON_CALL(*m_model, isResultGroupSelected(selectedGroup)).WillByDefault(Return(isResultGroup));

    EXPECT_CALL(*m_model, isResultGroupSelected(selectedGroup)).Times(1).WillOnce(Return(isResultGroup));
    EXPECT_CALL(*m_view, setWorkspaceComboBoxVisible(!isResultGroup)).Times(1);

    m_presenter->handleGroupWorkspaceChanged(selectedGroup);
  }

  void test_that_handleFroupWorkspaceChanged_will_check_the_resultGroup_plottablity_before_calling_setPlotEnabled() {
    std::string const selectedGroup("Result Group");
    auto const isPlottable(true);

    ON_CALL(*m_view, getSelectedGroupWorkspace()).WillByDefault(Return(selectedGroup));
    ON_CALL(*m_model, isSelectedGroupPlottable(selectedGroup)).WillByDefault(Return(isPlottable));

    EXPECT_CALL(*m_model, isSelectedGroupPlottable(selectedGroup)).Times(1).WillOnce(Return(isPlottable));
    EXPECT_CALL(*m_view, setPlotEnabled(isPlottable)).Times(1);

    m_presenter->handleGroupWorkspaceChanged(selectedGroup);
  }

  void test_that_handleGroupWorkspaceChanged_will_check_the_pdfGroup_plottablity_before_calling_setPlotEnabled() {
    std::string const selectedGroup("PDF Group");
    auto const isPlottable(true);

    ON_CALL(*m_view, getSelectedGroupWorkspace()).WillByDefault(Return(selectedGroup));
    ON_CALL(*m_model, isResultGroupSelected(selectedGroup)).WillByDefault(Return(false));
    ON_CALL(*m_model, isSelectedGroupPlottable(selectedGroup)).WillByDefault(Return(isPlottable));

    EXPECT_CALL(*m_model, isSelectedGroupPlottable(selectedGroup)).Times(1).WillOnce(Return(isPlottable));
    EXPECT_CALL(*m_view, setPlotEnabled(isPlottable)).Times(1);

    m_presenter->handleGroupWorkspaceChanged(selectedGroup);
  }

  void test_that_handleGroupWorkspaceChanged_will_try_and_set_the_plot_types_in_the_plot_types_combobox() {
    std::string const selectedGroup("Result Group");
    auto const parameters(getThreeParameters());

    ON_CALL(*m_model, getWorkspaceParameters(selectedGroup)).WillByDefault(Return(parameters));

    EXPECT_CALL(*m_view, clearPlotTypes()).Times(1);
    EXPECT_CALL(*m_model, getWorkspaceParameters(selectedGroup)).Times(1).WillOnce(Return(parameters));
    EXPECT_CALL(*m_view, setAvailablePlotTypes(parameters)).Times(1);

    m_presenter->handleGroupWorkspaceChanged(selectedGroup);
  }

  void test_that_handlePlotClicked_will_invoke_plotResult_if_the_selected_group_is_the_result_group() {
    std::string const selectedGroup("Result Group");
    std::string const plotType("All");

    ON_CALL(*m_view, getSelectedGroupWorkspace()).WillByDefault(Return(selectedGroup));
    ON_CALL(*m_model, isResultGroupSelected(selectedGroup)).WillByDefault(Return(true));
    ON_CALL(*m_view, getSelectedPlotType()).WillByDefault(Return(plotType));

    ExpectationSet getSelectedWorkspace =
        EXPECT_CALL(*m_view, getSelectedGroupWorkspace()).Times(1).WillOnce(Return(selectedGroup));
    getSelectedWorkspace += EXPECT_CALL(*m_model, isResultGroupSelected(selectedGroup)).Times(1);
    EXPECT_CALL(*m_model, plotResult(plotType)).Times(1).After(getSelectedWorkspace);

    m_presenter->handlePlotClicked();
  }

  void test_that_handlePlotClicked_will_invoke_plotPDF_if_the_selected_group_is_the_pdf_group() {
    std::string const selectedGroup("PDF Group");
    std::string const plotType("All");

    ON_CALL(*m_view, getSelectedGroupWorkspace()).WillByDefault(Return(selectedGroup));
    ON_CALL(*m_model, isResultGroupSelected(selectedGroup)).WillByDefault(Return(false));
    ON_CALL(*m_view, getSelectedPlotType()).WillByDefault(Return(plotType));

    ExpectationSet getSelectedWorkspace =
        EXPECT_CALL(*m_view, getSelectedGroupWorkspace()).Times(1).WillOnce(Return(selectedGroup));
    getSelectedWorkspace += EXPECT_CALL(*m_model, isResultGroupSelected(selectedGroup)).Times(1);
    EXPECT_CALL(*m_model, plotPDF("", plotType)).Times(1).After(getSelectedWorkspace);

    m_presenter->handlePlotClicked();
  }

  void test_that_handleSaveClicked_will_try_to_disable_and_then_enable_the_save_and_plot_buttons() {
    auto const selectedGroup("Result Group");

    ON_CALL(*m_view, getSelectedGroupWorkspace()).WillByDefault(Return(selectedGroup));
    ON_CALL(*m_model, isSelectedGroupPlottable(selectedGroup)).WillByDefault(Return(true));

    ExpectationSet buttonEnabling = EXPECT_CALL(*m_view, setSaveText("Saving...")).Times(1);
    buttonEnabling += EXPECT_CALL(*m_view, setPlotEnabled(false)).Times(1);
    buttonEnabling += EXPECT_CALL(*m_view, setSaveEnabled(false)).Times(1);
    buttonEnabling += EXPECT_CALL(*m_view, setSaveText("Save Result")).Times(1);
    buttonEnabling += EXPECT_CALL(*m_view, setPlotEnabled(true)).Times(1);
    EXPECT_CALL(*m_view, setSaveEnabled(true)).Times(1).After(buttonEnabling);

    m_presenter->handleSaveClicked();
  }

  void test_that_handleSaveClicked_will_invoke_saveResult_in_the_model() {
    EXPECT_CALL(*m_model, saveResult()).Times(1);
    m_presenter->handleSaveClicked();
  }

  ///----------------------------------------------------------------------
  /// Unit Tests that test the methods of the presenter
  ///----------------------------------------------------------------------

  void test_that_setResultWorkspace_will_invoke_setResultWorkspace_in_the_model() {
    auto const groupWorkspace = createGroupWorkspaceWithTextAxes(2, getThreeParameters(), 3);

    EXPECT_CALL(*m_model, setResultWorkspace(groupWorkspace)).Times(1);

    m_presenter->setResultWorkspace(groupWorkspace);
  }

  void test_that_setPDFWorkspace_will_invoke_setPDFWorkspace_in_the_model() {
    auto const groupWorkspace = createGroupWorkspaceWithTextAxes(2, getThreeParameters(), 3);

    EXPECT_CALL(*m_model, setPDFWorkspace(groupWorkspace)).Times(1);

    m_presenter->setPDFWorkspace(groupWorkspace);
  }

  void
  test_that_setPlotWorkspaces_will_set_the_availaible_plot_workspaces_if_names_are_returned_from_getPDFWorkspaceNames() {
    std::vector<std::string> const workspaceNames{"Name1", "Name2"};

    ON_CALL(*m_model, getPDFWorkspaceNames()).WillByDefault(Return(workspaceNames));

    ExpectationSet setAvailableWorkspaces = EXPECT_CALL(*m_view, clearPlotWorkspaces()).Times(1);
    setAvailableWorkspaces += EXPECT_CALL(*m_view, setAvailablePlotWorkspaces(workspaceNames)).Times(1);
    EXPECT_CALL(*m_view, setPlotWorkspacesIndex(0)).Times(1).After(setAvailableWorkspaces);

    m_presenter->setPlotWorkspaces();
  }

  void
  test_that_setPlotTypes_will_set_the_availaible_plot_types_if_parameters_are_returned_from_getWorkspaceParameters() {
    auto const selectedGroup("Result Group");
    auto const parameters(getThreeParameters());

    ON_CALL(*m_model, getWorkspaceParameters(selectedGroup)).WillByDefault(Return(parameters));

    ExpectationSet setAvailablePlotTypes = EXPECT_CALL(*m_view, clearPlotTypes()).Times(1);
    setAvailablePlotTypes += EXPECT_CALL(*m_view, setAvailablePlotTypes(parameters)).Times(1);
    EXPECT_CALL(*m_view, setPlotTypeIndex(0)).Times(1).After(setAvailablePlotTypes);

    m_presenter->setPlotTypes(selectedGroup);
  }

  void test_that_removePDFWorkspace_will_invoke_removePDFWorkspace_in_the_model() {
    EXPECT_CALL(*m_model, removePDFWorkspace()).Times(1);
    m_presenter->removePDFWorkspace();
  }

  void test_that_isSelectedGroupPlottable_will_invoke_isSelectedGroupPlottable_in_the_model() {
    std::string const selectedGroup("Result Group");
    ON_CALL(*m_view, getSelectedGroupWorkspace()).WillByDefault(Return(selectedGroup));

    EXPECT_CALL(*m_model, isSelectedGroupPlottable(selectedGroup)).Times(1);

    m_presenter->isSelectedGroupPlottable();
  }

  void test_that_setPlotting_will_attempt_to_set_the_plot_button_text_and_disable_all_buttons_when_passed_true() {
    auto const isPlotting(true);

    EXPECT_CALL(*m_view, setPlotText("Plotting...")).Times(1);
    EXPECT_CALL(*m_view, setPlotEnabled(!isPlotting)).Times(1);
    EXPECT_CALL(*m_view, setEditResultEnabled(!isPlotting)).Times(1);
    EXPECT_CALL(*m_view, setSaveEnabled(!isPlotting)).Times(1);

    m_presenter->setPlotting(isPlotting);
  }

  void test_that_setPlotting_will_attempt_to_set_the_plot_button_text_and_enable_all_buttons_when_passed_false() {
    auto const isPlotting(false);
    auto const selectedGroup("Result Group");

    ON_CALL(*m_view, getSelectedGroupWorkspace()).WillByDefault(Return(selectedGroup));
    ON_CALL(*m_model, isSelectedGroupPlottable(selectedGroup)).WillByDefault(Return(true));

    EXPECT_CALL(*m_view, setPlotText("Plot")).Times(1);
    EXPECT_CALL(*m_view, setPlotEnabled(!isPlotting)).Times(1);
    EXPECT_CALL(*m_view, setEditResultEnabled(!isPlotting)).Times(1);
    EXPECT_CALL(*m_view, setSaveEnabled(!isPlotting)).Times(1);

    m_presenter->setPlotting(isPlotting);
  }

  void test_that_setPlotEnabled_will_invoke_setPlotEnabled_in_the_view() {
    auto const selectedGroup("Result Group");
    ON_CALL(*m_view, getSelectedGroupWorkspace()).WillByDefault(Return(selectedGroup));
    ON_CALL(*m_model, isSelectedGroupPlottable(selectedGroup)).WillByDefault(Return(true));

    EXPECT_CALL(*m_view, setPlotEnabled(true)).Times(1);
    m_presenter->setPlotEnabled(true);
  }

  void test_that_setPlotEnabled_will_disable_the_plot_options_if_the_selected_workspace_is_not_plottable() {
    auto const selectedGroup("Result Group");
    ON_CALL(*m_view, getSelectedGroupWorkspace()).WillByDefault(Return(selectedGroup));
    ON_CALL(*m_model, isSelectedGroupPlottable(selectedGroup)).WillByDefault(Return(false));

    EXPECT_CALL(*m_view, setPlotEnabled(false)).Times(1);
    m_presenter->setPlotEnabled(true);
  }

  void test_that_setEditResultEnabled_will_invoke_setEditResultEnabled_in_the_view() {
    EXPECT_CALL(*m_view, setEditResultEnabled(true)).Times(1);
    m_presenter->setEditResultEnabled(true);
  }

  void test_that_setSaveEnabled_will_invoke_setSaveEnabled_in_the_view() {
    EXPECT_CALL(*m_view, setSaveEnabled(true)).Times(1);
    m_presenter->setSaveEnabled(true);
  }

  void test_that_clearSpectraToPlot_will_invoke_clearSpectraToPlot_in_the_model() {
    EXPECT_CALL(*m_model, clearSpectraToPlot()).Times(1);
    m_presenter->clearSpectraToPlot();
  }

  void test_that_getSpectraToPlot_will_invoke_getSpectraToPlot_in_the_model() {
    EXPECT_CALL(*m_model, getSpectraToPlot()).Times(1);
    m_presenter->getSpectraToPlot();
  }

  void test_that_setEditResultVisible_will_invoke_setEditResultVisible_in_the_view() {
    EXPECT_CALL(*m_view, setEditResultVisible(true)).Times(1);
    m_presenter->setEditResultVisible(true);
  }

private:
  std::unique_ptr<NiceMock<MockDataAnalysisTab>> m_tab;
  std::unique_ptr<NiceMock<MockFitOutputOptionsView>> m_view;
  NiceMock<MockFitOutputOptionsModel> *m_model;
  std::unique_ptr<FitOutputOptionsPresenter> m_presenter;
};