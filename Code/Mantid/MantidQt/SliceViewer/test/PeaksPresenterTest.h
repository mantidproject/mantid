#ifndef SLICE_VIEWER_PEAKS_PRESENTER_TEST_H_
#define SLICE_VIEWER_PEAKS_PRESENTER_TEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include "MantidAPI/ExperimentInfo.h"
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
  Mock Widget Factory.
  ------------------------------------------------------------*/
  class MockPeakOverlayFactory : public PeakOverlayViewFactory
  {
  public:
    MOCK_CONST_METHOD2(createView, PeakOverlayView*(const QPointF&, const QPointF&));
  };

public:

  void test_constructor_throws_if_factory_null()
  {
    PeakOverlayViewFactory* nullFactory = NULL;
    Mantid::API::IPeaksWorkspace_sptr peaksWS = WorkspaceCreationHelper::createPeaksWorkspace(1);

    TS_ASSERT_THROWS(PeaksPresenter(nullFactory, peaksWS), std::invalid_argument);
  }

  void test_construction()
  {
    // Create a widget factory mock
    auto peakWidgetFactory = new MockPeakOverlayFactory;

    // Set the expectation on the number of calls
    const int expectedNumberPeaks = 10;
    EXPECT_CALL(*peakWidgetFactory, createView(_, _)).Times(expectedNumberPeaks);
    Mantid::API::IPeaksWorkspace_sptr peaksWS = WorkspaceCreationHelper::createPeaksWorkspace(expectedNumberPeaks);

    // Construction should cause the widget factory to be used to generate peak overlay objects.
    PeaksPresenter presenter(peakWidgetFactory, peaksWS);

    TSM_ASSERT("PeakWidgetFactory not used as expected.", Mock::VerifyAndClearExpectations(peakWidgetFactory));
  }
  
};


#endif
