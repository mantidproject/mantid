#ifndef SLICE_VIEWER_PEAKS_PRESENTER_TEST_H_
#define SLICE_VIEWER_PEAKS_PRESENTER_TEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/IPeak.h"
#include "MantidQtSliceViewer/PeaksPresenter.h"
#include "MantidQtSliceViewer/PeakOverlayViewFactory.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <boost/make_shared.hpp>

using namespace MantidQt::SliceViewer;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace testing;

class PeaksPresenterTest : public CxxTest::TestSuite
{
private:

  /*------------------------------------------------------------
  Mock Peak Overlay View
  ------------------------------------------------------------*/
  class MockPeakOverlayView : public PeakOverlayView
  {
  public:
    MOCK_METHOD1(setPlaneDistance, void(const double&));
    MOCK_CONST_METHOD0(getOrigin, const QPointF&());
    MOCK_CONST_METHOD0(getRadius, double());
    MOCK_METHOD0(updateView, void());
    ~MockPeakOverlayView(){}
  };

  /*------------------------------------------------------------
  Mock Widget Factory.
  ------------------------------------------------------------*/
  class MockPeakOverlayFactory : public PeakOverlayViewFactory
  {
  public:
    MOCK_CONST_METHOD1(createView, boost::shared_ptr<PeakOverlayView>(const Mantid::API::IPeak&));
    MOCK_METHOD0(updateView, void());
  };

public:

  void test_constructor_throws_if_factory_null()
  {
    PeakOverlayViewFactory* nullFactory = NULL;
    Mantid::API::IPeaksWorkspace_sptr peaksWS = WorkspaceCreationHelper::createPeaksWorkspace(1);

    TS_ASSERT_THROWS(ConcretePeaksPresenter(nullFactory, peaksWS), std::invalid_argument);
  }

  void test_construction()
  {
    // Create a widget factory mock
    auto mockViewFactory = new NiceMock<MockPeakOverlayFactory>;

    // Set the expectation on the number of calls
    const int expectedNumberPeaks = 10;
    EXPECT_CALL(*mockViewFactory, createView(_)).Times(expectedNumberPeaks).WillRepeatedly(Return(boost::make_shared<NiceMock<MockPeakOverlayView> >()));;
    Mantid::API::IPeaksWorkspace_sptr peaksWS = WorkspaceCreationHelper::createPeaksWorkspace(expectedNumberPeaks);

    // Construction should cause the widget factory to be used to generate peak overlay objects.
    ConcretePeaksPresenter presenter(mockViewFactory, peaksWS);

    TSM_ASSERT("MockViewFactory not used as expected.", Mock::VerifyAndClearExpectations(mockViewFactory));
  }

  void test_update()
  {
    // Create a widget factory mock
    auto mockViewFactory = new MockPeakOverlayFactory;

    // Set the expectation on the number of calls
    const int expectedNumberPeaks = 10;

    // Create a mock view object that will be returned by the mock factory.
    auto pMockView = new NiceMock<MockPeakOverlayView>;
    EXPECT_CALL(*pMockView, updateView()).Times(expectedNumberPeaks);
    auto mockView = boost::shared_ptr<NiceMock<MockPeakOverlayView> >(pMockView);
    
    EXPECT_CALL(*mockViewFactory, createView(_)).Times(expectedNumberPeaks).WillRepeatedly(Return(mockView));
    Mantid::API::IPeaksWorkspace_sptr peaksWS = WorkspaceCreationHelper::createPeaksWorkspace(expectedNumberPeaks);

    // Construction should cause the widget factory to be used to generate peak overlay objects.
    ConcretePeaksPresenter presenter(mockViewFactory, peaksWS);

    // Updating should cause all of the held views to be updated too.
    presenter.update();
    
    TSM_ASSERT("MockViewFactory not used as expected.", Mock::VerifyAndClearExpectations(mockViewFactory));
    TSM_ASSERT("MockView not used as expected.", Mock::VerifyAndClearExpectations(pMockView));

  }
  
};


#endif
