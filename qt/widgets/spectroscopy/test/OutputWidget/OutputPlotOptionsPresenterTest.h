// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidQtWidgets/Spectroscopy/MockObjects.h"
#include "MantidQtWidgets/Spectroscopy/OutputWidget/OutputPlotOptionsPresenter.h"

#include "MantidKernel/WarningSuppressions.h"

using namespace MantidQt::CustomInterfaces;
using namespace testing;

namespace {

std::string const WORKSPACE_NAME = "WorkspaceName";
std::string const WORKSPACE_INDICES = "0-2,4";

std::map<std::string, std::string>
constructActions(std::optional<std::map<std::string, std::string>> const &availableActions) {
  std::map<std::string, std::string> actions;
  if (availableActions)
    actions = *availableActions;
  if (actions.find("Plot Spectra") == actions.end())
    actions["Plot Spectra"] = "Plot Spectra";
  if (actions.find("Plot Bins") == actions.end())
    actions["Plot Bins"] = "Plot Bins";
  if (actions.find("Open Slice Viewer") == actions.end())
    actions["Open Slice Viewer"] = "Open Slice Viewer";
  if (actions.find("Plot Tiled") == actions.end())
    actions["Plot Tiled"] = "Plot Tiled";
  if (actions.find("Plot 3D Surface") == actions.end())
    actions["Plot 3D Surface"] = "Plot 3D Surface";
  return actions;
}

} // namespace

class OutputPlotOptionsPresenterTest : public CxxTest::TestSuite {
public:
  static OutputPlotOptionsPresenterTest *createSuite() { return new OutputPlotOptionsPresenterTest(); }

  static void destroySuite(OutputPlotOptionsPresenterTest *suite) { delete suite; }

  void setUp() override {
    m_view = std::make_unique<NiceMock<MockOutputPlotOptionsView>>();
    auto model = std::make_unique<NiceMock<MockOutputPlotOptionsModel>>();
    m_model = model.get();

    m_presenter = std::make_unique<OutputPlotOptionsPresenter>(m_view.get(), std::move(model));
  }

  void tearDown() override {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_model));

    m_presenter.reset(); /// The model is destructed by the presenter
    m_view.reset();
  }

  ///----------------------------------------------------------------------
  /// Unit tests to check for successful presenter instantiation
  ///----------------------------------------------------------------------

  void test_that_the_presenter_has_been_instantiated() {
    TS_ASSERT(m_view);
    TS_ASSERT(m_model);
    TS_ASSERT(m_presenter);
  }

  void test_that_the_expected_setup_is_performed_when_instantiating_the_presenter() {
    tearDown();
    m_view = std::make_unique<NiceMock<MockOutputPlotOptionsView>>();
    auto model = std::make_unique<NiceMock<MockOutputPlotOptionsModel>>();
    m_model = model.get();

    auto actions = constructActions(std::nullopt);
    EXPECT_CALL(*m_view, setIndicesRegex(_)).Times(1);
    ON_CALL(*m_model, availableActions()).WillByDefault(Return(actions));
    EXPECT_CALL(*m_view, setPlotType(PlotWidget::Spectra, actions)).Times(1);
    EXPECT_CALL(*m_view, setIndices(QString(""))).Times(1);
    EXPECT_CALL(*m_model, setFixedIndices("")).Times(1);

    m_presenter = std::make_unique<OutputPlotOptionsPresenter>(m_view.get(), std::move(model));
  }

  ///----------------------------------------------------------------------
  /// Unit Tests that test the signals emitted from the view
  ///----------------------------------------------------------------------

  void test_that_notifyWorkspaceChanged_set_the_workspace_stored_by_the_model() {
    EXPECT_CALL(*m_model, setWorkspace(WORKSPACE_NAME)).Times(1);
    m_presenter->handleWorkspaceChanged(WORKSPACE_NAME);
  }

  void test_that_the_view_widgets_are_enabled_when_the_workspace_being_set_in_the_model_is_valid() {
    ON_CALL(*m_model, setWorkspace(WORKSPACE_NAME)).WillByDefault(Return(true));

    setExpectationsForWidgetEnabling(true);

    m_presenter->handleWorkspaceChanged(WORKSPACE_NAME);
  }

  void test_that_the_view_widgets_are_disabled_when_the_workspace_being_set_in_the_model_is_invalid() {
    ON_CALL(*m_model, setWorkspace(WORKSPACE_NAME)).WillByDefault(Return(false));

    setExpectationsForWidgetEnabling(false);

    m_presenter->handleWorkspaceChanged(WORKSPACE_NAME);
  }

  void test_that_the_indices_are_formatted_when_they_are_changed_before_being_set_in_the_view_and_model() {
    ON_CALL(*m_model, formatIndices(WORKSPACE_INDICES)).WillByDefault(Return(WORKSPACE_INDICES));
    ON_CALL(*m_model, setIndices(WORKSPACE_INDICES)).WillByDefault(Return(true));

    EXPECT_CALL(*m_model, formatIndices(WORKSPACE_INDICES)).Times(1);
    EXPECT_CALL(*m_view, setIndices(QString::fromStdString(WORKSPACE_INDICES))).Times(1);
    EXPECT_CALL(*m_model, setIndices(WORKSPACE_INDICES)).Times(1);
    EXPECT_CALL(*m_view, setIndicesErrorLabelVisible(false)).Times(1);

    m_presenter->handleSelectedIndicesChanged(WORKSPACE_INDICES);
  }

  void test_that_the_indicesErrorLabel_is_set_to_visible_when_the_indices_are_invalid() {
    ON_CALL(*m_model, formatIndices(WORKSPACE_INDICES)).WillByDefault(Return(WORKSPACE_INDICES));
    ON_CALL(*m_model, setIndices(WORKSPACE_INDICES)).WillByDefault(Return(false));

    EXPECT_CALL(*m_model, formatIndices(WORKSPACE_INDICES)).Times(1);
    EXPECT_CALL(*m_model, setIndices(WORKSPACE_INDICES)).Times(1);
    EXPECT_CALL(*m_view, setIndicesErrorLabelVisible(true)).Times(1);

    m_presenter->handleSelectedIndicesChanged(WORKSPACE_INDICES);
  }

  void test_that_a_new_indice_suggestion_is_set_when_the_formatted_indices_are_not_empty() {
    ON_CALL(*m_model, formatIndices(WORKSPACE_INDICES)).WillByDefault(Return(WORKSPACE_INDICES));

    EXPECT_CALL(*m_model, formatIndices(WORKSPACE_INDICES)).Times(1);
    EXPECT_CALL(*m_view, addIndicesSuggestion(QString::fromStdString(WORKSPACE_INDICES))).Times(1);

    m_presenter->handleSelectedIndicesChanged(WORKSPACE_INDICES);
  }

  void test_that_a_new_indice_suggestion_is_not_set_when_the_formatted_indices_are_empty() {
    ON_CALL(*m_model, formatIndices(WORKSPACE_INDICES)).WillByDefault(Return(""));

    EXPECT_CALL(*m_model, formatIndices("")).Times(1);
    EXPECT_CALL(*m_view, addIndicesSuggestion(QString::fromStdString(""))).Times(0);

    m_presenter->handleSelectedIndicesChanged("");
  }

  void test_that_the_plotSpectraClicked_signal_will_attempt_to_plot_the_spectra() {
    setExpectationsForWidgetEnabling(false);
    EXPECT_CALL(*m_model, plotSpectra()).Times(1);
    setExpectationsForWidgetEnabling(true);

    m_presenter->handlePlotSpectraClicked();
  }

  void test_that_the_plotBinsClicked_signal_will_attempt_to_plot_the_bins_when_the_bin_indices_are_valid() {
    ON_CALL(*m_model, validateIndices(_, MantidAxis::Bin)).WillByDefault(Return(true));

    setExpectationsForWidgetEnabling(false);
    EXPECT_CALL(*m_model, plotBins(_)).Times(1);
    setExpectationsForWidgetEnabling(true);

    m_presenter->handlePlotBinsClicked();
  }

  void test_that_the_plotBinsClicked_signal_will_display_a_warning_message_if_the_bin_indices_are_invalid() {
    ON_CALL(*m_model, validateIndices(_, MantidAxis::Bin)).WillByDefault(Return(false));

    EXPECT_CALL(*m_view, displayWarning(QString("Plot Bins failed: Invalid bin indices provided."))).Times(1);

    m_presenter->handlePlotBinsClicked();
  }

  void test_that_the_showSliceViewerClicked_signal_will_attempt_to_show_slice_viewer() {
    setExpectationsForWidgetEnabling(false);
    EXPECT_CALL(*m_model, showSliceViewer()).Times(1);
    setExpectationsForWidgetEnabling(true);

    m_presenter->handleShowSliceViewerClicked();
  }

  void test_that_the_plotTiledClicked_signal_will_attempt_to_plot_tiled_spectra() {
    setExpectationsForWidgetEnabling(false);
    EXPECT_CALL(*m_model, plotTiled()).Times(1);
    setExpectationsForWidgetEnabling(true);

    m_presenter->handlePlotTiledClicked();
  }

  void test_that_the_plot3DdClicked_signal_will_attempt_to_plot_3D_Surface() {
    setExpectationsForWidgetEnabling(false);
    EXPECT_CALL(*m_model, plot3DSurface()).Times(1);
    setExpectationsForWidgetEnabling(true);

    m_presenter->handlePlot3DClicked();
  }

  ///----------------------------------------------------------------------
  /// Unit Tests that test the public member functions of the presenter
  ///----------------------------------------------------------------------

  void test_setPlotType_sets_the_view() {
    auto actions = constructActions(std::nullopt);
    ON_CALL(*m_model, availableActions()).WillByDefault(Return(actions));
    EXPECT_CALL(*m_view, setPlotType(PlotWidget::Spectra, actions)).Times(1);
    m_presenter->setPlotType(PlotWidget::Spectra);
  }

  void test_that_setWorkspaces_will_set_the_workspaces_in_the_view_and_model() {
    std::vector<std::string> const workspaceNames{WORKSPACE_NAME};
    ON_CALL(*m_model, getAllWorkspaceNames(workspaceNames)).WillByDefault(Return(workspaceNames));

    EXPECT_CALL(*m_view, setWorkspaces(workspaceNames)).Times(1);
    EXPECT_CALL(*m_model, setWorkspace(WORKSPACE_NAME)).Times(1);

    m_presenter->setWorkspaces(workspaceNames);
  }

  void test_that_clearWorkspaces_will_clear_the_workspaces_in_the_view_and_model() {
    EXPECT_CALL(*m_view, clearWorkspaces()).Times(1);
    EXPECT_CALL(*m_model, removeWorkspace()).Times(1);

    m_presenter->clearWorkspaces();
  }

  void test_that_clearWorkspaces_will_disable_the_widgets() {
    setExpectationsForWidgetEnabling(false);
    m_presenter->clearWorkspaces();
  }

private:
  void setExpectationsForWidgetEnabling(bool enabled) {
    ON_CALL(*m_view, numberOfWorkspaces()).WillByDefault(Return(2));
    ON_CALL(*m_model, indicesFixed()).WillByDefault(Return(false));

    EXPECT_CALL(*m_view, setWorkspaceComboBoxEnabled(enabled)).Times(1);
    EXPECT_CALL(*m_view, setIndicesLineEditEnabled(enabled)).Times(1);
    EXPECT_CALL(*m_view, setPlotButtonEnabled(enabled)).Times(1);
  }

  std::unique_ptr<NiceMock<MockOutputPlotOptionsView>> m_view;
  NiceMock<MockOutputPlotOptionsModel> *m_model;
  std::unique_ptr<OutputPlotOptionsPresenter> m_presenter;
};
