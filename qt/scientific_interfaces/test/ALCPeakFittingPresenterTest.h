// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMINTERFACES_ALCPEAKFITTINGTEST_H_
#define MANTIDQT_CUSTOMINTERFACES_ALCPEAKFITTINGTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidKernel/WarningSuppressions.h"

#include "../Muon/ALCPeakFittingPresenter.h"
#include "../Muon/IALCPeakFittingModel.h"
#include "../Muon/IALCPeakFittingView.h"

#include <qwt_data.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;
using namespace testing;

namespace boost {
template <class CharType, class CharTrait>
std::basic_ostream<CharType, CharTrait> &
operator<<(std::basic_ostream<CharType, CharTrait> &out,
           optional<QString> const &maybe) {
  if (maybe)
    out << maybe->toStdString();
  return out;
}
} // namespace boost

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockALCPeakFittingView : public IALCPeakFittingView {
public:
  void requestFit() { emit fitRequested(); }
  void changeCurrentFunction() { emit currentFunctionChanged(); }
  void changePeakPicker() { emit peakPickerChanged(); }
  void changeParameter(const QString &funcIndex, const QString &paramName) {
    emit parameterChanged(funcIndex, paramName);
  }
  void plotGuess() override { emit plotGuessClicked(); }

  MOCK_CONST_METHOD1(function, IFunction_const_sptr(QString));
  MOCK_CONST_METHOD0(currentFunctionIndex, boost::optional<QString>());
  MOCK_CONST_METHOD0(peakPicker, IPeakFunction_const_sptr());

  MOCK_METHOD0(initialize, void());
  MOCK_METHOD2(setDataCurve, void(MatrixWorkspace_sptr workspace,
                                  const std::size_t &workspaceIndex));
  MOCK_METHOD2(setFittedCurve, void(MatrixWorkspace_sptr workspace,
                                    const std::size_t &workspaceIndex));
  MOCK_METHOD2(setGuessCurve, void(MatrixWorkspace_sptr workspace,
                                   const std::size_t &workspaceIndex));
  MOCK_METHOD1(setPeakPickerEnabled, void(bool));
  MOCK_METHOD1(setPeakPicker, void(const IPeakFunction_const_sptr &));
  MOCK_METHOD1(setFunction, void(const IFunction_const_sptr &));
  MOCK_METHOD3(setParameter, void(const QString &, const QString &, double));
  MOCK_METHOD1(displayError, void(const QString &));
  MOCK_METHOD0(help, void());
  MOCK_METHOD1(changePlotGuessState, void(bool));

  MOCK_METHOD1(removePlot, void(const QString &plotName));
};

class MockALCPeakFittingModel : public IALCPeakFittingModel {
public:
  void changeFittedPeaks() { emit fittedPeaksChanged(); }
  void changeData() { emit dataChanged(); }
  void setError(const QString &message) { emit errorInModel(message); }

  MOCK_CONST_METHOD0(fittedPeaks, IFunction_const_sptr());
  MOCK_CONST_METHOD0(data, MatrixWorkspace_sptr());
  MOCK_METHOD1(fitPeaks, void(IFunction_const_sptr));
  MOCK_METHOD2(guessData,
               MatrixWorkspace_sptr(IFunction_const_sptr function,
                                    const std::vector<double> &xValues));
};

// DoubleNear matcher was introduced in gmock 1.7 only
MATCHER_P2(DoubleDelta, value, delta, "") { return fabs(arg - value) < delta; }

using namespace MantidQt::CustomInterfaces;

class ALCPeakFittingPresenterTest : public CxxTest::TestSuite {
  MockALCPeakFittingView *m_view;
  MockALCPeakFittingModel *m_model;
  ALCPeakFittingPresenter *m_presenter;

  IPeakFunction_sptr createGaussian(double centre, double fwhm, double height) {
    auto peak = boost::dynamic_pointer_cast<IPeakFunction>(
        API::FunctionFactory::Instance().createFunction("Gaussian"));
    peak->setCentre(centre);
    peak->setFwhm(fwhm);
    peak->setHeight(height);
    return peak;
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ALCPeakFittingPresenterTest *createSuite() {
    return new ALCPeakFittingPresenterTest();
  }
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
    ON_CALL(*m_view, function(QString("")))
        .WillByDefault(Return(IFunction_const_sptr()));
    EXPECT_CALL(*m_view,
                displayError(QString("Couldn't fit with empty function/data")))
        .Times(1);

    m_view->requestFit();
  }

  void test_fit() {
    auto ws = WorkspaceCreationHelper::create2DWorkspace123(1, 3);
    ON_CALL(*m_model, data()).WillByDefault(Return(ws));

    IFunction_sptr peaks = createGaussian(1, 2, 3);

    ON_CALL(*m_view, function(QString(""))).WillByDefault(Return(peaks));

    EXPECT_CALL(*m_model, fitPeaks(Property(&IFunction_const_sptr::get,
                                            Property(&IFunction::asString,
                                                     peaks->asString()))));

    m_view->requestFit();
  }

  void test_onDataChanged() {
    auto dataWorkspace = boost::dynamic_pointer_cast<MatrixWorkspace>(
        WorkspaceCreationHelper::create2DWorkspace123(1, 3));

    ON_CALL(*m_model, data()).WillByDefault(Return(dataWorkspace));

    EXPECT_CALL(*m_view, setDataCurve(dataWorkspace, 0));
    m_model->changeData();
  }

  void test_onFittedPeaksChanged() {
    IFunction_const_sptr fitFunction = createGaussian(1, 2, 3);
    auto dataWorkspace = boost::dynamic_pointer_cast<MatrixWorkspace>(
        WorkspaceCreationHelper::create2DWorkspace123(1, 3));

    ON_CALL(*m_model, fittedPeaks()).WillByDefault(Return(fitFunction));
    ON_CALL(*m_model, data()).WillByDefault(Return(dataWorkspace));

    // TODO: check better
    EXPECT_CALL(*m_view, setFittedCurve(dataWorkspace, 1));
    EXPECT_CALL(*m_view, setFunction(fitFunction));

    m_model->changeFittedPeaks();
  }

  void test_onFittedPeaksChanged_toEmpty() {
    const auto dataWorkspace =
        WorkspaceCreationHelper::create2DWorkspace123(1, 3);

    ON_CALL(*m_model, fittedPeaks())
        .WillByDefault(Return(IFunction_const_sptr()));
    ON_CALL(*m_model, data()).WillByDefault(Return(dataWorkspace));

    EXPECT_CALL(*m_view, removePlot(QString("Fit")));
    EXPECT_CALL(*m_view, setFunction(IFunction_const_sptr()));

    m_model->changeFittedPeaks();
  }

  void test_onCurrentFunctionChanged_nothing() {
    ON_CALL(*m_view, currentFunctionIndex()).WillByDefault(Return(boost::none));

    EXPECT_CALL(*m_view, setPeakPickerEnabled(false));

    m_view->changeCurrentFunction();
  }

  void test_onCurrentFunctionChanged_peak() {
    ON_CALL(*m_view, currentFunctionIndex())
        .WillByDefault(Return(boost::optional<QString>("f1")));
    ON_CALL(*m_view, function(QString("f1")))
        .WillByDefault(Return(createGaussian(1, 2, 3)));

    EXPECT_CALL(*m_view, setPeakPickerEnabled(true));
    EXPECT_CALL(*m_view, setPeakPicker(Property(
                             &IPeakFunction_const_sptr::get,
                             AllOf(Property(&IPeakFunction::centre, 1),
                                   Property(&IPeakFunction::fwhm, 2),
                                   Property(&IPeakFunction::height, 3)))));

    m_view->changeCurrentFunction();
  }

  void test_onCurrentFunctionChanged_nonPeak() {
    ON_CALL(*m_view, currentFunctionIndex())
        .WillByDefault(Return(boost::optional<QString>("f1")));
    ON_CALL(*m_view, function(QString("f1")))
        .WillByDefault(Return(API::FunctionFactory::Instance().createFunction(
            "LinearBackground")));

    EXPECT_CALL(*m_view, setPeakPickerEnabled(false));

    m_view->changeCurrentFunction();
  }

  void test_onPeakPickerChanged() {
    ON_CALL(*m_view, currentFunctionIndex())
        .WillByDefault(Return(boost::optional<QString>("f1")));
    ON_CALL(*m_view, peakPicker())
        .WillByDefault(Return(createGaussian(4, 5, 6)));

    EXPECT_CALL(*m_view, setParameter(QString("f1"), QString("PeakCentre"), 4));
    EXPECT_CALL(*m_view, setParameter(QString("f1"), QString("Sigma"),
                                      DoubleDelta(2.123, 1E-3)));
    EXPECT_CALL(*m_view, setParameter(QString("f1"), QString("Height"), 6));

    m_view->changePeakPicker();
  }

  void test_onParameterChanged_peak() {
    ON_CALL(*m_view, currentFunctionIndex())
        .WillByDefault(Return(boost::optional<QString>("f1")));
    ON_CALL(*m_view, function(QString("f1")))
        .WillByDefault(Return(createGaussian(4, 2, 6)));
    ON_CALL(*m_view, peakPicker())
        .WillByDefault(Return(createGaussian(4, 5, 6)));

    EXPECT_CALL(*m_view, setPeakPicker(Property(
                             &IPeakFunction_const_sptr::get,
                             AllOf(Property(&IPeakFunction::centre, 4),
                                   Property(&IPeakFunction::fwhm, 2),
                                   Property(&IPeakFunction::height, 6)))));

    m_view->changeParameter(QString("f1"), QString("Sigma"));
  }

  // parameterChanged signal is thrown in many scenarios - we want to update the
  // PeakPicker only
  // if it's thrown for currently selected peak function, because that's when
  // PeakPicker is displayed
  void test_onParameterChanged_notACurrentFunction() {
    ON_CALL(*m_view, currentFunctionIndex())
        .WillByDefault(Return(boost::optional<QString>("f2")));

    EXPECT_CALL(*m_view, setPeakPicker(_)).Times(0);

    m_view->changeParameter(QString("f1"), QString("Sigma"));
  }

  void test_onParameterChanged_nonPeak() {
    ON_CALL(*m_view, currentFunctionIndex())
        .WillByDefault(Return(boost::optional<QString>("f1")));
    ON_CALL(*m_view, function(QString("f1")))
        .WillByDefault(Return(API::FunctionFactory::Instance().createFunction(
            "LinearBackground")));

    EXPECT_CALL(*m_view, setPeakPicker(_)).Times(0);

    m_view->changeParameter(QString("f1"), QString("A0"));
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
    ON_CALL(*m_view, function(QString("")))
        .WillByDefault(Return(IFunction_const_sptr()));

    EXPECT_CALL(*m_view, removePlot(QString("Guess")));
    m_view->plotGuess();
  }

  /**
   * Test that clicking "Plot guess" with no data plots nothing
   * (and doesn't crash)
   */
  void test_plotGuess_noData() {
    IFunction_sptr peaks = createGaussian(1, 2, 3);

    ON_CALL(*m_model, data()).WillByDefault(Return(nullptr));
    ON_CALL(*m_view, function(QString(""))).WillByDefault(Return(peaks));

    EXPECT_CALL(*m_view, removePlot(QString("Guess")));

    TS_ASSERT_THROWS_NOTHING(m_view->plotGuess());
  }

  /**
   * Test that "Plot guess" with a function set plots a function
   */
  void test_plotGuess() {
    auto dataWorkspace = boost::dynamic_pointer_cast<MatrixWorkspace>(
        WorkspaceCreationHelper::create2DWorkspace123(1, 3));
    auto guessWorkspace = boost::dynamic_pointer_cast<MatrixWorkspace>(
        WorkspaceCreationHelper::create2DWorkspace123(1, 4));
    IFunction_const_sptr peaks = createGaussian(1, 2, 3);
    const auto xValues = dataWorkspace->x(0).rawData();

    ON_CALL(*m_model, data()).WillByDefault(Return(dataWorkspace));
    ON_CALL(*m_view, function(QString(""))).WillByDefault(Return(peaks));
    ON_CALL(*m_model, guessData(peaks, xValues))
        .WillByDefault(Return(guessWorkspace));

    EXPECT_CALL(*m_view, setGuessCurve(guessWorkspace, 0));

    m_view->plotGuess();
  }

  /**
   * Test that plotting a guess, then clicking again, clears the guess
   */
  void test_plotGuessAndThenClear() {
    test_plotGuess();
    EXPECT_CALL(*m_view, removePlot(QString("Guess")));
    m_view->plotGuess(); // click again i.e. "Remove guess"
  }

  /**
   * Test that errors coming from the model are displayed in the view
   */
  void test_displayError() {
    EXPECT_CALL(*m_view, displayError(QString("Test error")));
    m_model->setError("Test error");
  }
};
GNU_DIAG_ON_SUGGEST_OVERRIDE
#endif /* MANTIDQT_CUSTOMINTERFACES_ALCPEAKFITTINGTEST_H_ */
