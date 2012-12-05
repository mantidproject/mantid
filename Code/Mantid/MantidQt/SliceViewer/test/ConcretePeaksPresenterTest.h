#ifndef SLICE_VIEWER_PEAKS_PRESENTER_TEST_H_
#define SLICE_VIEWER_PEAKS_PRESENTER_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/IPeak.h"
#include "MantidQtSliceViewer/ConcretePeaksPresenter.h"
#include "MantidQtSliceViewer/PeakOverlayViewFactory.h"
#include "MantidQtSliceViewer/PeakTransformFactory.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MockObjects.h"
#include <boost/make_shared.hpp>
#include <string>

using namespace MantidQt::SliceViewer;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace testing;
using boost::regex;

class ConcretePeaksPresenterTest : public CxxTest::TestSuite
{

  /// Helper method to create a good 'Integrated' peaks workspace
  Mantid::API::IPeaksWorkspace_sptr createPeaksWorkspace(const int nPeaks, const double radius=1)
  {
    Mantid::API::IPeaksWorkspace_sptr peaksWS = WorkspaceCreationHelper::createPeaksWorkspace(nPeaks);
    peaksWS->mutableRun().addProperty("PeaksIntegrated", true);
    peaksWS->mutableRun().addProperty("PeakRadius", radius);
    return peaksWS;
  }

public:

  void test_constructor_throws_if_view_factory_null()
  {
    PeakOverlayViewFactory* nullViewFactory = NULL; // View factory is null
    PeakOverlayViewFactory_sptr viewFactory(nullViewFactory);
    IPeaksWorkspace_sptr peaksWS = createPeaksWorkspace(1);
  
    PeakTransform_sptr mockTransform(new MockPeakTransform);  
 
    auto pMockTransformFactory = new NiceMock<MockPeakTransformFactory>;
    PeakTransformFactory_sptr peakTransformFactory(pMockTransformFactory);
    EXPECT_CALL(*pMockTransformFactory, createDefaultTransform()).WillRepeatedly(Return(mockTransform));

    TS_ASSERT_THROWS(ConcretePeaksPresenter(viewFactory, peaksWS, peakTransformFactory), std::invalid_argument);
  }

  void test_constructor_throws_if_peaks_workspace_not_integrated()
  {
    auto mockViewFactory = new NiceMock<MockPeakOverlayFactory>;
    PeakOverlayViewFactory_sptr viewFactory(mockViewFactory);

    IPeaksWorkspace_sptr peaksWS = WorkspaceCreationHelper::createPeaksWorkspace(1); // Peaks workspace is not integrated.
  
    PeakTransform_sptr mockTransform(new MockPeakTransform);  
 
    auto pMockTransformFactory = new NiceMock<MockPeakTransformFactory>;
    PeakTransformFactory_sptr peakTransformFactory(pMockTransformFactory);
    EXPECT_CALL(*pMockTransformFactory, createDefaultTransform()).WillRepeatedly(Return(mockTransform));

    TS_ASSERT_THROWS(ConcretePeaksPresenter(viewFactory, peaksWS, peakTransformFactory), std::invalid_argument);
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

    // Create an input workspace.
    Mantid::API::IPeaksWorkspace_sptr peaksWS = createPeaksWorkspace(expectedNumberPeaks);

    // Create a mock transform object.
    auto pMockTransform = new NiceMock<MockPeakTransform>;
    PeakTransform_sptr mockTransform(pMockTransform);
    const int numberOfTimesCalled = static_cast<int>(peaksWS->rowCount());// Should be called for every peak i.e. row.
    EXPECT_CALL(*pMockTransform, transformPeak(_)).Times(numberOfTimesCalled).WillRepeatedly(Return(V3D()));

    // Create a mock transform factory.
    auto pMockTransformFactory = new NiceMock<MockPeakTransformFactory>;
    PeakTransformFactory_sptr peakTransformFactory(pMockTransformFactory);
    EXPECT_CALL(*pMockTransformFactory, createDefaultTransform()).WillOnce(Return(mockTransform));
    EXPECT_CALL(*pMockTransformFactory, createTransform(_,_)).WillOnce(Return(mockTransform));

    // Construction should cause the widget factory to be used to generate peak overlay objects.
    ConcretePeaksPresenter presenter(PeakOverlayViewFactory_sptr(mockViewFactory), peaksWS, peakTransformFactory);

    TSM_ASSERT("MockViewFactory not used as expected.", Mock::VerifyAndClearExpectations(mockViewFactory));
    TSM_ASSERT("MockView not used as expected.", Mock::VerifyAndClearExpectations(pMockView));
    TSM_ASSERT("MockTransformFactory not used as expected", Mock::VerifyAndClearExpectations(pMockTransformFactory));
    TSM_ASSERT("MockTransform not used as expected", Mock::VerifyAndClearExpectations(pMockTransform));
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

    // Create a mock transform object.
    auto pMockTransform = new NiceMock<MockPeakTransform>;
    PeakTransform_sptr mockTransform(pMockTransform);
    EXPECT_CALL(*pMockTransform, transformPeak(_)).WillRepeatedly(Return(V3D()));

    // Create a mock transform factory.
    auto pMockTransformFactory = new NiceMock<MockPeakTransformFactory>;
    PeakTransformFactory_sptr peakTransformFactory(pMockTransformFactory);
    EXPECT_CALL(*pMockTransformFactory, createDefaultTransform()).WillOnce(Return(mockTransform));
    EXPECT_CALL(*pMockTransformFactory, createTransform(_,_)).WillOnce(Return(mockTransform));

    // Construction should cause the widget factory to be used to generate peak overlay objects.
    ConcretePeaksPresenter presenter(PeakOverlayViewFactory_sptr(mockViewFactory), peaksWS, peakTransformFactory);

    // Updating should cause all of the held views to be updated too.
    presenter.update();
    
    TSM_ASSERT("MockView not used as expected.", Mock::VerifyAndClearExpectations(pMockView));
    TSM_ASSERT("MockTransformFactory not used as expected", Mock::VerifyAndClearExpectations(pMockTransformFactory));
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

    // Create a mock transform object.
    auto pMockTransform = new NiceMock<MockPeakTransform>;
    PeakTransform_sptr mockTransform(pMockTransform);
    EXPECT_CALL(*pMockTransform, transformPeak(_)).WillRepeatedly(Return(V3D()));

    // Create a mock transform factory.
    auto pMockTransformFactory = new NiceMock<MockPeakTransformFactory>;
    PeakTransformFactory_sptr peakTransformFactory(pMockTransformFactory);
    EXPECT_CALL(*pMockTransformFactory, createDefaultTransform()).WillOnce(Return(mockTransform));
    EXPECT_CALL(*pMockTransformFactory, createTransform(_,_)).WillOnce(Return(mockTransform));

    // Construction should cause the widget factory to be used to generate peak overlay objects.
    ConcretePeaksPresenter presenter(PeakOverlayViewFactory_sptr(mockViewFactory), peaksWS, peakTransformFactory);

    // Updating should cause all of the held views to be updated too.
    presenter.updateWithSlicePoint(slicePoint);

    TSM_ASSERT("MockView not used as expected.", Mock::VerifyAndClearExpectations(pMockView));
    TSM_ASSERT("MockTransformFactory not used as expected", Mock::VerifyAndClearExpectations(pMockTransformFactory));
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

     // Create a mock transform object.
    auto pMockTransform = new NiceMock<MockPeakTransform>;
    PeakTransform_sptr mockTransform(pMockTransform);
    EXPECT_CALL(*pMockTransform, transformPeak(_)).WillRepeatedly(Return(V3D()));

    // Create a mock transform factory.
    auto pMockTransformFactory = new NiceMock<MockPeakTransformFactory>;
    PeakTransformFactory_sptr peakTransformFactory(pMockTransformFactory);
    EXPECT_CALL(*pMockTransformFactory, createDefaultTransform()).WillOnce(Return(mockTransform));
    EXPECT_CALL(*pMockTransformFactory, createTransform(_,_)).WillOnce(Return(mockTransform));

    {
      ConcretePeaksPresenter presenter(PeakOverlayViewFactory_sptr(mockViewFactory), peaksWS, peakTransformFactory);
    } // Guaranteed destruction at this point. Destructor should trigger hide on all owned views.

    TSM_ASSERT("MockView not used as expected.", Mock::VerifyAndClearExpectations(pMockView));
    TSM_ASSERT("MockTransformFactory not used as expected", Mock::VerifyAndClearExpectations(pMockTransformFactory));
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

     // Create a mock transform object.
    auto pMockTransform = new NiceMock<MockPeakTransform>;
    PeakTransform_sptr mockTransform(pMockTransform);
    EXPECT_CALL(*pMockTransform, transformPeak(_)).WillRepeatedly(Return(V3D()));

    // Create a mock transform factory.
    auto pMockTransformFactory = new NiceMock<MockPeakTransformFactory>;
    PeakTransformFactory_sptr peakTransformFactory(pMockTransformFactory);
    EXPECT_CALL(*pMockTransformFactory, createDefaultTransform()).WillOnce(Return(mockTransform));
    EXPECT_CALL(*pMockTransformFactory, createTransform(_,_)).WillRepeatedly(Throw(PeakTransformException())); // The actual transform will throw if a mix of Qx and Qy were used.

    ConcretePeaksPresenter presenter(PeakOverlayViewFactory_sptr(mockViewFactory), peaksWS, peakTransformFactory);
    TSM_ASSERT("MockView not used as expected.", Mock::VerifyAndClearExpectations(pMockView));
    TSM_ASSERT("MockTransformFactory not used as expected", Mock::VerifyAndClearExpectations(pMockTransformFactory));
  }
};


#endif
