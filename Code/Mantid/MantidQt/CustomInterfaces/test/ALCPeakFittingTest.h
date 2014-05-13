#ifndef MANTIDQT_CUSTOMINTERFACES_ALCPEAKFITTINGTEST_H_
#define MANTIDQT_CUSTOMINTERFACES_ALCPEAKFITTINGTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FrameworkManager.h"

#include "MantidQtCustomInterfaces/Muon/IALCPeakFittingView.h"
#include "MantidQtCustomInterfaces/Muon/ALCPeakFittingPresenter.h"

using namespace Mantid;
using namespace MantidQt::CustomInterfaces;
using namespace testing;

class MockALCPeakFittingView : public IALCPeakFittingView
{
public:
  void requestFit() { emit fitRequested(); }
  void changeCurrentFunction() { emit currentFunctionChanged(); }
  void changePeakPicker() { emit peakPickerChanged(); }
  void changeParameter(const QString& funcIndex, const QString& paramName)
  {
    emit parameterChanged(funcIndex, paramName);
  }

  MOCK_CONST_METHOD1(function, IFunction_const_sptr(QString));
  MOCK_CONST_METHOD0(currentFunctionIndex, boost::optional<QString>());
  MOCK_CONST_METHOD0(peakPicker, IPeakFunction_const_sptr());

  MOCK_METHOD0(initialize, void());
  MOCK_METHOD1(setDataCurve, void(const QwtData&));
  MOCK_METHOD1(setFittedCurve, void(const QwtData&));
  MOCK_METHOD1(setPeakPickerEnabled, void(bool));
  MOCK_METHOD1(setPeakPicker, void(const IPeakFunction_const_sptr&));
  MOCK_METHOD1(setFunction, void(const IFunction_const_sptr&));
  MOCK_METHOD3(setParameter, void(const QString&, const QString&, double));
};

struct FunctionWrapper
{
  FunctionWrapper(IFunction_const_sptr func) : m_func(func) {};

  double operator()(double x, int iSpec)
  {
    UNUSED_ARG(iSpec); // We return the same values for all spectra

    FunctionDomain1DVector domain(x);
    FunctionValues values(domain);

    m_func->function(domain, values);

    assert(values.size() == 1);

    return values.getCalculated(0);
  }

private:
  IFunction_const_sptr m_func;
};

MATCHER_P3(QwtDataX, i, value, delta, "") { return fabs(arg.x(i) - value) < delta; }
MATCHER_P3(QwtDataY, i, value, delta, "") { return fabs(arg.y(i) - value) < delta; }

// DoubleNear matcher was introduced in gmock 1.7 only
MATCHER_P2(DoubleDelta, value, delta, "") { return fabs(arg - value) < delta; }

using namespace MantidQt::CustomInterfaces;

class ALCPeakFittingTest : public CxxTest::TestSuite
{
  MockALCPeakFittingView* m_view;
  ALCPeakFittingPresenter* m_presenter;

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ALCPeakFittingTest *createSuite() { return new ALCPeakFittingTest(); }
  static void destroySuite( ALCPeakFittingTest *suite ) { delete suite; }

  ALCPeakFittingTest()
  {
    API::FrameworkManager::Instance(); // To make sure everything is initialized
  }

  void setUp()
  {
    m_view = new NiceMock<MockALCPeakFittingView>();
    m_presenter = new ALCPeakFittingPresenter(m_view);
    m_presenter->initialize();
  }

  void tearDown()
  {
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_view));
    delete m_presenter;
    delete m_view;
  }

  void test_initialize()
  {
    MockALCPeakFittingView view;
    ALCPeakFittingPresenter presenter(&view);
    EXPECT_CALL(view, initialize()).Times(1);
    presenter.initialize();
  }

  void test_setData()
  {
    MatrixWorkspace_sptr data = WorkspaceCreationHelper::Create2DWorkspace123(1,3);

    EXPECT_CALL(*m_view, setDataCurve(AllOf(Property(&QwtData::size, 3),
                                            QwtDataX(0,1,1E-8), QwtDataX(2,1,1E-8),
                                            QwtDataY(0,2,1E-8), QwtDataY(2,2,1E-8))));

    m_presenter->setData(data);
  }

  IPeakFunction_sptr createGaussian(double centre, double fwhm, double height)
  {
    auto peak = boost::dynamic_pointer_cast<IPeakFunction>(
          API::FunctionFactory::Instance().createFunction("Gaussian"));
    peak->setCentre(centre);
    peak->setFwhm(fwhm);
    peak->setHeight(height);
    return peak;
  }

  void test_fittingOnePeak()
  {
    IFunction_sptr peak = createGaussian(0,1,2);
    FunctionWrapper peakWrapper(peak);

    MatrixWorkspace_const_sptr data =
        WorkspaceCreationHelper::Create2DWorkspaceFromFunction(peakWrapper, 1, -5, 5, 0.5);
    m_presenter->setData(data);

    EXPECT_CALL(*m_view, function(QString(""))).WillRepeatedly(Return(createGaussian(0.2,0.8,1.8)));

    IFunction_const_sptr fittedFunc;

    EXPECT_CALL(*m_view, setFunction(_)).WillOnce(SaveArg<0>(&fittedFunc));

    EXPECT_CALL(*m_view, setFittedCurve(AllOf(Property(&QwtData::size, 21),
                                              QwtDataX(0,-5,1E-8),
                                              QwtDataX(12, 1, 1E-8),
                                              QwtDataX(20,5,1E-8),
                                              QwtDataY(0, peakWrapper(-5,0), 1E-8),
                                              QwtDataY(12, peakWrapper(1,0), 1E-8),
                                              QwtDataY(20, peakWrapper(5,0), 1E-8))));

    m_view->requestFit();

    TS_ASSERT_EQUALS(fittedFunc->name(), "Gaussian");

    auto fittedPeak = boost::dynamic_pointer_cast<const IPeakFunction>(fittedFunc);
    TS_ASSERT(fittedPeak);

    TS_ASSERT_DELTA(fittedPeak->centre(), 0, 1E-6);
    TS_ASSERT_DELTA(fittedPeak->fwhm(), 1, 1E-6);
    TS_ASSERT_DELTA(fittedPeak->height(), 2, 1E-6);
  }

  void test_onCurrentFunctionChanged_nothing()
  {
    ON_CALL(*m_view, currentFunctionIndex()).WillByDefault(Return(boost::none));

    EXPECT_CALL(*m_view, setPeakPickerEnabled(false));

    m_view->changeCurrentFunction();
  }

  void test_onCurrentFunctionChanged_peak()
  {
    ON_CALL(*m_view, currentFunctionIndex()).WillByDefault(Return(boost::optional<QString>("f1")));
    ON_CALL(*m_view, function(QString("f1"))).WillByDefault(Return(createGaussian(1,2,3)));

    EXPECT_CALL(*m_view, setPeakPickerEnabled(true));
    EXPECT_CALL(*m_view, setPeakPicker(Property(&IPeakFunction_const_sptr::get,
                                                AllOf(Property(&IPeakFunction::centre, 1),
                                                      Property(&IPeakFunction::fwhm, 2),
                                                      Property(&IPeakFunction::height, 3)))));

    m_view->changeCurrentFunction();
  }

  void test_onCurrentFunctionChanged_nonPeak()
  {
    ON_CALL(*m_view, currentFunctionIndex()).WillByDefault(Return(boost::optional<QString>("f1")));
    ON_CALL(*m_view, function(QString("f1"))).WillByDefault(
          Return(API::FunctionFactory::Instance().createFunction("LinearBackground")));

    EXPECT_CALL(*m_view, setPeakPickerEnabled(false));

    m_view->changeCurrentFunction();
  }

  void test_onPeakPickerChanged()
  {
    ON_CALL(*m_view, currentFunctionIndex()).WillByDefault(Return(boost::optional<QString>("f1")));
    ON_CALL(*m_view, peakPicker()).WillByDefault(Return(createGaussian(4,5,6)));

    EXPECT_CALL(*m_view, setParameter(QString("f1"), QString("PeakCentre"), 4));
    EXPECT_CALL(*m_view, setParameter(QString("f1"), QString("Sigma"), DoubleDelta(2.123, 1E-3)));
    EXPECT_CALL(*m_view, setParameter(QString("f1"), QString("Height"), 6));

    m_view->changePeakPicker();
  }

  void test_onParameterChanged_peak()
  {
    ON_CALL(*m_view, currentFunctionIndex()).WillByDefault(Return(boost::optional<QString>("f1")));
    ON_CALL(*m_view, function(QString("f1"))).WillByDefault(Return(createGaussian(4,2,6)));
    ON_CALL(*m_view, peakPicker()).WillByDefault(Return(createGaussian(4,5,6)));

    EXPECT_CALL(*m_view, setPeakPicker(Property(&IPeakFunction_const_sptr::get,
                                                AllOf(Property(&IPeakFunction::centre, 4),
                                                      Property(&IPeakFunction::fwhm, 2),
                                                      Property(&IPeakFunction::height, 6)))));

    m_view->changeParameter(QString("f1"), QString("Sigma"));
  }

  // parameterChanged signal is thrown in many scenarios - we want to update the PeakPicker only
  // if it's thrown for currently selected peak function, because that's when PeakPicker is displayed
  void test_onParameterChanged_notACurrentFunction()
  {
    ON_CALL(*m_view, currentFunctionIndex()).WillByDefault(Return(boost::optional<QString>("f2")));

    EXPECT_CALL(*m_view, setPeakPicker(_)).Times(0);

    m_view->changeParameter(QString("f1"), QString("Sigma"));
  }

  void test_onParameterChanged_nonPeak()
  {
    ON_CALL(*m_view, currentFunctionIndex()).WillByDefault(Return(boost::optional<QString>("f1")));
    ON_CALL(*m_view, function(QString("f1"))).WillByDefault(
          Return(API::FunctionFactory::Instance().createFunction("LinearBackground")));

    EXPECT_CALL(*m_view, setPeakPicker(_)).Times(0);

    m_view->changeParameter(QString("f1"), QString("A0"));
  }

};


#endif /* MANTIDQT_CUSTOMINTERFACES_ALCPEAKFITTINGTEST_H_ */
