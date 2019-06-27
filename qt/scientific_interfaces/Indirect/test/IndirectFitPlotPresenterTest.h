// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_INDIRECTFITPLOTPRESENTERTEST_H_
#define MANTIDQT_INDIRECTFITPLOTPRESENTERTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "IIndirectFitPlotView.h"
#include "IndirectFitPlotPresenter.h"
#include "IndirectFittingModel.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidTestHelpers/IndirectFitDataCreationHelper.h"

#include <QString>

using namespace Mantid::API;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces;
using namespace MantidQt::CustomInterfaces::IDA;
using namespace testing;
using IDAWorkspaceIndex = MantidQt::CustomInterfaces::IDA::WorkspaceIndex;

namespace {

MultiDomainFunction_sptr getFunction(QString const &functionString) {
  auto fun = FunctionFactory::Instance().createInitialized(
      functionString.toStdString());
  return boost::dynamic_pointer_cast<MultiDomainFunction>(fun);
}

MultiDomainFunction_sptr
getFunctionWithWorkspaceName(std::string const &workspaceName, size_t nSpec) {
  auto const singleFunctionString =
      QString("(composite=CompositeFunction,$domains=i;name=LinearBackground,"
              "A0=0,A1=0,ties=(A0=0.000000,A1=0.0);"
              "(composite=Convolution,FixResolution=true;"
              "name=Resolution,Workspace=%1,WorkspaceIndex=0;name=Lorentzian,"
              "Amplitude=1,PeakCentre=0,FWHM=0.0175))")
          .arg(QString::fromStdString(workspaceName));
  QString functionString("composite=MultiDomainFunction");
  for (size_t i = 0; i < nSpec; ++i) {
    functionString += ";" + singleFunctionString;
  }
  return getFunction(functionString);
}

} // namespace

GNU_DIAG_OFF_SUGGEST_OVERRIDE

/// Mock object to mock the view
class MockIndirectFitPlotView : public IIndirectFitPlotView {
public:
  /// Signals
  void emitSelectedFitDataChanged(DatasetIndex index) {
    emit selectedFitDataChanged(index);
  }

  void emitPlotCurrentPreview() { emit plotCurrentPreview(); }

  void emitPlotSpectrumChanged(IDAWorkspaceIndex spectrum) {
    emit plotSpectrumChanged(spectrum);
  }

  void emitPlotGuessChanged(bool doPlotGuess) {
    emit plotGuessChanged(doPlotGuess);
  }

  void emitStartXChanged(double startX) { emit startXChanged(startX); }

  void emitEndXChanged(double endX) { emit endXChanged(endX); }

  void emitHWHMMinimumChanged(double minimum) {
    emit hwhmMinimumChanged(minimum);
  }

  void emitHWHMMaximumChanged(double maximum) {
    emit hwhmMaximumChanged(maximum);
  }

  void emitBackgroundChanged(double value) { emit backgroundChanged(value); }

  /// Public methods
  MOCK_CONST_METHOD0(getSelectedSpectrum, IDAWorkspaceIndex());
  MOCK_CONST_METHOD0(getSelectedSpectrumIndex, SpectrumRowIndex());
  MOCK_CONST_METHOD0(getSelectedDataIndex, DatasetIndex());
  MOCK_CONST_METHOD0(dataSelectionSize, DatasetIndex());
  MOCK_CONST_METHOD0(isPlotGuessChecked, bool());

  MOCK_METHOD0(hideMultipleDataSelection, void());
  MOCK_METHOD0(showMultipleDataSelection, void());

  MOCK_METHOD2(setAvailableSpectra,
               void(IDAWorkspaceIndex minimum, IDAWorkspaceIndex maximum));
  MOCK_METHOD2(setAvailableSpectra,
               void(std::vector<IDAWorkspaceIndex>::const_iterator const &from,
                    std::vector<IDAWorkspaceIndex>::const_iterator const &to));

  MOCK_METHOD1(setMinimumSpectrum, void(int minimum));
  MOCK_METHOD1(setMaximumSpectrum, void(int maximum));
  MOCK_METHOD1(setPlotSpectrum, void(IDAWorkspaceIndex spectrum));
  MOCK_METHOD1(appendToDataSelection, void(std::string const &dataName));
  MOCK_METHOD2(setNameInDataSelection,
               void(std::string const &dataName, DatasetIndex index));
  MOCK_METHOD0(clearDataSelection, void());

  MOCK_METHOD4(plotInTopPreview,
               void(QString const &name,
                    Mantid::API::MatrixWorkspace_sptr workspace,
                    IDAWorkspaceIndex spectrum, Qt::GlobalColor colour));
  MOCK_METHOD4(plotInBottomPreview,
               void(QString const &name,
                    Mantid::API::MatrixWorkspace_sptr workspace,
                    IDAWorkspaceIndex spectrum, Qt::GlobalColor colour));

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

  MOCK_METHOD1(setBackgroundRangeVisible, void(bool visible));
  MOCK_METHOD1(setHWHMRangeVisible, void(bool visible));

  MOCK_CONST_METHOD1(displayMessage, void(std::string const &message));

  /// Public Slots
  MOCK_METHOD0(clearTopPreview, void());
  MOCK_METHOD0(clearBottomPreview, void());
  MOCK_METHOD0(clear, void());

  MOCK_METHOD2(setHWHMRange, void(double minimum, double maximum));
  MOCK_METHOD1(setHWHMMinimum, void(double minimum));
  MOCK_METHOD1(setHWHMMaximum, void(double maximum));
};

class MockIndirectFittingModel : public IndirectFittingModel {
public:
  /// Public methods
  MOCK_CONST_METHOD1(getWorkspace, MatrixWorkspace_sptr(DatasetIndex index));
  MOCK_CONST_METHOD2(getFittingRange,
                     std::pair<double, double>(DatasetIndex dataIndex,
                                               IDAWorkspaceIndex spectrum));
  MOCK_CONST_METHOD3(createDisplayName,
                     std::string(std::string const &formatString,
                                 std::string const &rangeDelimiter,
                                 DatasetIndex dataIndex));
  MOCK_CONST_METHOD0(isMultiFit, bool());
  MOCK_CONST_METHOD0(numberOfWorkspaces, DatasetIndex());
  MOCK_CONST_METHOD0(getFittingFunction, MultiDomainFunction_sptr());

  MOCK_METHOD3(setStartX, void(double startX, DatasetIndex dataIndex,
                               IDAWorkspaceIndex spectrum));
  MOCK_METHOD3(setEndX, void(double endX, DatasetIndex dataIndex,
                             IDAWorkspaceIndex spectrum));

  MOCK_METHOD3(setDefaultParameterValue,
               void(std::string const &name, double value,
                    DatasetIndex dataIndex));

private:
  std::string sequentialFitOutputName() const override { return ""; };
  std::string simultaneousFitOutputName() const override { return ""; };
  std::string singleFitOutputName(DatasetIndex index,
                                  IDAWorkspaceIndex spectrum) const override {
    UNUSED_ARG(index);
    UNUSED_ARG(spectrum);
    return "";
  };

  std::vector<std::string> getSpectrumDependentAttributes() const override {
    return {};
  };
};

GNU_DIAG_ON_SUGGEST_OVERRIDE

class IndirectFitPlotPresenterTest : public CxxTest::TestSuite {
public:
  /// Needed to make sure everything is initialized
  IndirectFitPlotPresenterTest() { FrameworkManager::Instance(); }

  static IndirectFitPlotPresenterTest *createSuite() {
    return new IndirectFitPlotPresenterTest();
  }

  static void destroySuite(IndirectFitPlotPresenterTest *suite) {
    delete suite;
  }

  void setUp() override {
    /// Note that the IndirectFitPlotModel could not be mocked as the Presenter
    /// takes an IndirectFittingModel. This means the IndirectFittingModel is
    /// mocked instead - which is a good substitute anyway
    m_view = std::make_unique<NiceMock<MockIndirectFitPlotView>>();
    m_fittingModel = std::make_unique<NiceMock<MockIndirectFittingModel>>();
    m_presenter = std::make_unique<IndirectFitPlotPresenter>(
        std::move(m_fittingModel.get()), std::move(m_view.get()));

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
    IDAWorkspaceIndex const selectedSpectrum{3};

    ON_CALL(*m_view, getSelectedSpectrum())
        .WillByDefault(Return(selectedSpectrum));
    ON_CALL(*m_fittingModel, isMultiFit()).WillByDefault(Return(false));

    EXPECT_CALL(*m_view, getSelectedSpectrum())
        .Times(1)
        .WillOnce(Return(selectedSpectrum));
    EXPECT_CALL(*m_fittingModel, isMultiFit()).Times(1).WillOnce(Return(false));

    m_view->getSelectedSpectrum();
    m_fittingModel->isMultiFit();
  }

  void
  test_that_invoking_a_presenter_method_will_call_the_relevant_methods_in_the_model_and_view() {
    DatasetIndex const selectionSize{2};

    ON_CALL(*m_view, dataSelectionSize()).WillByDefault(Return(selectionSize));

    EXPECT_CALL(*m_fittingModel, numberOfWorkspaces())
        .Times(2)
        .WillRepeatedly(Return(DatasetIndex{1}));
    EXPECT_CALL(*m_view, dataSelectionSize())
        .Times(1)
        .WillOnce(Return(selectionSize));

    m_presenter->appendLastDataToSelection();
  }

  ///----------------------------------------------------------------------
  /// Unit Tests that test the signals (only the view emits signals here)
  ///----------------------------------------------------------------------

  void test_that_the_selectedFitDataChanged_signal_will_set_the_activeIndex() {
    m_view->emitSelectedFitDataChanged(DatasetIndex{1});
    TS_ASSERT_EQUALS(m_presenter->getSelectedDataIndex(), DatasetIndex{1});
  }

  void
  test_that_the_selectedFitDataChanged_signal_will_set_the_available_spectra() {
    DatasetIndex const index{0};
    ON_CALL(*m_fittingModel, getWorkspace(index))
        .WillByDefault(Return(m_ads->retrieveWorkspace("WorkspaceName")));

    EXPECT_CALL(*m_view,
                setAvailableSpectra(IDAWorkspaceIndex{0}, IDAWorkspaceIndex{9}))
        .Times(1);

    m_view->emitSelectedFitDataChanged(index);
  }

  void
  test_that_the_selectedFitDataChanged_signal_will_disable_selectors_when_there_is_no_workspace() {
    DatasetIndex const index{0};
    ON_CALL(*m_fittingModel, getWorkspace(index))
        .WillByDefault(Return(nullptr));

    EXPECT_CALL(*m_view, enableSpectrumSelection(false)).Times(1);
    EXPECT_CALL(*m_view, enableFitRangeSelection(false)).Times(1);

    m_view->emitSelectedFitDataChanged(index);
  }

  void
  test_that_the_selectedFitDataChanged_signal_will_plot_the_input_when_there_is_only_an_input_workspace() {
    DatasetIndex const index{0};
    ON_CALL(*m_fittingModel, getWorkspace(index))
        .WillByDefault(Return(m_ads->retrieveWorkspace("WorkspaceName")));

    EXPECT_CALL(*m_fittingModel, getWorkspace(index)).Times(3);
    EXPECT_CALL(*m_view, removeFromBottomPreview(QString("Difference")))
        .Times(1);

    m_view->emitSelectedFitDataChanged(index);
  }

  void
  test_that_the_selectedFitDataChanged_signal_will_clear_the_plots_when_there_is_no_input_workspace() {
    DatasetIndex const index{0};
    ON_CALL(*m_fittingModel, getWorkspace(index))
        .WillByDefault(Return(nullptr));

    EXPECT_CALL(*m_fittingModel, getWorkspace(index)).Times(2);
    EXPECT_CALL(*m_view, clear()).Times(1);

    m_view->emitSelectedFitDataChanged(index);
  }

  void
  test_that_the_selectedFitDataChanged_signal_will_set_the_minimum_and_maximum_of_the_fit_range() {
    DatasetIndex const index{0};
    auto const range = std::make_pair(1.0, 2.0);
    ON_CALL(*m_fittingModel, getFittingRange(index, IDAWorkspaceIndex{0}))
        .WillByDefault(Return(range));

    EXPECT_CALL(*m_fittingModel, getFittingRange(index, IDAWorkspaceIndex{0}))
        .Times(2)
        .WillRepeatedly(Return(range));
    EXPECT_CALL(*m_view, setFitRangeMinimum(1.0)).Times(2);
    EXPECT_CALL(*m_view, setFitRangeMaximum(2.0)).Times(2);

    m_view->emitSelectedFitDataChanged(index);
  }

  void
  test_that_the_selectedFitDataChanged_signal_will_enable_PlotGuess_when_there_is_a_fit_function_and_workspace() {
    DatasetIndex const index{0};
    std::string const workspaceName("WorkspaceName");
    auto const fitFunction = getFunctionWithWorkspaceName(workspaceName, 10);

    ON_CALL(*m_fittingModel, getFittingFunction())
        .WillByDefault(Return(fitFunction));
    ON_CALL(*m_fittingModel, getWorkspace(index))
        .WillByDefault(Return(m_ads->retrieveWorkspace(workspaceName)));

    EXPECT_CALL(*m_view, enablePlotGuess(true)).Times(1);

    m_view->emitSelectedFitDataChanged(index);
  }

  void
  test_that_the_selectedFitDataChanged_signal_will_disable_the_guess_plot_when_there_is_no_fit_function() {
    DatasetIndex const index{0};
    ON_CALL(*m_fittingModel, getWorkspace(index))
        .WillByDefault(Return(m_ads->retrieveWorkspace("WorkspaceName")));

    EXPECT_CALL(*m_view, enablePlotGuess(false)).Times(1);

    m_view->emitSelectedFitDataChanged(index);
  }

  void test_that_the_plotSpectrumChanged_signal_will_set_the_active_spectrum() {
    m_view->emitPlotSpectrumChanged(IDAWorkspaceIndex{2});
    TS_ASSERT_EQUALS(m_presenter->getSelectedSpectrum(), IDAWorkspaceIndex{2});
  }

  void
  test_that_the_plotSpectrumChanged_signal_will_plot_the_input_when_there_is_only_an_input_workspace() {
    DatasetIndex const index{0};
    ON_CALL(*m_fittingModel, getWorkspace(index))
        .WillByDefault(Return(m_ads->retrieveWorkspace("WorkspaceName")));

    EXPECT_CALL(*m_fittingModel, getWorkspace(index)).Times(2);
    EXPECT_CALL(*m_view, removeFromBottomPreview(QString("Difference")))
        .Times(1);

    m_view->emitPlotSpectrumChanged(IDAWorkspaceIndex{0});
  }

  void
  test_that_the_plotSpectrumChanged_signal_will_clear_the_plots_when_there_is_no_input_workspace() {
    DatasetIndex const index{0};
    ON_CALL(*m_fittingModel, getWorkspace(index))
        .WillByDefault(Return(nullptr));

    EXPECT_CALL(*m_fittingModel, getWorkspace(index)).Times(1);
    EXPECT_CALL(*m_view, clear()).Times(1);

    m_view->emitPlotSpectrumChanged(IDAWorkspaceIndex{0});
  }

  void
  test_that_the_plotSpectrumChanged_signal_will_set_the_minimum_and_maximum_of_the_fit_range() {
    DatasetIndex const index{0};
    auto const range = std::make_pair(1.0, 2.0);
    ON_CALL(*m_fittingModel, getFittingRange(index, IDAWorkspaceIndex{0}))
        .WillByDefault(Return(range));

    EXPECT_CALL(*m_fittingModel, getFittingRange(index, IDAWorkspaceIndex{0}))
        .Times(2)
        .WillRepeatedly(Return(range));
    EXPECT_CALL(*m_view, setFitRangeMinimum(1.0)).Times(2);
    EXPECT_CALL(*m_view, setFitRangeMaximum(2.0)).Times(2);

    m_view->emitPlotSpectrumChanged(IDAWorkspaceIndex{0});
  }

  void
  test_that_the_plotCurrentPreview_signal_will_display_an_error_message_if_there_is_no_input_workspace() {
    DatasetIndex const index{0};
    std::string const message("Workspace not found - data may not be loaded.");

    ON_CALL(*m_fittingModel, getWorkspace(index))
        .WillByDefault(Return(nullptr));

    Expectation getWorkspace =
        EXPECT_CALL(*m_fittingModel, getWorkspace(index)).Times(1);
    EXPECT_CALL(*m_view, displayMessage(message)).Times(1).After(getWorkspace);

    m_view->emitPlotCurrentPreview();
  }

  void
  test_that_the_plotGuessChanged_signal_will_not_clear_the_guess_plot_when_passed_true() {
    DatasetIndex const index{0};
    std::string const workspaceName("WorkspaceName");
    auto const range = std::make_pair(1.0, 2.0);
    auto const fitFunction = getFunctionWithWorkspaceName(workspaceName, 10);

    ON_CALL(*m_fittingModel, getFittingRange(index, IDAWorkspaceIndex{0}))
        .WillByDefault(Return(range));
    ON_CALL(*m_fittingModel, getFittingFunction())
        .WillByDefault(Return(fitFunction));
    ON_CALL(*m_fittingModel, getWorkspace(index))
        .WillByDefault(Return(m_ads->retrieveWorkspace(workspaceName)));

    EXPECT_CALL(*m_view, removeFromTopPreview(QString("Guess"))).Times(0);

    m_view->emitPlotGuessChanged(true);
  }

  void
  test_that_the_plotGuessChanged_signal_will_clear_the_guess_plot_when_passed_false() {
    DatasetIndex const index{0};
    ON_CALL(*m_fittingModel, getWorkspace(index))
        .WillByDefault(Return(m_ads->retrieveWorkspace("WorkspaceName")));

    EXPECT_CALL(*m_view, removeFromTopPreview(QString("Guess"))).Times(1);

    m_view->emitPlotGuessChanged(false);
  }

  void test_that_the_startXChanged_signal_will_set_the_fitting_models_startX() {
    auto const range = std::make_pair(0.0, 2.0);
    ON_CALL(*m_fittingModel,
            getFittingRange(DatasetIndex{0}, IDAWorkspaceIndex{0}))
        .WillByDefault(Return(range));

    EXPECT_CALL(*m_fittingModel,
                setStartX(1.0, DatasetIndex{0}, IDAWorkspaceIndex{0}))
        .Times(1);

    m_view->emitStartXChanged(1.0);
  }

  void test_that_the_endXChanged_signal_will_set_the_fitting_models_endX() {
    EXPECT_CALL(*m_fittingModel,
                setEndX(2.0, DatasetIndex{0}, IDAWorkspaceIndex{0}))
        .Times(1);
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

  // void
  // test_that_the_backgroundChanged_signal_will_set_the_functions_background()
  // {
  //  double const background(1.2);
  //  auto const fitFunction = getFunctionWithWorkspaceName("WorkspaceName",
  //  10);

  //  ON_CALL(*m_fittingModel, getFittingFunction())
  //      .WillByDefault(Return(fitFunction));

  //  Expectation setDefault =
  //      EXPECT_CALL(*m_fittingModel,
  //                  setDefaultParameterValue("A0", background, 0))
  //          .Times(1);
  //  EXPECT_CALL(*m_fittingModel, getFittingFunction())
  //      .Times(1)
  //      .After(setDefault);

  //  m_view->emitBackgroundChanged(background);
  //}

  ///----------------------------------------------------------------------
  /// Unit Tests that test the methods and slots
  ///----------------------------------------------------------------------

  void
  test_that_getSelectedSpectrumIndex_will_get_the_selected_spectrum_from_the_view() {
    EXPECT_CALL(*m_view, getSelectedSpectrumIndex())
        .Times(1)
        .WillOnce(Return(SpectrumRowIndex{0}));
    m_presenter->getSelectedSpectrumIndex();
  }

  void
  test_that_isCurrentlySelected_returns_true_if_the_index_and_spectrum_given_are_selected() {
    m_view->emitSelectedFitDataChanged(DatasetIndex{2});
    TS_ASSERT(
        m_presenter->isCurrentlySelected(DatasetIndex{2}, IDAWorkspaceIndex{0}));
  }

  void
  test_that_isCurrentlySelected_returns_false_if_the_index_and_spectrum_given_are_not_selected() {
    m_view->emitSelectedFitDataChanged(DatasetIndex{2});
    TS_ASSERT(!m_presenter->isCurrentlySelected(DatasetIndex{0},
                                                IDAWorkspaceIndex{0}));
  }

  void test_that_setStartX_will_set_the_fit_range_minimum_in_the_view() {
    EXPECT_CALL(*m_view, setFitRangeMinimum(2.0)).Times(1);
    m_presenter->setStartX(2.0);
  }

  void test_that_setEndX_will_set_the_fit_range_maximum_in_the_view() {
    EXPECT_CALL(*m_view, setFitRangeMaximum(3.0)).Times(1);
    m_presenter->setEndX(3.0);
  }

  void
  test_that_hideMultipleDataSelection_will_call_hideMultipleDataSelection_in_the_view() {
    EXPECT_CALL(*m_view, hideMultipleDataSelection()).Times(1);
    m_presenter->hideMultipleDataSelection();
  }

  void
  test_that_showMultipleDataSelection_will_call_showMultipleDataSelection_in_the_view() {
    EXPECT_CALL(*m_view, showMultipleDataSelection()).Times(1);
    m_presenter->showMultipleDataSelection();
  }

  void test_that_updateRangeSelectors_will_update_the_background_selector() {
    auto const fitFunction = getFunctionWithWorkspaceName("WorkspaceName", 10);

    ON_CALL(*m_fittingModel, getFittingFunction())
        .WillByDefault(Return(fitFunction));

    Expectation setVisible =
        EXPECT_CALL(*m_view, setBackgroundRangeVisible(true)).Times(1);
    EXPECT_CALL(*m_view, setBackgroundLevel(0.0)).Times(1).After(setVisible);

    m_presenter->updateRangeSelectors();
  }

  // void test_that_updateRangeSelectors_will_update_the_hwhm_selector() {
  //  auto const fitFunction = getFunctionWithWorkspaceName("WorkspaceName",
  //  10);

  //  ON_CALL(*m_fittingModel, getFittingFunction())
  //      .WillByDefault(Return(fitFunction));

  //  Expectation setVisible =
  //      EXPECT_CALL(*m_view, setHWHMRangeVisible(true)).Times(1);
  //  EXPECT_CALL(*m_view, setHWHMMinimum(-0.00875)).Times(1).After(setVisible);
  //  EXPECT_CALL(*m_view, setHWHMMaximum(0.00875)).Times(1).After(setVisible);

  //  m_presenter->updateRangeSelectors();
  //}

  void
  test_that_appendLastDataToSelection_will_set_the_name_of_the_data_selection_if_the_dataSelectionSize_and_numberOfWorkspaces_are_equal() {
    DatasetIndex const index{1};

    ON_CALL(*m_view, dataSelectionSize()).WillByDefault(Return(DatasetIndex{2}));
    ON_CALL(*m_fittingModel, numberOfWorkspaces()).WillByDefault(Return(DatasetIndex{2}));
    ON_CALL(*m_fittingModel, createDisplayName("%1% (%2%)", "-", index))
        .WillByDefault(Return("DisplayName-1"));
    ON_CALL(*m_fittingModel, getWorkspace(index))
        .WillByDefault(Return(m_ads->retrieveWorkspace("WorkspaceName")));

    Expectation createName =
        EXPECT_CALL(*m_fittingModel, createDisplayName("%1% (%2%)", "-", index))
            .Times(1);
    EXPECT_CALL(*m_view, setNameInDataSelection("DisplayName-1", index))
        .Times(1)
        .After(createName);

    m_presenter->appendLastDataToSelection();
  }

  void
  test_that_appendLastDataToSelection_will_add_to_the_data_selection_if_the_dataSelectionSize_and_numberOfWorkspaces_are_not_equal() {
    DatasetIndex const index{1};

    ON_CALL(*m_view, dataSelectionSize()).WillByDefault(Return(DatasetIndex{1}));
    ON_CALL(*m_fittingModel, numberOfWorkspaces())
        .WillByDefault(Return(DatasetIndex{2}));
    ON_CALL(*m_fittingModel, createDisplayName("%1% (%2%)", "-", index))
        .WillByDefault(Return("DisplayName-1"));
    ON_CALL(*m_fittingModel, getWorkspace(index))
        .WillByDefault(Return(m_ads->retrieveWorkspace("WorkspaceName")));

    Expectation createName =
        EXPECT_CALL(*m_fittingModel, createDisplayName("%1% (%2%)", "-", index))
            .Times(1);
    EXPECT_CALL(*m_view, appendToDataSelection("DisplayName-1"))
        .Times(1)
        .After(createName);

    m_presenter->appendLastDataToSelection();
  }

  void
  test_that_updateSelectedDataName_will_update_the_name_in_the_data_selection() {
    DatasetIndex const index{0};

    ON_CALL(*m_fittingModel, createDisplayName("%1% (%2%)", "-", index))
        .WillByDefault(Return("DisplayName-1"));
    ON_CALL(*m_fittingModel, getWorkspace(index))
        .WillByDefault(Return(m_ads->retrieveWorkspace("WorkspaceName")));

    Expectation createName =
        EXPECT_CALL(*m_fittingModel, createDisplayName("%1% (%2%)", "-", index))
            .Times(1);
    EXPECT_CALL(*m_view, setNameInDataSelection("DisplayName-1", DatasetIndex{0}))
        .Times(1)
        .After(createName);

    m_presenter->updateSelectedDataName();
  }

private:
  std::unique_ptr<MockIndirectFitPlotView> m_view;
  std::unique_ptr<MockIndirectFittingModel> m_fittingModel;
  std::unique_ptr<IndirectFitPlotPresenter> m_presenter;

  SetUpADSWithWorkspace *m_ads;
};

#endif