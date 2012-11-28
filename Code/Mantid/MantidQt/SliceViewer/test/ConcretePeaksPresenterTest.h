#ifndef SLICE_VIEWER_PEAKS_PRESENTER_TEST_H_
#define SLICE_VIEWER_PEAKS_PRESENTER_TEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/IPeak.h"
#include "MantidQtSliceViewer/ConcretePeaksPresenter.h"
#include "MantidQtSliceViewer/PeakOverlayViewFactory.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <boost/make_shared.hpp>

using namespace MantidQt::SliceViewer;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace testing;

class ConcretePeaksPresenterTest : public CxxTest::TestSuite
{
private:

  /*------------------------------------------------------------
  Mock Peak Overlay View
  ------------------------------------------------------------*/
  class MockPeakOverlayView : public PeakOverlayView
  {
  public:
    MOCK_METHOD1(setPlaneDistance, void(const double&));
    MOCK_METHOD0(updateView, void());
    MOCK_METHOD1(setSlicePoint, void(const double&));
    MOCK_METHOD0(hideView, void());
    MOCK_METHOD0(showView, void());
    MOCK_METHOD1(movePosition, void(const PeakTransform&));
    ~MockPeakOverlayView(){}
  };

  /*------------------------------------------------------------
  Mock Widget Factory.
  ------------------------------------------------------------*/
  class MockPeakOverlayFactory : public PeakOverlayViewFactory
  {
  public:
    MOCK_CONST_METHOD1(createView, boost::shared_ptr<PeakOverlayView>(const Mantid::Kernel::V3D&));
    MOCK_METHOD1(setRadius, void(const double&));
    MOCK_CONST_METHOD0(getPlotXLabel, std::string());
    MOCK_CONST_METHOD0(getPlotYLabel, std::string());
    MOCK_METHOD0(updateView, void());
  };

  /// Helper method to create a good 'Integrated' peaks workspace
  Mantid::API::IPeaksWorkspace_sptr createPeaksWorkspace(const int nPeaks, const double radius=1)
  {
    Mantid::API::IPeaksWorkspace_sptr peaksWS = WorkspaceCreationHelper::createPeaksWorkspace(nPeaks);
    peaksWS->mutableRun().addProperty("PeaksIntegrated", true);
    peaksWS->mutableRun().addProperty("PeakRadius", radius);
    return peaksWS;
  }

public:

  void test_constructor_throws_if_factory_null()
  {
    PeakOverlayViewFactory* nullFactory = NULL;
    Mantid::API::IPeaksWorkspace_sptr peaksWS = WorkspaceCreationHelper::createPeaksWorkspace(1);

    TS_ASSERT_THROWS(ConcretePeaksPresenter(nullFactory, peaksWS), std::invalid_argument);
  }

  void test_constructor_throws_if_peaks_workspace_null()
  {
  }

  void test_constructor_throws_if_peaks_workspace_not_integrated()
  {
  }

  void test_construction()
  {
    // Create a widget factory mock
    auto mockViewFactory = new NiceMock<MockPeakOverlayFactory>;

    // Set the expectation on the number of calls
    const int expectedNumberPeaks = 10;
    
    // Create a mock view object that will be returned by the mock factory.
    auto pMockView = new NiceMock<MockPeakOverlayView>;
    auto mockView = boost::shared_ptr<NiceMock<MockPeakOverlayView> >(pMockView);

    EXPECT_CALL(*mockViewFactory, setRadius(_)).Times(1);
    EXPECT_CALL(*mockViewFactory, createView(_)).Times(expectedNumberPeaks).WillRepeatedly(Return(mockView));
    EXPECT_CALL(*mockViewFactory, getPlotXLabel()).WillOnce(Return("H"));
    EXPECT_CALL(*mockViewFactory, getPlotYLabel()).WillOnce(Return("K"));

    Mantid::API::IPeaksWorkspace_sptr peaksWS = createPeaksWorkspace(expectedNumberPeaks);

    // Construction should cause the widget factory to be used to generate peak overlay objects.
    ConcretePeaksPresenter presenter(mockViewFactory, peaksWS);

    TSM_ASSERT("MockViewFactory not used as expected.", Mock::VerifyAndClearExpectations(mockViewFactory));
    TSM_ASSERT("MockView not used as expected.", Mock::VerifyAndClearExpectations(pMockView));
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
    
    EXPECT_CALL(*mockViewFactory, setRadius(_)).Times(1);
    EXPECT_CALL(*mockViewFactory, createView(_)).WillRepeatedly(Return(mockView));
    EXPECT_CALL(*mockViewFactory, getPlotXLabel()).WillOnce(Return("H"));
    EXPECT_CALL(*mockViewFactory, getPlotYLabel()).WillOnce(Return("K"));
    Mantid::API::IPeaksWorkspace_sptr peaksWS = createPeaksWorkspace(expectedNumberPeaks);

    // Construction should cause the widget factory to be used to generate peak overlay objects.
    ConcretePeaksPresenter presenter(mockViewFactory, peaksWS);

    // Updating should cause all of the held views to be updated too.
    presenter.update();
    
    TSM_ASSERT("MockView not used as expected.", Mock::VerifyAndClearExpectations(pMockView));
  }

  void test_set_slice_point()
  {
    // Create a widget factory mock
    auto mockViewFactory = new MockPeakOverlayFactory;

    const double slicePoint = 0.1;
    const int expectedNumberPeaks = 10;

    // Create a mock view object that will be returned by the mock factory.
    auto pMockView = new NiceMock<MockPeakOverlayView>;
    EXPECT_CALL(*pMockView, setSlicePoint(slicePoint)).Times(expectedNumberPeaks);
    auto mockView = boost::shared_ptr<NiceMock<MockPeakOverlayView> >(pMockView);

    EXPECT_CALL(*mockViewFactory, setRadius(_)).Times(1);
    EXPECT_CALL(*mockViewFactory, createView(_)).WillRepeatedly(Return(mockView));
    EXPECT_CALL(*mockViewFactory, getPlotXLabel()).WillOnce(Return("H"));
    EXPECT_CALL(*mockViewFactory, getPlotYLabel()).WillOnce(Return("K"));
    Mantid::API::IPeaksWorkspace_sptr peaksWS = createPeaksWorkspace(expectedNumberPeaks);

    // Construction should cause the widget factory to be used to generate peak overlay objects.
    ConcretePeaksPresenter presenter(mockViewFactory, peaksWS);

    // Updating should cause all of the held views to be updated too.
    presenter.updateWithSlicePoint(slicePoint);

    TSM_ASSERT("MockView not used as expected.", Mock::VerifyAndClearExpectations(pMockView));
  }

  void test_hide_owned_views_on_death()
  {
    // Create a widget factory mock
    auto mockViewFactory = new MockPeakOverlayFactory;

    const int expectedNumberPeaks = 1;

    // Create a mock view object that will be returned by the mock factory.
    auto pMockView = new NiceMock<MockPeakOverlayView>;
    EXPECT_CALL(*pMockView, hideView()).Times(expectedNumberPeaks);
    auto mockView = boost::shared_ptr<NiceMock<MockPeakOverlayView> >(pMockView);

    EXPECT_CALL(*mockViewFactory, setRadius(_)).Times(1);
    EXPECT_CALL(*mockViewFactory, createView(_)).WillRepeatedly(Return(mockView));
    EXPECT_CALL(*mockViewFactory, getPlotXLabel()).WillOnce(Return("H"));
    EXPECT_CALL(*mockViewFactory, getPlotYLabel()).WillOnce(Return("K"));
    Mantid::API::IPeaksWorkspace_sptr peaksWS = createPeaksWorkspace(expectedNumberPeaks);

    {
      ConcretePeaksPresenter presenter(mockViewFactory, peaksWS);
    } // Guaranteed destruction at this point. Destructor should trigger hide on all owned views.

    TSM_ASSERT("MockView not used as expected.", Mock::VerifyAndClearExpectations(pMockView));
  }

  void test_handle_non_hkl_xy_mappings()
  {
    // Create a widget factory mock
    auto mockViewFactory = new MockPeakOverlayFactory;

    const int expectedNumberPeaks = 1;

    // Create a mock view object that will be returned by the mock factory.
    auto pMockView = new NiceMock<MockPeakOverlayView>;
    EXPECT_CALL(*pMockView, hideView()).Times(expectedNumberPeaks); // This will be called automatically because the presenter won't be able to map Qx (below).
    auto mockView = boost::shared_ptr<NiceMock<MockPeakOverlayView> >(pMockView);

    EXPECT_CALL(*mockViewFactory, setRadius(_)).Times(1);
    EXPECT_CALL(*mockViewFactory, createView(_)).WillRepeatedly(Return(mockView));
    EXPECT_CALL(*mockViewFactory, getPlotXLabel()).WillOnce(Return("Qx")); // Not either H, K or L
    EXPECT_CALL(*mockViewFactory, getPlotYLabel()).WillOnce(Return("K"));
    Mantid::API::IPeaksWorkspace_sptr peaksWS = createPeaksWorkspace(expectedNumberPeaks);

    ConcretePeaksPresenter presenter(mockViewFactory, peaksWS);
    TSM_ASSERT("MockView not used as expected.", Mock::VerifyAndClearExpectations(pMockView));
  }


  
};


#endif
