// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "QENSFitting/FitOutput.h"
#include "QENSFitting/FitPlotPresenter.h"
#include "QENSFitting/FitPlotView.h"
#include "QENSFitting/FittingModel.h"
#include "QENSFitting/IFitPlotView.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidFrameworkTestHelpers/IndirectFitDataCreationHelper.h"
#include "MockObjects.h"

using namespace Mantid::API;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces;
using namespace MantidQt::CustomInterfaces::Inelastic;
using namespace testing;

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

class FitPlotPresenterTest : public CxxTest::TestSuite {
public:
  static FitPlotPresenterTest *createSuite() { return new FitPlotPresenterTest(); }

  static void destroySuite(FitPlotPresenterTest *suite) { delete suite; }

  void setUp() override {
    m_tab = std::make_unique<NiceMock<MockFitTab>>();
    m_model = std::make_unique<NiceMock<MockFitPlotModel>>();
    m_view = std::make_unique<NiceMock<MockFitPlotView>>();
    m_presenter = std::make_unique<FitPlotPresenter>(m_tab.get(), m_view.get(), m_model.get());

    m_workspace = createWorkspaceWithInstrument(6, 5);
    m_ads = std::make_unique<SetUpADSWithWorkspace>("WorkspaceName", m_workspace);
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_view.get()));

    m_presenter.reset();
    m_view.reset();
  }

  ///----------------------------------------------------------------------
  /// Unit tests to check for successful presenter instantiation
  ///----------------------------------------------------------------------

  void test_that_the_model_and_view_have_been_instantiated_correctly() {
    WorkspaceIndex const selectedSpectrum(3);

    ON_CALL(*m_view, getSelectedSpectrum()).WillByDefault(Return(selectedSpectrum));

    EXPECT_CALL(*m_view, getSelectedSpectrum()).Times(1).WillOnce(Return(selectedSpectrum));

    m_view->getSelectedSpectrum();
  }

  void test_that_invoking_a_presenter_method_will_call_the_relevant_methods_in_the_model_and_view() {
    WorkspaceID const selectionSize(2);

    ON_CALL(*m_view, dataSelectionSize()).WillByDefault(Return(selectionSize));

    EXPECT_CALL(*m_view, dataSelectionSize()).Times(1).WillOnce(Return(selectionSize));
    m_presenter->appendLastDataToSelection({"WorkspaceName", "WorkspaceName"});
  }

  ///----------------------------------------------------------------------
  /// Unit Tests that test the signals (only the view emits signals here)
  ///----------------------------------------------------------------------

  void test_that_handleSelectedFitDataChanged_will_set_the_activeIndex() {
    m_presenter->handleSelectedFitDataChanged(WorkspaceID(1));
    TS_ASSERT_EQUALS(m_presenter->getActiveWorkspaceIndex(), 0);
  }

  void test_that_handleSelectedFitDataChanged_will_set_the_available_spectra() {
    ON_CALL(*m_model, getWorkspace()).WillByDefault(Return(m_workspace));
    ON_CALL(*m_model, getActiveWorkspaceID()).WillByDefault(Return(WorkspaceID{0}));
    ON_CALL(*m_model, getSpectra(WorkspaceID{0})).WillByDefault(Return(FunctionModelSpectra("0-5")));

    EXPECT_CALL(*m_view, setAvailableSpectra(WorkspaceIndex(0), WorkspaceIndex(5))).Times(1);

    m_presenter->handleSelectedFitDataChanged(WorkspaceID(0));
  }

  void test_that_handleSelectedFitDataChanged_will_enable_selectors_when_workspace_presenter() {
    ON_CALL(*m_model, getWorkspace()).WillByDefault(Return(m_workspace));
    ON_CALL(*m_model, getActiveWorkspaceID()).WillByDefault(Return(WorkspaceID{0}));
    ON_CALL(*m_model, getSpectra(WorkspaceID{0})).WillByDefault(Return(FunctionModelSpectra("0-5")));

    EXPECT_CALL(*m_view, enableSpectrumSelection(true)).Times(1);
    EXPECT_CALL(*m_view, enableFitRangeSelection(true)).Times(1);

    m_presenter->handleSelectedFitDataChanged(WorkspaceID(0));
  }

  void test_that_handleSelectedFitDataChanged_will_disable_selectors_when_there_is_no_workspace() {
    ON_CALL(*m_model, getWorkspace()).WillByDefault(Return(nullptr));

    EXPECT_CALL(*m_view, enableSpectrumSelection(false)).Times(1);
    EXPECT_CALL(*m_view, enableFitRangeSelection(false)).Times(1);

    m_presenter->handleSelectedFitDataChanged(WorkspaceID(0));
  }

  void test_that_handleSelectedFitDataChanged_will_clear_the_plots_when_there_is_no_input_workspace() {
    EXPECT_CALL(*m_view, clearPreviews()).Times(1);
    m_presenter->handleSelectedFitDataChanged(WorkspaceID(0));
  }

  void test_that_handleSelectedFitDataChanged_will_set_the_minimum_and_maximum_of_the_fit_range() {
    ON_CALL(*m_model, getRange()).WillByDefault(Return(std::pair<double, double>{1.0, 2.0}));

    EXPECT_CALL(*m_view, setFitRangeMinimum(1.0)).Times(1);
    EXPECT_CALL(*m_view, setFitRangeMaximum(2.0)).Times(1);

    m_presenter->handleSelectedFitDataChanged(WorkspaceID(0));
  }

  void test_that_handlePlotSpectrumChanged_will_set_the_active_spectrum() {
    auto workspaceIndex = WorkspaceIndex(2);

    EXPECT_CALL(*m_model, setActiveSpectrum(workspaceIndex)).Times(1);
    EXPECT_CALL(*m_view, setPlotSpectrum(workspaceIndex)).Times(1);

    m_presenter->handlePlotSpectrumChanged(workspaceIndex);
  }

  void test_that_handlePlotSpectrumChanged_will_plot_the_input_when_there_is_only_an_input_workspace() {
    EXPECT_CALL(*m_view, clearPreviews()).Times(1);

    m_presenter->handlePlotSpectrumChanged(WorkspaceIndex(0));
  }

  void test_that_handlePlotSpectrumChanged_will_clear_the_plots_when_there_is_no_input_workspace() {
    EXPECT_CALL(*m_view, clearPreviews()).Times(1);

    m_presenter->handlePlotSpectrumChanged(WorkspaceIndex(0));
  }

  void test_that_handlePlotSpectrumChanged_will_set_the_minimum_and_maximum_of_the_fit_range() {
    ON_CALL(*m_model, getRange()).WillByDefault(Return(std::pair<double, double>{1.0, 2.0}));

    EXPECT_CALL(*m_view, setFitRangeMinimum(1.0)).Times(1);
    EXPECT_CALL(*m_view, setFitRangeMaximum(2.0)).Times(1);

    m_presenter->handlePlotSpectrumChanged(WorkspaceIndex(0));
  }

  void test_that_handlePlotCurrentPreview_will_display_an_error_message_if_there_is_no_input_workspace() {
    std::string const message("Workspace not found - data may not be loaded.");

    EXPECT_CALL(*m_view, displayMessage(message)).Times(1);

    m_presenter->handlePlotCurrentPreview();
  }

  void test_that_handlePlotGuess_will_not_clear_the_guess_plot_when_passed_true() {
    WorkspaceID const index(0);
    std::string const workspaceName("WorkspaceName");
    auto const fitFunction = getFunctionWithWorkspaceName(workspaceName);
    m_presenter->setFitFunction(fitFunction);

    EXPECT_CALL(*m_view, removeFromTopPreview(QString::fromStdString("Guess"))).Times(0);

    m_presenter->handlePlotGuess(true);
  }

  void test_that_handlePlotGuess_will_clear_the_plot_when_passed_false() {
    WorkspaceID const index(0);

    EXPECT_CALL(*m_view, removeFromTopPreview(QString::fromStdString("Guess"))).Times(1);

    m_presenter->handlePlotGuess(false);
  }

  void test_that_handleHWHMMinimumChanged_will_set_the_hwhm_minimum() {
    double hwhmMin{2.0};

    ON_CALL(*m_model, calculateHWHMMinimum(hwhmMin)).WillByDefault(Return(-hwhmMin));
    EXPECT_CALL(*m_view, setHWHMMinimum(-hwhmMin)).Times(1);

    m_presenter->handleHWHMMinimumChanged(hwhmMin);
  }

  void test_that_handleHWHMMaximumChanged_will_set_the_hwhm_maximum() {
    double hwhmMax{2.0};

    ON_CALL(*m_model, calculateHWHMMaximum(hwhmMax)).WillByDefault(Return(-hwhmMax));
    EXPECT_CALL(*m_view, setHWHMMaximum(-hwhmMax)).Times(1);

    m_presenter->handleHWHMMaximumChanged(2.0);
  }

  ///----------------------------------------------------------------------
  /// Unit Tests that test the methods and slots
  ///----------------------------------------------------------------------

  void test_that_setActiveSpectrum_will_set_the_spectrum_in_view_and_model() {
    auto const workspaceIndex = WorkspaceIndex{3};

    EXPECT_CALL(*m_model, setActiveSpectrum(workspaceIndex)).Times(1);
    EXPECT_CALL(*m_view, setPlotSpectrum(workspaceIndex)).Times(1);

    m_presenter->setActiveSpectrum(workspaceIndex);
  }

  void test_that_setFitSingleSpectrum_methods_calls_view() {
    EXPECT_CALL(*m_view, setFitSingleSpectrumText(QString("Fitting..."))).Times(1);
    EXPECT_CALL(*m_view, setFitSingleSpectrumEnabled(true)).Times(1);
    m_presenter->setFitSingleSpectrumIsFitting(true);
    m_presenter->setFitSingleSpectrumEnabled(true);
  }

  void test_that_setXBounds_calls_the_correct_method_in_the_view() {
    auto const bounds = std::make_pair(0.0, 1.0);

    EXPECT_CALL(*m_view, setFitRangeBounds(bounds)).Times(1);

    m_presenter->setXBounds(bounds);
  }

  void test_that_setStartX_will_set_the_fit_range_minimum_in_the_view() {
    EXPECT_CALL(*m_view, setFitRangeMinimum(2.0)).Times(1);
    m_presenter->setStartX(2.0);
  }

  void test_that_setEndX_will_set_the_fit_range_maximum_in_the_view() {
    EXPECT_CALL(*m_view, setFitRangeMaximum(3.0)).Times(1);
    m_presenter->setEndX(3.0);
  }

  void test_that_updateRangeSelectors_will_update_the_background_selector() {
    auto const fitFunction = getFunctionWithWorkspaceName("WorkspaceName");
    m_presenter->setFitFunction(fitFunction);

    ON_CALL(*m_model, getFirstBackgroundLevel()).WillByDefault(Return(0.0));
    Expectation setVisible = EXPECT_CALL(*m_view, setBackgroundRangeVisible(true)).Times(1);
    EXPECT_CALL(*m_view, setBackgroundLevel(0.0)).Times(1).After(setVisible);

    m_presenter->updateRangeSelectors();
  }

  void test_that_updateRangeSelectors_will_update_the_hwhm_selector() {
    auto const fitFunction = getFunctionWithWorkspaceName("WorkspaceName");
    m_presenter->setFitFunction(fitFunction);

    ON_CALL(*m_model, getFirstHWHM()).WillByDefault(Return(0.00875));
    ON_CALL(*m_model, getFirstPeakCentre()).WillByDefault(Return(0.0));

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

    m_presenter->appendLastDataToSelection({"WorkspaceName", "WorkspaceName"});
  }

  void
  test_that_appendLastDataToSelection_will_add_to_the_data_selection_if_the_dataSelectionSize_and_numberOfWorkspaces_are_not_equal() {
    WorkspaceID const index(1);

    ON_CALL(*m_view, dataSelectionSize()).WillByDefault(Return(WorkspaceID(1)));

    m_presenter->appendLastDataToSelection({"WorkspaceName", "WorkspaceName"});
  }

  void test_updateDataSelection_sets_active_spectra_to_zero() {
    WorkspaceID const index1(0);
    WorkspaceID const index2(1);

    ON_CALL(*m_view, dataSelectionSize()).WillByDefault(Return(WorkspaceID(2)));

    EXPECT_CALL(*m_view, clearDataSelection()).Times(1);
    EXPECT_CALL(*m_view, appendToDataSelection("DisplayName-0")).Times(1);
    EXPECT_CALL(*m_view, appendToDataSelection("DisplayName-1")).Times(1);
    EXPECT_CALL(*m_view, setPlotSpectrum(WorkspaceIndex{0})).Times(1);
    TS_ASSERT_EQUALS(m_presenter->getActiveWorkspaceIndex(), WorkspaceIndex{0});

    m_presenter->updateDataSelection({"DisplayName-0", "DisplayName-1"});
  }

  void test_updateAvailableSpectra_uses_minmax_if_spectra_is_continuous() {
    auto spectra = FunctionModelSpectra("0-5");
    auto minmax = spectra.getMinMax();

    ON_CALL(*m_model, getWorkspace()).WillByDefault(Return(m_workspace));
    ON_CALL(*m_model, getActiveWorkspaceID()).WillByDefault(Return(WorkspaceID{0}));
    ON_CALL(*m_model, getSpectra(WorkspaceID{0})).WillByDefault(Return(spectra));

    EXPECT_CALL(*m_view, setAvailableSpectra(minmax.first, minmax.second)).Times(1);

    m_presenter->updateAvailableSpectra();
  }

  void test_updatePlots_holds_redrawing_and_updates_guess() {
    EXPECT_CALL(*m_view, allowRedraws(false)).Times(1);
    EXPECT_CALL(*m_view, allowRedraws(true)).Times(1);
    EXPECT_CALL(*m_view, redrawPlots()).Times(1);
    EXPECT_CALL(*m_view, clearPreviews()).Times(1);

    m_presenter->updatePlots();
  }

  void test_updateFit_holds_redrawing_and_updates_guess() {
    EXPECT_CALL(*m_view, allowRedraws(false)).Times(1);
    EXPECT_CALL(*m_view, allowRedraws(true)).Times(1);
    EXPECT_CALL(*m_view, redrawPlots()).Times(2);
    EXPECT_CALL(*m_view, enablePlotGuess(false)).Times(1);

    m_presenter->updateFit();
  }

  void test_updateGuess_enables_plot_guess_if_model_can_calculate_guess() {
    auto const fitFunction = getFunctionWithWorkspaceName("WorkspaceName");
    m_presenter->setFitFunction(fitFunction);
    ON_CALL(*m_model, canCalculateGuess()).WillByDefault(Return(true));
    EXPECT_CALL(*m_view, enablePlotGuess(true)).Times(1);
    EXPECT_CALL(*m_view, isPlotGuessChecked()).Times(1);
    m_presenter->updateGuess();
  }

  void test_updateGuess_disables_plot_guess_if_model_can_calculate_guess() {
    EXPECT_CALL(*m_view, enablePlotGuess(false)).Times(1);
    m_presenter->updateGuess();
  }

  void test_updateGuessAvailability_enables_plot_guess_if_model_can_calculate_guess() {
    auto const fitFunction = getFunctionWithWorkspaceName("WorkspaceName");
    m_presenter->setFitFunction(fitFunction);
    ON_CALL(*m_model, canCalculateGuess()).WillByDefault(Return(true));
    EXPECT_CALL(*m_view, enablePlotGuess(true)).Times(1);
    m_presenter->updateGuessAvailability();
  }

  void test_updateGuessAvailability_disables_plot_guess_if_model_can_calculate_guess() {
    EXPECT_CALL(*m_view, enablePlotGuess(false)).Times(1);
    m_presenter->updateGuessAvailability();
  }

private:
  std::unique_ptr<NiceMock<MockFitTab>> m_tab;
  std::unique_ptr<NiceMock<MockFitPlotView>> m_view;
  std::unique_ptr<NiceMock<MockFitPlotModel>> m_model;
  std::unique_ptr<FitPlotPresenter> m_presenter;

  MatrixWorkspace_sptr m_workspace;
  std::unique_ptr<SetUpADSWithWorkspace> m_ads;
};
