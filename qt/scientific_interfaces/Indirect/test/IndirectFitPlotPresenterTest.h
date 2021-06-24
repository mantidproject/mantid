// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "IIndirectFitPlotView.h"
#include "IndirectFitPlotPresenter.h"
#include "IndirectFitPlotView.h"
#include "IndirectFittingModel.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidTestHelpers/IndirectFitDataCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces;
using namespace MantidQt::CustomInterfaces::IDA;
using namespace testing;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

namespace {
MultiDomainFunction_sptr getFunction(std::string const &functionString) {
  return FunctionFactory::Instance().createInitializedMultiDomainFunction(functionString, 10);
}

MultiDomainFunction_sptr getFunctionWithWorkspaceName(std::string const &workspaceName) {
  std::string const functionString = "name=LinearBackground,A0=0,A1=0,ties=(A0=0.000000,A1=0.0);"
                                     "(composite=Convolution,FixResolution=true,NumDeriv=true;"
                                     "name=Resolution,Workspace=" +
                                     workspaceName +
                                     ",WorkspaceIndex=0;((composite=ProductFunction,NumDeriv="
                                     "false;name=Lorentzian,Amplitude=1,PeakCentre=0,FWHM=0."
                                     "0175)))";
  return getFunction(functionString);
}
} // namespace

/// Mock object to mock the view
class MockIndirectFitPlotView : public IIndirectFitPlotView {
public:
  /// Signals
  void emitSelectedFitDataChanged(WorkspaceID workspaceID) { emit selectedFitDataChanged(workspaceID); }

  void emitPlotCurrentPreview() { emit plotCurrentPreview(); }

  void emitPlotSpectrumChanged(WorkspaceIndex spectrum) { emit plotSpectrumChanged(spectrum); }

  void emitPlotGuessChanged(bool doPlotGuess) { emit plotGuessChanged(doPlotGuess); }

  void emitStartXChanged(double startX) { emit startXChanged(startX); }

  void emitEndXChanged(double endX) { emit endXChanged(endX); }

  void emitHWHMMinimumChanged(double minimum) { emit hwhmMinimumChanged(minimum); }

  void emitHWHMMaximumChanged(double maximum) { emit hwhmMaximumChanged(maximum); }

  void emitBackgroundChanged(double value) { emit backgroundChanged(value); }

  /// Public methods
  MOCK_METHOD1(watchADS, void(bool watch));
  MOCK_METHOD0(disableSpectrumPlotSelection, void());

  MOCK_CONST_METHOD0(getSelectedSpectrum, WorkspaceIndex());
  MOCK_CONST_METHOD0(getSelectedSpectrumIndex, FitDomainIndex());
  MOCK_CONST_METHOD0(getSelectedDataIndex, WorkspaceID());
  MOCK_CONST_METHOD0(dataSelectionSize, WorkspaceID());
  MOCK_CONST_METHOD0(isPlotGuessChecked, bool());

  MOCK_METHOD0(hideMultipleDataSelection, void());
  MOCK_METHOD0(showMultipleDataSelection, void());

  MOCK_METHOD2(setAvailableSpectra, void(WorkspaceIndex minimum, WorkspaceIndex maximum));
  MOCK_METHOD2(setAvailableSpectra, void(std::vector<WorkspaceIndex>::const_iterator const &from,
                                         std::vector<WorkspaceIndex>::const_iterator const &to));

  MOCK_METHOD1(setMinimumSpectrum, void(int minimum));
  MOCK_METHOD1(setMaximumSpectrum, void(int maximum));
  MOCK_METHOD1(setPlotSpectrum, void(WorkspaceIndex spectrum));
  MOCK_METHOD1(appendToDataSelection, void(std::string const &dataName));
  MOCK_METHOD2(setNameInDataSelection, void(std::string const &dataName, WorkspaceID workspaceID));
  MOCK_METHOD0(clearDataSelection, void());

  MOCK_METHOD4(plotInTopPreview, void(QString const &name, Mantid::API::MatrixWorkspace_sptr workspace,
                                      WorkspaceIndex spectrum, Qt::GlobalColor colour));
  MOCK_METHOD4(plotInBottomPreview, void(QString const &name, Mantid::API::MatrixWorkspace_sptr workspace,
                                         WorkspaceIndex spectrum, Qt::GlobalColor colour));

  MOCK_METHOD1(removeFromTopPreview, void(QString const &name));
  MOCK_METHOD1(removeFromBottomPreview, void(QString const &name));

  MOCK_METHOD1(enableFitSingleSpectrum, void(bool enable));
  MOCK_METHOD1(enablePlotGuess, void(bool enable));
  MOCK_METHOD1(enableSpectrumSelection, void(bool enable));
  MOCK_METHOD1(enableFitRangeSelection, void(bool enable));

  MOCK_METHOD1(setFitSingleSpectrumText, void(QString const &text));
  MOCK_METHOD1(setFitSingleSpectrumEnabled, void(bool enable));

  MOCK_METHOD1(setBackgroundLevel, void(double value));

  MOCK_METHOD2(setFitRange, void(double minimum, double maximum));
  MOCK_METHOD1(setFitRangeMinimum, void(double minimum));
  MOCK_METHOD1(setFitRangeMaximum, void(double maximum));
  MOCK_METHOD1(setFitRangeBounds, void(std::pair<double, double> const &bounds));

  MOCK_METHOD1(setBackgroundRangeVisible, void(bool visible));
  MOCK_METHOD1(setHWHMRangeVisible, void(bool visible));

  MOCK_METHOD1(allowRedraws, void(bool state));
  MOCK_METHOD0(redrawPlots, void());

  MOCK_CONST_METHOD1(displayMessage, void(std::string const &message));

  /// Public Slots
  MOCK_METHOD0(clearTopPreview, void());
  MOCK_METHOD0(clearBottomPreview, void());
  MOCK_METHOD0(clearPreviews, void());

  MOCK_METHOD2(setHWHMRange, void(double minimum, double maximum));
  MOCK_METHOD1(setHWHMMinimum, void(double minimum));
  MOCK_METHOD1(setHWHMMaximum, void(double maximum));
};

class MockIndirectFittingModel : public IndirectFittingModel {
public:
  /// Public methods
  MOCK_CONST_METHOD1(getWorkspace, MatrixWorkspace_sptr(WorkspaceID workspaceID));
  MOCK_CONST_METHOD2(getFittingRange, std::pair<double, double>(WorkspaceID workspaceID, WorkspaceIndex spectrum));
  MOCK_CONST_METHOD1(createDisplayName, std::string(WorkspaceID workspaceID));
  MOCK_CONST_METHOD0(isMultiFit, bool());
  MOCK_CONST_METHOD0(getNumberOfWorkspaces, WorkspaceID());
  MOCK_CONST_METHOD0(getFitFunction, Mantid::API::MultiDomainFunction_sptr());

  MOCK_METHOD3(setStartX, void(double startX, WorkspaceID workspaceID, WorkspaceIndex spectrum));
  MOCK_METHOD3(setEndX, void(double endX, WorkspaceID workspaceID, WorkspaceIndex spectrum));

  MOCK_METHOD2(setEndX, void(double endX, WorkspaceID workspaceID));
  MOCK_METHOD2(setStartX, void(double endX, WorkspaceID workspaceID));

  MOCK_METHOD3(setDefaultParameterValue, void(std::string const &name, double value, WorkspaceID workspaceID));

private:
  std::string sequentialFitOutputName() const override { return ""; };
  std::string simultaneousFitOutputName() const override { return ""; };
  std::string singleFitOutputName(WorkspaceID workspaceID, WorkspaceIndex spectrum) const override {
    UNUSED_ARG(workspaceID);
    UNUSED_ARG(spectrum);
    return "";
  };
};

GNU_DIAG_ON_SUGGEST_OVERRIDE

class IndirectFitPlotPresenterTest : public CxxTest::TestSuite {
public:
  /// Needed to make sure everything is initialized
  IndirectFitPlotPresenterTest() { FrameworkManager::Instance(); }

  static IndirectFitPlotPresenterTest *createSuite() { return new IndirectFitPlotPresenterTest(); }

  static void destroySuite(IndirectFitPlotPresenterTest *suite) { delete suite; }

  void setUp() override {
    /// Note that the IndirectFitPlotModel could not be mocked as the
    /// Presenter takes an IndirectFittingModel. This means the
    /// IndirectFittingModel is mocked instead - which is a good
    /// substitute anyway
    m_view = std::make_unique<NiceMock<MockIndirectFitPlotView>>();
    m_fittingModel = std::make_unique<NiceMock<MockIndirectFittingModel>>();
    m_presenter = std::make_unique<IndirectFitPlotPresenter>(std::move(m_fittingModel.get()), std::move(m_view.get()));

    SetUpADSWithWorkspace m_ads("WorkspaceName", createWorkspace(10));
    m_fittingModel->addWorkspace("WorkspaceName");
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();

    TS_ASSERT(Mock::VerifyAndClearExpectations(m_view.get()));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_fittingModel.get()));

    m_presenter.reset();
    m_fittingModel.reset();
    m_view.reset();
  }

  ///----------------------------------------------------------------------
  /// Unit tests to check for successful presenter instantiation
  ///----------------------------------------------------------------------

  void test_that_the_model_and_view_have_been_instantiated_correctly() {
    WorkspaceIndex const selectedSpectrum(3);

    ON_CALL(*m_view, getSelectedSpectrum()).WillByDefault(Return(selectedSpectrum));
    ON_CALL(*m_fittingModel, isMultiFit()).WillByDefault(Return(false));

    EXPECT_CALL(*m_view, getSelectedSpectrum()).Times(1).WillOnce(Return(selectedSpectrum));
    EXPECT_CALL(*m_fittingModel, isMultiFit()).Times(1).WillOnce(Return(false));

    m_view->getSelectedSpectrum();
    m_fittingModel->isMultiFit();
  }

  void test_that_invoking_a_presenter_method_will_call_the_relevant_methods_in_the_model_and_view() {
    WorkspaceID const selectionSize(2);

    ON_CALL(*m_view, dataSelectionSize()).WillByDefault(Return(selectionSize));

    EXPECT_CALL(*m_fittingModel, getNumberOfWorkspaces()).Times(1).WillRepeatedly(Return(2));
    EXPECT_CALL(*m_view, dataSelectionSize()).Times(1).WillOnce(Return(selectionSize));

    m_presenter->appendLastDataToSelection();
  }

  ///----------------------------------------------------------------------
  /// Unit Tests that test the signals (only the view emits signals here)
  ///----------------------------------------------------------------------

  void test_that_the_selectedFitDataChanged_signal_will_set_the_activeIndex() {
    m_view->emitSelectedFitDataChanged(1);
    TS_ASSERT_EQUALS(m_presenter->getSelectedDataIndex(), 1);
  }

  void test_that_the_selectedFitDataChanged_signal_will_set_the_available_spectra() {
    WorkspaceID const index(0);
    ON_CALL(*m_fittingModel, getWorkspace(index)).WillByDefault(Return(m_ads->retrieveWorkspace("WorkspaceName")));

    EXPECT_CALL(*m_view, setAvailableSpectra(WorkspaceIndex(0), WorkspaceIndex(9))).Times(1);

    m_view->emitSelectedFitDataChanged(index);
  }

  void test_that_selectedFitDataChanged_signal_will_enable_selectors_when_workspace_presenter() {
    WorkspaceID const index(0);
    ON_CALL(*m_fittingModel, getWorkspace(index)).WillByDefault(Return(m_ads->retrieveWorkspace("WorkspaceName")));

    EXPECT_CALL(*m_view, enableSpectrumSelection(true)).Times(1);
    EXPECT_CALL(*m_view, enableFitRangeSelection(true)).Times(1);

    m_view->emitSelectedFitDataChanged(index);
  }

  void test_that_the_selectedFitDataChanged_signal_will_disable_selectors_when_there_is_no_workspace() {
    WorkspaceID const index(0);
    ON_CALL(*m_fittingModel, getWorkspace(index)).WillByDefault(Return(nullptr));

    EXPECT_CALL(*m_view, enableSpectrumSelection(false)).Times(1);
    EXPECT_CALL(*m_view, enableFitRangeSelection(false)).Times(1);

    m_view->emitSelectedFitDataChanged(index);
  }

  void test_that_the_selectedFitDataChanged_signal_will_plot_the_input_when_there_is_only_an_input_workspace() {
    WorkspaceID const index(0);
    ON_CALL(*m_fittingModel, getWorkspace(index)).WillByDefault(Return(m_ads->retrieveWorkspace("WorkspaceName")));
    EXPECT_CALL(*m_fittingModel, getWorkspace(index)).Times(3);

    m_view->emitSelectedFitDataChanged(index);
  }

  void test_that_the_selectedFitDataChanged_signal_will_clear_the_plots_when_there_is_no_input_workspace() {
    WorkspaceID const index(0);
    ON_CALL(*m_fittingModel, getWorkspace(index)).WillByDefault(Return(nullptr));

    EXPECT_CALL(*m_fittingModel, getWorkspace(index)).Times(2);
    EXPECT_CALL(*m_view, clearPreviews()).Times(1);

    m_view->emitSelectedFitDataChanged(index);
  }

  void test_that_the_selectedFitDataChanged_signal_will_set_the_minimum_and_maximum_of_the_fit_range() {
    WorkspaceID const index(0);
    auto const range = std::make_pair(1.0, 2.0);
    ON_CALL(*m_fittingModel, getFittingRange(index, WorkspaceIndex(0))).WillByDefault(Return(range));

    EXPECT_CALL(*m_fittingModel, getFittingRange(index, WorkspaceIndex(0))).Times(1).WillRepeatedly(Return(range));
    EXPECT_CALL(*m_view, setFitRangeMinimum(1.0)).Times(1);
    EXPECT_CALL(*m_view, setFitRangeMaximum(2.0)).Times(1);

    m_view->emitSelectedFitDataChanged(index);
  }

  void test_that_the_plotSpectrumChanged_signal_will_set_the_active_spectrum() {
    m_view->emitPlotSpectrumChanged(2);
    TS_ASSERT_EQUALS(m_presenter->getSelectedSpectrum(), 2);
  }

  void test_that_the_plotSpectrumChanged_signal_will_plot_the_input_when_there_is_only_an_input_workspace() {
    WorkspaceID const index(0);
    ON_CALL(*m_fittingModel, getWorkspace(index)).WillByDefault(Return(m_ads->retrieveWorkspace("WorkspaceName")));

    EXPECT_CALL(*m_fittingModel, getWorkspace(index)).Times(2);
    EXPECT_CALL(*m_view, clearPreviews()).Times(1);

    m_view->emitPlotSpectrumChanged(0);
  }

  void test_that_the_plotSpectrumChanged_signal_will_clear_the_plots_when_there_is_no_input_workspace() {
    WorkspaceID const index(0);
    ON_CALL(*m_fittingModel, getWorkspace(index)).WillByDefault(Return(nullptr));

    EXPECT_CALL(*m_fittingModel, getWorkspace(index)).Times(1);
    EXPECT_CALL(*m_view, clearPreviews()).Times(1);

    m_view->emitPlotSpectrumChanged(0);
  }

  void test_that_the_plotSpectrumChanged_signal_will_set_the_minimum_and_maximum_of_the_fit_range() {
    WorkspaceID const index(0);
    auto const range = std::make_pair(1.0, 2.0);
    ON_CALL(*m_fittingModel, getFittingRange(index, WorkspaceIndex(0))).WillByDefault(Return(range));

    EXPECT_CALL(*m_fittingModel, getFittingRange(index, WorkspaceIndex(0))).Times(1).WillOnce(Return(range));
    EXPECT_CALL(*m_view, setFitRangeMinimum(1.0)).Times(1);
    EXPECT_CALL(*m_view, setFitRangeMaximum(2.0)).Times(1);

    m_view->emitPlotSpectrumChanged(0);
  }

  void test_that_the_plotCurrentPreview_signal_will_display_an_error_message_if_there_is_no_input_workspace() {
    WorkspaceID const index(0);
    std::string const message("Workspace not found - data may not be loaded.");

    ON_CALL(*m_fittingModel, getWorkspace(index)).WillByDefault(Return(nullptr));

    Expectation getWorkspace = EXPECT_CALL(*m_fittingModel, getWorkspace(index)).Times(1);
    EXPECT_CALL(*m_view, displayMessage(message)).Times(1).After(getWorkspace);

    m_view->emitPlotCurrentPreview();
  }

  void test_that_the_plotGuessChanged_signal_will_not_clear_the_guess_plot_when_passed_true() {
    WorkspaceID const index(0);
    std::string const workspaceName("WorkspaceName");
    auto const range = std::make_pair(1.0, 2.0);
    auto const fitFunction = getFunctionWithWorkspaceName(workspaceName);

    ON_CALL(*m_fittingModel, getFittingRange(index, WorkspaceIndex(0))).WillByDefault(Return(range));
    ON_CALL(*m_fittingModel, getFitFunction()).WillByDefault(Return(fitFunction));
    ON_CALL(*m_fittingModel, getWorkspace(index)).WillByDefault(Return(m_ads->retrieveWorkspace(workspaceName)));

    EXPECT_CALL(*m_view, removeFromTopPreview(QString::fromStdString("Guess"))).Times(0);

    m_view->emitPlotGuessChanged(true);
  }

  void test_that_the_plotGuessChanged_signal_will_clear_the_plot_when_passed_false() {
    WorkspaceID const index(0);
    ON_CALL(*m_fittingModel, getWorkspace(index)).WillByDefault(Return(m_ads->retrieveWorkspace("WorkspaceName")));

    EXPECT_CALL(*m_view, removeFromTopPreview(QString::fromStdString("Guess"))).Times(1);

    m_view->emitPlotGuessChanged(false);
  }

  void test_that_the_startXChanged_signal_will_set_the_fitting_models_startX() {
    auto const range = std::make_pair(0.0, 2.0);
    ON_CALL(*m_fittingModel, getFittingRange(WorkspaceID(0), WorkspaceIndex(0))).WillByDefault(Return(range));

    EXPECT_CALL(*m_fittingModel, setStartX(1.0, WorkspaceID(0))).Times(1);

    m_view->emitStartXChanged(1.0);
  }

  void test_that_the_endXChanged_signal_will_set_the_fitting_models_endX() {
    EXPECT_CALL(*m_fittingModel, setEndX(2.0, WorkspaceID(0))).Times(1);
    m_view->emitEndXChanged(2.0);
  }

  void test_that_the_hwhmMaximumChanged_signal_will_set_the_hwhm_minimum() {
    EXPECT_CALL(*m_view, setHWHMMinimum(-2.0)).Times(1);
    m_view->emitHWHMMaximumChanged(2.0);
  }

  void test_that_the_hwhmMinimumChanged_signal_will_set_the_hwhm_maximum() {
    EXPECT_CALL(*m_view, setHWHMMaximum(-2.0)).Times(1);
    m_view->emitHWHMMinimumChanged(2.0);
  }

  void test_that_the_backgroundChanged_signal_will_set_the_functions_background() {
    double const background(1.2);
    auto const fitFunction = getFunctionWithWorkspaceName("WorkspaceName");

    ON_CALL(*m_fittingModel, getFitFunction()).WillByDefault(Return(fitFunction));

    Expectation setDefault =
        EXPECT_CALL(*m_fittingModel, setDefaultParameterValue("A0", background, WorkspaceID(0))).Times(1);
    EXPECT_CALL(*m_fittingModel, getFitFunction()).Times(1).After(setDefault);

    m_view->emitBackgroundChanged(background);
  }

  ///----------------------------------------------------------------------
  /// Unit Tests that test the methods and slots
  ///----------------------------------------------------------------------

  void test_that_getSelectedSpectrumIndex_will_get_the_selected_spectrum_from_the_view() {
    EXPECT_CALL(*m_view, getSelectedSpectrumIndex()).Times(1).WillOnce(Return(FitDomainIndex(0)));
    m_presenter->getSelectedSpectrumIndex();
  }

  void test_that_setActiveSpectrum_will_set_the_spectrum_in_view_and_model() {
    EXPECT_CALL(*m_view, setPlotSpectrum(WorkspaceIndex{3})).Times(1);

    m_presenter->setActiveSpectrum(WorkspaceIndex{3});
    ON_CALL(*m_fittingModel, getNumberOfWorkspaces()).WillByDefault(Return(1));

    TS_ASSERT_EQUALS(m_presenter->getSelectedDomainIndex(), FitDomainIndex{3});
  }

  void test_that_isCurrentlySelected_returns_true_if_the_index_and_spectrum_given_are_selected() {
    m_view->emitSelectedFitDataChanged(2);
    TS_ASSERT(m_presenter->isCurrentlySelected(2, 0));
  }

  void test_that_isCurrentlySelected_returns_false_if_the_index_and_spectrum_given_are_not_selected() {
    m_view->emitSelectedFitDataChanged(2);
    TS_ASSERT(!m_presenter->isCurrentlySelected(0, 0));
  }

  void test_that_setFitSingleSpectrum_methods_calls_view() {
    EXPECT_CALL(*m_view, setFitSingleSpectrumText(QString("Fitting..."))).Times(1);
    EXPECT_CALL(*m_view, setFitSingleSpectrumEnabled(true)).Times(1);
    m_presenter->setFitSingleSpectrumIsFitting(true);
    m_presenter->setFitSingleSpectrumEnabled(true);
  }

  void test_that_setStartX_will_set_the_fit_range_minimum_in_the_view() {
    EXPECT_CALL(*m_view, setFitRangeMinimum(2.0)).Times(1);
    m_presenter->setStartX(2.0);
  }

  void test_that_setEndX_will_set_the_fit_range_maximum_in_the_view() {
    EXPECT_CALL(*m_view, setFitRangeMaximum(3.0)).Times(1);
    m_presenter->setEndX(3.0);
  }

  void test_updatePlotSpectrum_calls_correct_slots() {
    EXPECT_CALL(*m_view, setPlotSpectrum(WorkspaceIndex{3})).Times(1);
    EXPECT_CALL(*m_view, clearPreviews()).Times(1);
    m_presenter->updatePlotSpectrum(WorkspaceIndex{3});

    ON_CALL(*m_fittingModel, getNumberOfWorkspaces()).WillByDefault(Return(1));
    TS_ASSERT_EQUALS(m_presenter->getSelectedDomainIndex(), FitDomainIndex{3});
  }

  void test_that_hideMultipleDataSelection_will_call_hideMultipleDataSelection_in_the_view() {
    EXPECT_CALL(*m_view, hideMultipleDataSelection()).Times(1);
    m_presenter->hideMultipleDataSelection();
  }

  void test_that_showMultipleDataSelection_will_call_showMultipleDataSelection_in_the_view() {
    EXPECT_CALL(*m_view, showMultipleDataSelection()).Times(1);
    m_presenter->showMultipleDataSelection();
  }

  void test_that_updateRangeSelectors_will_update_the_background_selector() {
    auto const fitFunction = getFunctionWithWorkspaceName("WorkspaceName");

    ON_CALL(*m_fittingModel, getFitFunction()).WillByDefault(Return(fitFunction));

    Expectation setVisible = EXPECT_CALL(*m_view, setBackgroundRangeVisible(true)).Times(1);
    EXPECT_CALL(*m_view, setBackgroundLevel(0.0)).Times(1).After(setVisible);

    m_presenter->updateRangeSelectors();
  }

  void test_that_updateRangeSelectors_will_update_the_hwhm_selector() {
    auto const fitFunction = getFunctionWithWorkspaceName("WorkspaceName");

    ON_CALL(*m_fittingModel, getFitFunction()).WillByDefault(Return(fitFunction));

    Expectation setVisible = EXPECT_CALL(*m_view, setHWHMRangeVisible(true)).Times(1);
    EXPECT_CALL(*m_view, setHWHMMinimum(-0.00875)).Times(1).After(setVisible);
    EXPECT_CALL(*m_view, setHWHMMaximum(0.00875)).Times(1).After(setVisible);

    m_presenter->updateRangeSelectors();
  }

  void
  test_that_appendLastDataToSelection_will_set_the_name_of_the_data_selection_if_the_dataSelectionSize_and_numberOfWorkspaces_are_equal() {
    WorkspaceID const index1(0);
    WorkspaceID const index2(1);

    ON_CALL(*m_view, dataSelectionSize()).WillByDefault(Return(WorkspaceID(2)));
    ON_CALL(*m_fittingModel, getNumberOfWorkspaces()).WillByDefault(Return(2));
    ON_CALL(*m_fittingModel, createDisplayName(WorkspaceID(0))).WillByDefault(Return("DisplayName-0"));
    ON_CALL(*m_fittingModel, createDisplayName(WorkspaceID(1))).WillByDefault(Return("DisplayName-1"));
    ON_CALL(*m_fittingModel, getWorkspace(index1)).WillByDefault(Return(m_ads->retrieveWorkspace("WorkspaceName")));
    ON_CALL(*m_fittingModel, getWorkspace(index2)).WillByDefault(Return(m_ads->retrieveWorkspace("WorkspaceName")));

    Expectation createName1 = EXPECT_CALL(*m_fittingModel, createDisplayName(index1)).Times(1);
    EXPECT_CALL(*m_view, setNameInDataSelection("DisplayName-0", index1)).Times(1).After(createName1);
    Expectation createName2 = EXPECT_CALL(*m_fittingModel, createDisplayName(index2)).Times(1);
    EXPECT_CALL(*m_view, setNameInDataSelection("DisplayName-1", index2)).Times(1).After(createName2);

    m_presenter->appendLastDataToSelection();
  }

  void
  test_that_appendLastDataToSelection_will_add_to_the_data_selection_if_the_dataSelectionSize_and_numberOfWorkspaces_are_not_equal() {
    WorkspaceID const index(1);

    ON_CALL(*m_view, dataSelectionSize()).WillByDefault(Return(WorkspaceID(1)));
    ON_CALL(*m_fittingModel, getNumberOfWorkspaces()).WillByDefault(Return(2));
    ON_CALL(*m_fittingModel, createDisplayName(index)).WillByDefault(Return("DisplayName-1"));
    ON_CALL(*m_fittingModel, getWorkspace(index)).WillByDefault(Return(m_ads->retrieveWorkspace("WorkspaceName")));

    Expectation createName = EXPECT_CALL(*m_fittingModel, createDisplayName(index)).Times(1);
    EXPECT_CALL(*m_view, appendToDataSelection("DisplayName-1")).Times(1).After(createName);

    m_presenter->appendLastDataToSelection();
  }

  void test_that_updateSelectedDataName_will_update_the_name_in_the_data_selection() {
    WorkspaceID const index(0);

    ON_CALL(*m_fittingModel, createDisplayName(index)).WillByDefault(Return("DisplayName-1"));
    ON_CALL(*m_fittingModel, getWorkspace(index)).WillByDefault(Return(m_ads->retrieveWorkspace("WorkspaceName")));

    Expectation createName = EXPECT_CALL(*m_fittingModel, createDisplayName(index)).Times(1);
    EXPECT_CALL(*m_view, setNameInDataSelection("DisplayName-1", WorkspaceID(0))).Times(1).After(createName);

    m_presenter->updateSelectedDataName();
  }

  void test_updateDataSelection_appends_for_each_workspace() {
    WorkspaceID const index1(0);
    WorkspaceID const index2(1);

    ON_CALL(*m_view, dataSelectionSize()).WillByDefault(Return(WorkspaceID(2)));
    ON_CALL(*m_fittingModel, getNumberOfWorkspaces()).WillByDefault(Return(2));
    ON_CALL(*m_fittingModel, createDisplayName(WorkspaceID(0))).WillByDefault(Return("DisplayName-0"));
    ON_CALL(*m_fittingModel, createDisplayName(WorkspaceID(1))).WillByDefault(Return("DisplayName-1"));
    ON_CALL(*m_fittingModel, getWorkspace(index1)).WillByDefault(Return(m_ads->retrieveWorkspace("WorkspaceName")));
    ON_CALL(*m_fittingModel, getWorkspace(index2)).WillByDefault(Return(m_ads->retrieveWorkspace("WorkspaceName")));

    EXPECT_CALL(*m_view, clearDataSelection()).Times(1);
    EXPECT_CALL(*m_view, appendToDataSelection("DisplayName-0")).Times(1);
    EXPECT_CALL(*m_view, appendToDataSelection("DisplayName-1")).Times(1);

    m_presenter->updateDataSelection();
  }

  void test_updateDataSelection_sets_active_spectra_to_zero() {
    WorkspaceID const index1(0);
    WorkspaceID const index2(1);

    ON_CALL(*m_view, dataSelectionSize()).WillByDefault(Return(WorkspaceID(2)));
    ON_CALL(*m_fittingModel, getNumberOfWorkspaces()).WillByDefault(Return(2));
    ON_CALL(*m_fittingModel, createDisplayName(WorkspaceID(0))).WillByDefault(Return("DisplayName-0"));
    ON_CALL(*m_fittingModel, createDisplayName(WorkspaceID(1))).WillByDefault(Return("DisplayName-1"));
    ON_CALL(*m_fittingModel, getWorkspace(index1)).WillByDefault(Return(m_ads->retrieveWorkspace("WorkspaceName")));
    ON_CALL(*m_fittingModel, getWorkspace(index2)).WillByDefault(Return(m_ads->retrieveWorkspace("WorkspaceName")));

    EXPECT_CALL(*m_view, clearDataSelection()).Times(1);
    EXPECT_CALL(*m_view, appendToDataSelection("DisplayName-0")).Times(1);
    EXPECT_CALL(*m_view, appendToDataSelection("DisplayName-1")).Times(1);
    EXPECT_CALL(*m_view, setPlotSpectrum(WorkspaceIndex{0})).Times(2);
    TS_ASSERT_EQUALS(m_presenter->getSelectedSpectrum(), WorkspaceIndex{0});

    m_presenter->updateDataSelection();
  }

  void test_updateAvailableSpectra_uses_minmax_if_spectra_is_continuous() {
    auto spectra = FunctionModelSpectra("0-9");
    auto minmax = spectra.getMinMax();
    EXPECT_CALL(*m_view, setAvailableSpectra(minmax.first, minmax.second)).Times(1);

    WorkspaceID const index1(0);
    ON_CALL(*m_fittingModel, getWorkspace(index1)).WillByDefault(Return(m_ads->retrieveWorkspace("WorkspaceName")));

    m_presenter->updateAvailableSpectra();
  }

  void test_disables_data_selection_if_no_workspace() {
    auto spectra = FunctionModelSpectra("0-9");
    EXPECT_CALL(*m_view, enableSpectrumSelection(false)).Times(1);
    EXPECT_CALL(*m_view, enableFitRangeSelection(false)).Times(1);

    WorkspaceID const index1(0);
    ON_CALL(*m_fittingModel, getWorkspace(index1)).WillByDefault(Return(nullptr));

    m_presenter->updateAvailableSpectra();
  }

  void test_updateFit_holds_redrawing_and_updates_guess() {
    EXPECT_CALL(*m_view, allowRedraws(false)).Times(1);
    EXPECT_CALL(*m_view, allowRedraws(true)).Times(1);
    EXPECT_CALL(*m_view, redrawPlots()).Times(2);
    EXPECT_CALL(*m_view, enablePlotGuess(false)).Times(1);

    m_presenter->updateFit();
  }

  void test_that_setXBounds_calls_the_correct_method_in_the_view() {
    auto const bounds = std::make_pair(0.0, 1.0);

    EXPECT_CALL(*m_view, setFitRangeBounds(bounds)).Times(1);

    m_presenter->setXBounds(bounds);
  }

private:
  std::unique_ptr<MockIndirectFitPlotView> m_view;
  std::unique_ptr<MockIndirectFittingModel> m_fittingModel;
  std::unique_ptr<IndirectFitPlotPresenter> m_presenter;

  SetUpADSWithWorkspace *m_ads;
};
