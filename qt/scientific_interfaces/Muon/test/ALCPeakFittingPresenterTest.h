// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidKernel/WarningSuppressions.h"

#include "../Muon/ALCPeakFittingPresenter.h"
#include "../Muon/IALCPeakFittingModel.h"
#include "../Muon/IALCPeakFittingView.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;
using namespace testing;

namespace boost {
template <class CharType, class CharTrait>
std::basic_ostream<CharType, CharTrait> &operator<<(std::basic_ostream<CharType, CharTrait> &out,
                                                    std::optional<QString> const &maybe) {
  if (maybe)
    out << maybe->toStdString();
  return out;
}
} // namespace boost

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockALCPeakFittingView : public IALCPeakFittingView {
public:
  MOCK_METHOD(IFunction_const_sptr, function, (std::string const &), (const, override));
  MOCK_METHOD(std::optional<std::string>, currentFunctionIndex, (), (const, override));
  MOCK_METHOD(IPeakFunction_const_sptr, peakPicker, (), (const, override));

  MOCK_METHOD(void, initialize, (), (override));
  MOCK_METHOD(void, setDataCurve, (MatrixWorkspace_sptr workspace, const std::size_t &workspaceIndex), (override));
  MOCK_METHOD(void, setFittedCurve, (MatrixWorkspace_sptr workspace, const std::size_t &workspaceIndex), (override));
  MOCK_METHOD(void, setGuessCurve, (MatrixWorkspace_sptr workspace, const std::size_t &workspaceIndex), (override));
  MOCK_METHOD(void, setPeakPickerEnabled, (bool), (override));
  MOCK_METHOD(void, setPeakPicker, (const IPeakFunction_const_sptr &), (override));
  MOCK_METHOD(void, setFunction, (const IFunction_const_sptr &), (override));
  MOCK_METHOD(void, setParameter, (std::string const &, std::string const &, double), (override));
  MOCK_METHOD(void, help, (), (override));
  MOCK_METHOD(void, changePlotGuessState, (bool), (override));

  MOCK_METHOD(void, removePlot, (const std::string &plotName), (override));
  MOCK_METHOD(void, displayError, (std::string const &), (override));
  MOCK_METHOD(void, plotGuess, (), (override));
  MOCK_METHOD(void, subscribe, (IALCPeakFittingViewSubscriber *), (override));
  MOCK_METHOD(void, onParameterChanged, (std::string const &, std::string const &), (override));
  MOCK_METHOD(void, fitRequested, (), (override));
};

class MockALCPeakFittingModel : public IALCPeakFittingModel {
public:
  virtual ~MockALCPeakFittingModel() = default;
  MOCK_METHOD(IFunction_const_sptr, fittedPeaks, (), (const, override));
  MOCK_METHOD(MatrixWorkspace_sptr, data, (), (const, override));
  MOCK_METHOD(void, fitPeaks, (IFunction_const_sptr), (override));
  MOCK_METHOD(MatrixWorkspace_sptr, guessData, (IFunction_const_sptr function, const std::vector<double> &xValues),
              (override));
  MOCK_METHOD(void, subscribe, (IALCPeakFittingModelSubscriber *), (override));
};

// DoubleNear matcher was introduced in gmock 1.7 only
MATCHER_P2(DoubleDelta, value, delta, "") { return fabs(arg - value) < delta; }

using namespace MantidQt::CustomInterfaces;

class ALCPeakFittingPresenterTest : public CxxTest::TestSuite {
  MockALCPeakFittingView *m_view;
  MockALCPeakFittingModel *m_model;
  ALCPeakFittingPresenter *m_presenter;

  IPeakFunction_sptr createGaussian(double centre, double fwhm, double height) {
    auto peak = std::dynamic_pointer_cast<IPeakFunction>(API::FunctionFactory::Instance().createFunction("Gaussian"));
    peak->setCentre(centre);
    peak->setFwhm(fwhm);
    peak->setHeight(height);
    return peak;
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ALCPeakFittingPresenterTest *createSuite() { return new ALCPeakFittingPresenterTest(); }
  static void destroySuite(ALCPeakFittingPresenterTest *suite) { delete suite; }

  ALCPeakFittingPresenterTest() {
    API::FrameworkManager::Instance(); // To make sure everything is initialized
  }

  void setUp() override {
    m_view = new NiceMock<MockALCPeakFittingView>();
    m_model = new NiceMock<MockALCPeakFittingModel>();

    m_presenter = new ALCPeakFittingPresenter(m_view, m_model);
    m_presenter->initialize();
  }

  void tearDown() override {
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_model));

    delete m_presenter;
    delete m_model;
    delete m_view;
  }

  void test_initialize() {
    MockALCPeakFittingView view;
    MockALCPeakFittingModel model;
    ALCPeakFittingPresenter presenter(&view, &model);
    EXPECT_CALL(view, initialize()).Times(1);
    presenter.initialize();
  }

  void test_fitEmptyFunction() {
    auto ws = WorkspaceCreationHelper::create2DWorkspace123(1, 3);
    ON_CALL(*m_model, data()).WillByDefault(Return(ws));
    ON_CALL(*m_view, function(std::string(""))).WillByDefault(Return(IFunction_const_sptr()));
    EXPECT_CALL(*m_view, displayError(std::string("Couldn't fit with empty function/data"))).Times(1);

    m_presenter->onFitRequested();
  }

  void test_fit() {
    auto ws = WorkspaceCreationHelper::create2DWorkspace123(1, 3);
    ON_CALL(*m_model, data()).WillByDefault(Return(ws));

    IFunction_sptr peaks = createGaussian(1, 2, 3);

    ON_CALL(*m_view, function(std::string(""))).WillByDefault(Return(peaks));

    EXPECT_CALL(*m_model,
                fitPeaks(Property(&IFunction_const_sptr::get, Property(&IFunction::asString, peaks->asString()))));

    m_presenter->onFitRequested();
  }

  void test_onDataChanged() {
    auto dataWorkspace =
        std::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceCreationHelper::create2DWorkspace123(1, 3));

    ON_CALL(*m_model, data()).WillByDefault(Return(dataWorkspace));

    EXPECT_CALL(*m_view, setDataCurve(dataWorkspace, 0));
    m_presenter->dataChanged();
  }

  void test_fittedPeaksChanged() {
    IFunction_const_sptr fitFunction = createGaussian(1, 2, 3);
    auto dataWorkspace =
        std::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceCreationHelper::create2DWorkspace123(1, 3));

    ON_CALL(*m_model, fittedPeaks()).WillByDefault(Return(fitFunction));
    ON_CALL(*m_model, data()).WillByDefault(Return(dataWorkspace));

    // TODO: check better
    EXPECT_CALL(*m_view, setFittedCurve(dataWorkspace, 1));
    EXPECT_CALL(*m_view, setFunction(fitFunction));

    m_presenter->fittedPeaksChanged();
  }

  void test_fittedPeaksChanged_toEmpty() {
    const auto dataWorkspace = WorkspaceCreationHelper::create2DWorkspace123(1, 3);

    ON_CALL(*m_model, fittedPeaks()).WillByDefault(Return(IFunction_const_sptr()));
    ON_CALL(*m_model, data()).WillByDefault(Return(dataWorkspace));

    EXPECT_CALL(*m_view, removePlot(std::string("Fit")));
    EXPECT_CALL(*m_view, setFunction(IFunction_const_sptr()));

    m_presenter->fittedPeaksChanged();
  }

  void test_onCurrentFunctionChanged_nothing() {
    ON_CALL(*m_view, currentFunctionIndex()).WillByDefault(Return(std::nullopt));

    EXPECT_CALL(*m_view, setPeakPickerEnabled(false));

    m_presenter->onCurrentFunctionChanged();
  }

  void test_onCurrentFunctionChanged_peak() {
    ON_CALL(*m_view, currentFunctionIndex()).WillByDefault(Return(std::optional<std::string>("f1")));
    ON_CALL(*m_view, function(std::string("f1"))).WillByDefault(Return(createGaussian(1, 2, 3)));

    EXPECT_CALL(*m_view, setPeakPickerEnabled(true));
    EXPECT_CALL(*m_view,
                setPeakPicker(Property(&IPeakFunction_const_sptr::get,
                                       AllOf(Property(&IPeakFunction::centre, 1), Property(&IPeakFunction::fwhm, 2),
                                             Property(&IPeakFunction::height, 3)))));

    m_presenter->onCurrentFunctionChanged();
  }

  void test_onCurrentFunctionChanged_nonPeak() {
    ON_CALL(*m_view, currentFunctionIndex()).WillByDefault(Return(std::optional<std::string>("f1")));
    ON_CALL(*m_view, function(std::string("f1")))
        .WillByDefault(Return(API::FunctionFactory::Instance().createFunction("LinearBackground")));

    EXPECT_CALL(*m_view, setPeakPickerEnabled(false));

    m_presenter->onCurrentFunctionChanged();
  }

  void test_onPeakPickerChanged() {
    ON_CALL(*m_view, currentFunctionIndex()).WillByDefault(Return(std::optional<std::string>("f1")));
    ON_CALL(*m_view, peakPicker()).WillByDefault(Return(createGaussian(4, 5, 6)));

    EXPECT_CALL(*m_view, setParameter(std::string("f1"), std::string("PeakCentre"), 4));
    EXPECT_CALL(*m_view, setParameter(std::string("f1"), std::string("Sigma"), DoubleDelta(2.123, 1E-3)));
    EXPECT_CALL(*m_view, setParameter(std::string("f1"), std::string("Height"), 6));

    m_presenter->onPeakPickerChanged();
  }

  void test_onParameterChanged_peak() {
    ON_CALL(*m_view, currentFunctionIndex()).WillByDefault(Return(std::optional<std::string>("f1")));
    ON_CALL(*m_view, function(std::string("f1"))).WillByDefault(Return(createGaussian(4, 2, 6)));
    ON_CALL(*m_view, peakPicker()).WillByDefault(Return(createGaussian(4, 5, 6)));

    EXPECT_CALL(*m_view,
                setPeakPicker(Property(&IPeakFunction_const_sptr::get,
                                       AllOf(Property(&IPeakFunction::centre, 4), Property(&IPeakFunction::fwhm, 2),
                                             Property(&IPeakFunction::height, 6)))));

    m_presenter->onParameterChanged("f1", "Sigma");
  }

  // parameterChanged signal is thrown in many scenarios - we want to update the
  // PeakPicker only
  // if it's thrown for currently selected peak function, because that's when
  // PeakPicker is displayed
  void test_onParameterChanged_notACurrentFunction() {
    ON_CALL(*m_view, currentFunctionIndex()).WillByDefault(Return(std::optional<std::string>("f2")));

    EXPECT_CALL(*m_view, setPeakPicker(_)).Times(0);

    m_presenter->onParameterChanged("f1", "Sigma");
  }

  void test_onParameterChanged_nonPeak() {
    ON_CALL(*m_view, currentFunctionIndex()).WillByDefault(Return(std::optional<std::string>("f1")));
    ON_CALL(*m_view, function(std::string("f1")))
        .WillByDefault(Return(API::FunctionFactory::Instance().createFunction("LinearBackground")));

    EXPECT_CALL(*m_view, setPeakPicker(_)).Times(0);

    m_presenter->onParameterChanged("f1", "A0");
  }

  void test_helpPage() {
    EXPECT_CALL(*m_view, help()).Times(1);
    m_view->help();
  }

  /**
   * Test that clicking "Plot guess" with no function set plots nothing
   */
  void test_plotGuess_noFunction() {
    auto dataWorkspace = WorkspaceCreationHelper::create2DWorkspace123(1, 3);

    ON_CALL(*m_model, data()).WillByDefault(Return(dataWorkspace));
    ON_CALL(*m_view, function(std::string(""))).WillByDefault(Return(IFunction_const_sptr()));

    EXPECT_CALL(*m_view, removePlot(std::string("Guess")));
    m_presenter->onPlotGuessClicked();
  }

  /**
   * Test that clicking "Plot guess" with no data plots nothing
   * (and doesn't crash)
   */
  void test_plotGuess_noData() {
    IFunction_sptr peaks = createGaussian(1, 2, 3);

    ON_CALL(*m_model, data()).WillByDefault(Return(nullptr));
    ON_CALL(*m_view, function(std::string(""))).WillByDefault(Return(peaks));

    EXPECT_CALL(*m_view, removePlot(std::string("Guess")));

    TS_ASSERT_THROWS_NOTHING(m_presenter->onPlotGuessClicked());
  }

  /**
   * Test that "Plot guess" with a function set plots a function
   */
  void test_plotGuess() {
    auto dataWorkspace =
        std::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceCreationHelper::create2DWorkspace123(1, 3));
    auto guessWorkspace =
        std::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceCreationHelper::create2DWorkspace123(1, 4));
    IFunction_const_sptr peaks = createGaussian(1, 2, 3);
    const auto xValues = dataWorkspace->x(0).rawData();

    ON_CALL(*m_model, data()).WillByDefault(Return(dataWorkspace));
    ON_CALL(*m_view, function(std::string(""))).WillByDefault(Return(peaks));
    ON_CALL(*m_model, guessData(peaks, xValues)).WillByDefault(Return(guessWorkspace));

    EXPECT_CALL(*m_view, setGuessCurve(guessWorkspace, 0));

    m_presenter->onPlotGuessClicked();
  }

  /**
   * Test that plotting a guess, then clicking again, clears the guess
   */
  void test_plotGuessAndThenClear() {
    test_plotGuess();
    EXPECT_CALL(*m_view, removePlot(std::string("Guess")));
    m_presenter->onPlotGuessClicked(); // click again i.e. "Remove guess"
  }

  /**
   * Test that errors coming from the model are displayed in the view
   */
  void test_displayError() {
    EXPECT_CALL(*m_view, displayError(std::string("Test error")));
    m_presenter->errorInModel("Test error");
  }
};
GNU_DIAG_ON_SUGGEST_OVERRIDE
