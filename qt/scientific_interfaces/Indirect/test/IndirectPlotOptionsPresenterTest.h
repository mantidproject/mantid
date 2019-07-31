// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_INDIRECTPLOTOPTIONSPRESENTERTEST_H_
#define MANTIDQT_INDIRECTPLOTOPTIONSPRESENTERTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "IndirectPlotOptionsPresenter.h"

#include "MantidKernel/WarningSuppressions.h"

using namespace MantidQt::CustomInterfaces;
using namespace testing;

namespace {

std::string const WORKSPACE_NAME = "WorkspaceName";
std::string const WORKSPACE_INDICES = "0-2,4";

std::map<std::string, std::string>
constructActions(boost::optional<std::map<std::string, std::string>> const
                     &availableActions) {
  std::map<std::string, std::string> actions;
  if (availableActions)
    actions = availableActions.get();
  if (actions.find("Plot Spectra") == actions.end())
    actions["Plot Spectra"] = "Plot Spectra";
  if (actions.find("Plot Bins") == actions.end())
    actions["Plot Bins"] = "Plot Bins";
  if (actions.find("Plot Contour") == actions.end())
    actions["Plot Contour"] = "Plot Contour";
  if (actions.find("Plot Tiled") == actions.end())
    actions["Plot Tiled"] = "Plot Tiled";
  return actions;
}

} // namespace

GNU_DIAG_OFF_SUGGEST_OVERRIDE

/// Mock object to mock the view
class MockIndirectPlotOptionsView : public IndirectPlotOptionsView {
public:
  void emitSelectedWorkspaceChanged(std::string const &workspaceName) {
    emit selectedWorkspaceChanged(workspaceName);
  }

  void emitSelectedIndicesChanged(std::string const &indices) {
    emit selectedIndicesChanged(indices);
  }

  void emitPlotSpectraClicked() { emit plotSpectraClicked(); }

  void emitPlotBinsClicked() { emit plotBinsClicked(); }

  void emitPlotContourClicked() { emit plotContourClicked(); }

  void emitPlotTiledClicked() { emit plotTiledClicked(); }

  /// Public Methods
  MOCK_METHOD2(
      setPlotType,
      void(PlotWidget const &plotType,
           std::map<std::string, std::string> const &availableActions));

  MOCK_METHOD1(setIndicesRegex, void(QString const &regex));

  MOCK_METHOD1(setWorkspaces, void(std::vector<std::string> const &workspaces));
  MOCK_METHOD0(clearWorkspaces, void());

  MOCK_METHOD1(setIndices, void(QString const &indices));
  MOCK_METHOD1(setIndicesErrorLabelVisible, void(bool visible));

  MOCK_METHOD1(setWorkspaceComboBoxEnabled, void(bool enable));
  MOCK_METHOD1(setIndicesLineEditEnabled, void(bool enable));
  MOCK_METHOD1(setPlotButtonEnabled, void(bool enable));

  MOCK_CONST_METHOD0(numberOfWorkspaces, int());

  MOCK_METHOD1(addIndicesSuggestion, void(QString const &spectra));

  MOCK_METHOD1(displayWarning, void(QString const &message));
};

/// Mock object to mock an IndirectTab
class MockIndirectPlotOptionsModel : public IndirectPlotOptionsModel {
public:
  /// Public Methods
  MOCK_METHOD1(setWorkspace, bool(std::string const &workspaceName));
  MOCK_METHOD0(removeWorkspace, void());

  MOCK_CONST_METHOD1(
      getAllWorkspaceNames,
      std::vector<std::string>(std::vector<std::string> const &workspaceNames));

  MOCK_METHOD1(setFixedIndices, void(std::string const &indices));
  MOCK_CONST_METHOD0(indicesFixed, bool());

  MOCK_CONST_METHOD1(formatIndices, std::string(std::string const &indices));
  MOCK_CONST_METHOD2(validateIndices, bool(std::string const &indices,
                                           MantidAxis const &axisType));
  MOCK_METHOD1(setIndices, bool(std::string const &indices));

  MOCK_METHOD0(plotSpectra, void());
  MOCK_METHOD1(plotBins, void(std::string const &binIndices));
  MOCK_METHOD0(plotContour, void());
  MOCK_METHOD0(plotTiled, void());
};

GNU_DIAG_ON_SUGGEST_OVERRIDE

class IndirectPlotOptionsPresenterTest : public CxxTest::TestSuite {
public:
  static IndirectPlotOptionsPresenterTest *createSuite() {
    return new IndirectPlotOptionsPresenterTest();
  }

  static void destroySuite(IndirectPlotOptionsPresenterTest *suite) {
    delete suite;
  }

  void setUp() override {
    m_view = std::make_unique<NiceMock<MockIndirectPlotOptionsView>>();
    m_model = new NiceMock<MockIndirectPlotOptionsModel>();

    m_presenter =
        std::make_unique<IndirectPlotOptionsPresenter>(m_view.get(), m_model);
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

  void
  test_that_the_expected_setup_is_performed_when_instantiating_the_presenter() {
    tearDown();
    m_view = std::make_unique<NiceMock<MockIndirectPlotOptionsView>>();
    m_model = new NiceMock<MockIndirectPlotOptionsModel>();

    EXPECT_CALL(*m_view, setIndicesRegex(_)).Times(1);
    EXPECT_CALL(*m_view,
                setPlotType(PlotWidget::Spectra, constructActions(boost::none)))
        .Times(1);
    EXPECT_CALL(*m_view, setIndices(QString(""))).Times(1);
    EXPECT_CALL(*m_model, setFixedIndices("")).Times(1);

    m_presenter =
        std::make_unique<IndirectPlotOptionsPresenter>(m_view.get(), m_model);
  }

  ///----------------------------------------------------------------------
  /// Unit Tests that test the signals emitted from the view
  ///----------------------------------------------------------------------

  void
  test_that_the_workspace_stored_by_the_model_is_changed_when_it_is_altered_in_the_view() {
    EXPECT_CALL(*m_model, setWorkspace(WORKSPACE_NAME)).Times(1);
    m_view->emitSelectedWorkspaceChanged(WORKSPACE_NAME);
  }

  void
  test_that_the_view_widgets_are_enabled_when_the_workspace_being_set_in_the_model_is_valid() {
    ON_CALL(*m_model, setWorkspace(WORKSPACE_NAME)).WillByDefault(Return(true));

    setExpectationsForWidgetEnabling(true);

    m_view->emitSelectedWorkspaceChanged(WORKSPACE_NAME);
  }

  void
  test_that_the_view_widgets_are_disabled_when_the_workspace_being_set_in_the_model_is_invalid() {
    ON_CALL(*m_model, setWorkspace(WORKSPACE_NAME))
        .WillByDefault(Return(false));

    setExpectationsForWidgetEnabling(false);

    m_view->emitSelectedWorkspaceChanged(WORKSPACE_NAME);
  }

  void
  test_that_the_indices_are_formatted_when_they_are_changed_before_being_set_in_the_view_and_model() {
    ON_CALL(*m_model, formatIndices(WORKSPACE_INDICES))
        .WillByDefault(Return(WORKSPACE_INDICES));
    ON_CALL(*m_model, setIndices(WORKSPACE_INDICES))
        .WillByDefault(Return(true));

    EXPECT_CALL(*m_model, formatIndices(WORKSPACE_INDICES)).Times(1);
    EXPECT_CALL(*m_view, setIndices(QString::fromStdString(WORKSPACE_INDICES)))
        .Times(1);
    EXPECT_CALL(*m_model, setIndices(WORKSPACE_INDICES)).Times(1);
    EXPECT_CALL(*m_view, setIndicesErrorLabelVisible(false)).Times(1);

    m_view->emitSelectedIndicesChanged(WORKSPACE_INDICES);
  }

  void
  test_that_the_indicesErrorLabel_is_set_to_visible_when_the_indices_are_invalid() {
    ON_CALL(*m_model, formatIndices(WORKSPACE_INDICES))
        .WillByDefault(Return(WORKSPACE_INDICES));
    ON_CALL(*m_model, setIndices(WORKSPACE_INDICES))
        .WillByDefault(Return(false));

    EXPECT_CALL(*m_model, formatIndices(WORKSPACE_INDICES)).Times(1);
    EXPECT_CALL(*m_model, setIndices(WORKSPACE_INDICES)).Times(1);
    EXPECT_CALL(*m_view, setIndicesErrorLabelVisible(true)).Times(1);

    m_view->emitSelectedIndicesChanged(WORKSPACE_INDICES);
  }

  void
  test_that_a_new_indice_suggestion_is_set_when_the_formatted_indices_are_not_empty() {
    ON_CALL(*m_model, formatIndices(WORKSPACE_INDICES))
        .WillByDefault(Return(WORKSPACE_INDICES));

    EXPECT_CALL(*m_model, formatIndices(WORKSPACE_INDICES)).Times(1);
    EXPECT_CALL(*m_view,
                addIndicesSuggestion(QString::fromStdString(WORKSPACE_INDICES)))
        .Times(1);

    m_view->emitSelectedIndicesChanged(WORKSPACE_INDICES);
  }

  void
  test_that_a_new_indice_suggestion_is_not_set_when_the_formatted_indices_are_empty() {
    ON_CALL(*m_model, formatIndices(WORKSPACE_INDICES))
        .WillByDefault(Return(""));

    EXPECT_CALL(*m_model, formatIndices("")).Times(1);
    EXPECT_CALL(*m_view, addIndicesSuggestion(QString::fromStdString("")))
        .Times(0);

    m_view->emitSelectedIndicesChanged("");
  }

  void
  test_that_the_plotSpectraClicked_signal_will_attempt_to_plot_the_spectra() {
    setExpectationsForWidgetEnabling(false);
    EXPECT_CALL(*m_model, plotSpectra()).Times(1);
    setExpectationsForWidgetEnabling(true);

    m_view->emitPlotSpectraClicked();
  }

  void
  test_that_the_plotBinsClicked_signal_will_attempt_to_plot_the_bins_when_the_bin_indices_are_valid() {
    ON_CALL(*m_model, validateIndices(_, MantidAxis::Bin))
        .WillByDefault(Return(true));

    setExpectationsForWidgetEnabling(false);
    EXPECT_CALL(*m_model, plotBins(_)).Times(1);
    setExpectationsForWidgetEnabling(true);

    m_view->emitPlotBinsClicked();
  }

  void
  test_that_the_plotBinsClicked_signal_will_display_a_warning_message_if_the_bin_indices_are_invalid() {
    ON_CALL(*m_model, validateIndices(_, MantidAxis::Bin))
        .WillByDefault(Return(false));

    EXPECT_CALL(*m_view,
                displayWarning(
                    QString("Plot Bins failed: Invalid bin indices provided.")))
        .Times(1);

    m_view->emitPlotBinsClicked();
  }

  void
  test_that_the_plotContourClicked_signal_will_attempt_to_plot_a_contour() {
    setExpectationsForWidgetEnabling(false);
    EXPECT_CALL(*m_model, plotContour()).Times(1);
    setExpectationsForWidgetEnabling(true);

    m_view->emitPlotContourClicked();
  }

  void
  test_that_the_plotTiledClicked_signal_will_attempt_to_plot_tiled_spectra() {
    setExpectationsForWidgetEnabling(false);
    EXPECT_CALL(*m_model, plotTiled()).Times(1);
    setExpectationsForWidgetEnabling(true);

    m_view->emitPlotTiledClicked();
  }

  ///----------------------------------------------------------------------
  /// Unit Tests that test the public member functions of the presenter
  ///----------------------------------------------------------------------

  void test_that_setWorkspaces_will_set_the_workspaces_in_the_view_and_model() {
    std::vector<std::string> const workspaceNames{WORKSPACE_NAME};
    ON_CALL(*m_model, getAllWorkspaceNames(workspaceNames))
        .WillByDefault(Return(workspaceNames));

    EXPECT_CALL(*m_view, setWorkspaces(workspaceNames)).Times(1);
    EXPECT_CALL(*m_model, setWorkspace(WORKSPACE_NAME)).Times(1);

    m_presenter->setWorkspaces(workspaceNames);
  }

  void
  test_that_clearWorkspaces_will_clear_the_workspaces_in_the_view_and_model() {
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

  std::unique_ptr<MockIndirectPlotOptionsView> m_view;
  MockIndirectPlotOptionsModel *m_model;
  std::unique_ptr<IndirectPlotOptionsPresenter> m_presenter;
};

#endif /* MANTIDQT_INDIRECTPLOTOPTIONSPRESENTERTEST_H_ */
