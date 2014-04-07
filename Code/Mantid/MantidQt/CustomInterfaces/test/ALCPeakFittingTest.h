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
  void requestFit() { emit fit(); }

  MOCK_CONST_METHOD0(peaks, ListOfPeaks());
  MOCK_METHOD0(initialize, void());
  MOCK_METHOD1(setData, void(MatrixWorkspace_const_sptr));
  MOCK_METHOD1(setPeaks, void(const ListOfPeaks&));
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
    m_view = new MockALCPeakFittingView();
    m_presenter = new ALCPeakFittingPresenter(m_view);

    EXPECT_CALL(*m_view, initialize()).Times(1);

    m_presenter->initialize();
  }

  void tearDown()
  {
    delete m_presenter;
    delete m_view;
  }

  void setData(MatrixWorkspace_const_sptr data)
  {
    EXPECT_CALL(*m_view, setData(_)).Times(1);
    m_presenter->setData(data);
  }

  void test_fittingOnePeak()
  {
    auto peak = boost::dynamic_pointer_cast<IPeakFunction>(
          API::FunctionFactory::Instance().createFunction("Gaussian"));
    peak->setCentre(0); peak->setFwhm(1); peak->setHeight(2);
    FunctionWrapper peakWrapper(peak);

    MatrixWorkspace_const_sptr data =
        WorkspaceCreationHelper::Create2DWorkspaceFromFunction(peakWrapper, 1, -5, 5, 0.1);
    setData(data);

    auto peakToFit = boost::dynamic_pointer_cast<IPeakFunction>(
          API::FunctionFactory::Instance().createFunction("Gaussian"));
    peakToFit->setCentre(0.2); peakToFit->setFwhm(0.8); peakToFit->setHeight(1.8);

    IALCPeakFittingView::ListOfPeaks peaks;
    peaks.push_back(peakToFit);

    EXPECT_CALL(*m_view, peaks()).WillRepeatedly(Return(peaks));

    IALCPeakFittingView::ListOfPeaks fittedPeaks;

    EXPECT_CALL(*m_view, setData(_)).Times(0); // Data shouldn't be changed after fit
    EXPECT_CALL(*m_view, setPeaks(_)).Times(1).WillOnce(SaveArg<0>(&fittedPeaks));

    m_view->requestFit();

    TS_ASSERT_EQUALS(fittedPeaks.size(), 1);

    IALCPeakFittingView::Peak fittedPeak = fittedPeaks[0];

    TS_ASSERT(fittedPeak);
    TS_ASSERT_EQUALS(fittedPeak->name(), "Gaussian");
    TS_ASSERT_DELTA(fittedPeak->centre(), 0, 1E-8);
    TS_ASSERT_DELTA(fittedPeak->fwhm(), 1, 1E-8);
    TS_ASSERT_DELTA(fittedPeak->height(), 2, 1E-8);
  }

};


#endif /* MANTIDQT_CUSTOMINTERFACES_ALCPEAKFITTINGTEST_H_ */
