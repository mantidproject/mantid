#ifndef SLICE_VIEWER_PEAKS_PRESENTER_TEST_H_
#define SLICE_VIEWER_PEAKS_PRESENTER_TEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/IPeak.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidQtSliceViewer/ConcretePeaksPresenter.h"
#include "MantidQtSliceViewer/PeakOverlayViewFactory.h"
#include "MantidQtSliceViewer/PeakTransformFactory.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MockObjects.h"
#include <boost/make_shared.hpp>
#include <string>
#include <algorithm>

using namespace MantidQt::SliceViewer;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace testing;
using boost::regex;

// Alias.
typedef boost::shared_ptr<Mantid::API::MDGeometry> MDGeometry_sptr;

class ConcretePeaksPresenterTest : public CxxTest::TestSuite
{
  /**
   * Helper method.
   * Determine whether a vector is sorted Ascending
   * @param potentiallySorted : Vector that might be sorted ascending.
   * @return False if not sortedAscending
   */
  template<typename T>
  bool isSortedAscending(std::vector<T> potentiallySorted)
  {
    return std::adjacent_find(potentiallySorted.begin(), potentiallySorted.end(), std::greater<T>()) == potentiallySorted.end();
  }

  /**
   * Helper method.
   * Determine whether a vector is sorted Descending
   * @param potentiallySorted : Vector that might be sorted descending.
   * @return False if not sortedAscending
   */
  template<typename T>
  bool isSortedDescending(std::vector<T> potentiallySorted)
  {
    return std::adjacent_find(potentiallySorted.begin(), potentiallySorted.end(), std::less<T>()) == potentiallySorted.end();
  }

  /// Alias.
  typedef boost::shared_ptr<MantidQt::SliceViewer::ConcretePeaksPresenter> ConcretePeaksPresenter_sptr;

  /// Helper method to create a good 'Integrated' peaks workspace
  Mantid::API::IPeaksWorkspace_sptr createPeaksWorkspace(const int nPeaks, const double radius=1)
  {
    Mantid::API::IPeaksWorkspace_sptr peaksWS = WorkspaceCreationHelper::createPeaksWorkspace(nPeaks);
    peaksWS->mutableRun().addProperty("PeaksIntegrated", true);
    peaksWS->mutableRun().addProperty("PeakRadius", radius);
    peaksWS->mutableRun().addProperty("BackgroundInnerRadius", radius+1);
    peaksWS->mutableRun().addProperty("BackgroundOuterRadius", radius+2);
    return peaksWS;
  }

  /// Helper method to create a mock MDDimension.
  IMDDimension_sptr createExpectedMDDimension(const std::string returnLabel)
  {
    auto* pDim = new NiceMock<MockIMDDimension>;
    IMDDimension_sptr dim(pDim);
    EXPECT_CALL(*pDim, getName()).WillRepeatedly(Return(returnLabel));
    return dim;
  }

  /// Helper method to create an expected MDGeometry (we call it MDWorkspace here).
  MDGeometry_sptr createExpectedMDWorkspace()
  {
    // Create a mock H Dim
    IMDDimension_sptr HDim = createExpectedMDDimension("H");
    // Create a mock K Dim
    IMDDimension_sptr KDim = createExpectedMDDimension("K");
    // Create a mock L Dim
    IMDDimension_sptr LDim = createExpectedMDDimension("L");

    // Create the mock MD geometry
    MockMDGeometry* pGeometry = new MockMDGeometry;
    EXPECT_CALL(*pGeometry, getNumDims()).WillRepeatedly(Return(3));
    EXPECT_CALL(*pGeometry, getDimension(0)).WillRepeatedly(Return(HDim));
    EXPECT_CALL(*pGeometry, getDimension(1)).WillRepeatedly(Return(KDim));
    EXPECT_CALL(*pGeometry, getDimension(2)).WillRepeatedly(Return(LDim));

    return boost::shared_ptr<MockMDGeometry>(pGeometry);
  }

  /** Make the tests easier to write and understand by utilising
  a builder. This means that we can create a standard product in one line of test code, but explicilty override 
  constructor inputs as the test requires.
  */
  class ConcretePeaksPresenterBuilder
  {
  private:
    PeakOverlayViewFactory_sptr m_nonIntegratedViewFactory;
    PeakOverlayViewFactory_sptr m_integratedViewFactory; 
    IPeaksWorkspace_sptr m_peaksWS; 
    boost::shared_ptr<MDGeometry> m_mdWS; 
    PeakTransformFactory_sptr m_transformFactory;
    
  public:

    ConcretePeaksPresenterBuilder()
    {
    }

    ConcretePeaksPresenterBuilder(const ConcretePeaksPresenterBuilder& other)
    {
      m_nonIntegratedViewFactory = other.m_nonIntegratedViewFactory;
      m_integratedViewFactory = other.m_integratedViewFactory; 
      m_peaksWS = other.m_peaksWS;
      m_mdWS = other.m_mdWS;
      m_transformFactory = other.m_transformFactory;
    }

    void withNonIntegratedViewFactory(PeakOverlayViewFactory_sptr val)
    {
      m_nonIntegratedViewFactory = val;
    }
    void withIntegratedViewFactory(PeakOverlayViewFactory_sptr val)
    {
      m_integratedViewFactory = val;
    }
    void withPeaksWorkspace(IPeaksWorkspace_sptr val)
    {
      m_peaksWS = val;
    }
    void withMDWorkspace(boost::shared_ptr<MDGeometry> val)
    {
      m_mdWS = val;
    }
    void withTransformFactory(PeakTransformFactory_sptr val)
    {
      m_transformFactory = val;
    }

    ConcretePeaksPresenter_sptr create()
    {
      return boost::make_shared<ConcretePeaksPresenter>(m_nonIntegratedViewFactory, m_integratedViewFactory, m_peaksWS, m_mdWS, m_transformFactory);
    }

  };

  /**
  Helper method that will produce a customisable object (builder) for making ConcretePeaks presenter.
  1) All constructor parameters can be overriden using methods with....() on the returned builder object
  2) The default builder has been set up to create a ubiquitious ConcretePeaksPresenter product.
  */
  ConcretePeaksPresenterBuilder createStandardBuild(const int expectedNumberPeaks=5)
  {
    // Create a mock view object that will be returned by the mock factory.
    auto mockView = boost::shared_ptr<NiceMock<MockPeakOverlayView> >(new NiceMock<MockPeakOverlayView>);
    
    // Create a widget factory mock
    auto pMockViewFactory = new MockPeakOverlayFactory;
    
    PeakOverlayViewFactory_sptr mockViewFactory = PeakOverlayViewFactory_sptr(pMockViewFactory);
    EXPECT_CALL(*pMockViewFactory, createView(_)).WillRepeatedly(Return(mockView));
    EXPECT_CALL(*pMockViewFactory, getPlotXLabel()).WillRepeatedly(Return("H"));
    EXPECT_CALL(*pMockViewFactory, getPlotYLabel()).WillRepeatedly(Return("K"));
    EXPECT_CALL(*pMockViewFactory, setPeakRadius(_,_,_)).Times(AtLeast(0));
    EXPECT_CALL(*pMockViewFactory, setZRange(_,_)).Times(AtLeast(0));

    // Create an input MODEL Peaks workspace (INTEGRATED)
    Mantid::API::IPeaksWorkspace_sptr peaksWS = createPeaksWorkspace(expectedNumberPeaks);
    // Create an input MODEL IMDWorkspace (Geom)
    MDGeometry_sptr mdWS = createExpectedMDWorkspace();

    // Create a mock transform object.
    auto pMockTransform = new NiceMock<MockPeakTransform>;
    PeakTransform_sptr mockTransform(pMockTransform);
    EXPECT_CALL(*pMockTransform, transformPeak(_)).WillRepeatedly(Return(V3D()));

    // Create a mock transform factory.
    auto pMockTransformFactory = new NiceMock<MockPeakTransformFactory>;
    PeakTransformFactory_sptr peakTransformFactory(pMockTransformFactory);
    EXPECT_CALL(*pMockTransformFactory, createDefaultTransform()).WillOnce(Return(mockTransform));
    EXPECT_CALL(*pMockTransformFactory, createTransform(_,_)).WillOnce(Return(mockTransform));

    // Create and return a configurable builder.
    ConcretePeaksPresenterBuilder builder;
    builder.withNonIntegratedViewFactory(mockViewFactory);
    builder.withIntegratedViewFactory(mockViewFactory);
    builder.withPeaksWorkspace(peaksWS);
    builder.withMDWorkspace(mdWS);
    builder.withTransformFactory(peakTransformFactory);
    return builder;
  }

public:

  void test_constructor_throws_if_either_view_factory_null()
  {
    // Create a Null view Factory
    PeakOverlayViewFactory* pNullViewFactory = NULL; // View factory is null
    PeakOverlayViewFactory_sptr nullViewFactory(pNullViewFactory);

    // Create a view factory
    PeakOverlayViewFactory* pNormalViewFactory = new MockPeakOverlayFactory;
    PeakOverlayViewFactory_sptr normalViewFactory(pNormalViewFactory);

    // Create a peaks workspace
    IPeaksWorkspace_sptr peaksWS = createPeaksWorkspace(1);
  
    // Create a transform factory and product.
    PeakTransform_sptr mockTransform(new MockPeakTransform);  
    auto pMockTransformFactory = new NiceMock<MockPeakTransformFactory>;
    PeakTransformFactory_sptr peakTransformFactory(pMockTransformFactory);
    EXPECT_CALL(*pMockTransformFactory, createDefaultTransform()).WillRepeatedly(Return(mockTransform));

    // Create a md workspace
    MDGeometry_sptr mdWS = boost::make_shared<MockMDGeometry>();

    // Use builder to setup construction.
    ConcretePeaksPresenterBuilder builder;
    builder.withPeaksWorkspace(peaksWS);
    builder.withMDWorkspace(mdWS);
    builder.withTransformFactory(peakTransformFactory);

    builder.withNonIntegratedViewFactory(nullViewFactory);
    builder.withIntegratedViewFactory(normalViewFactory);
    TSM_ASSERT_THROWS("Non integrated view factory is null, should throw.", builder.create(), std::invalid_argument);

    builder.withNonIntegratedViewFactory(normalViewFactory);
    builder.withIntegratedViewFactory(nullViewFactory);
    TSM_ASSERT_THROWS("Integrated view factory is null, should throw.",builder.create(), std::invalid_argument);
  }

  void test_construction()
  {
    // Expected number of peaks to create
    const size_t expectedNumberPeaks = 3;

    // Peaks workspace IS INTEGRATED.
    IPeaksWorkspace_sptr peaksWS = createPeaksWorkspace(expectedNumberPeaks);

    // Create an MDWorkspace
    MDGeometry_sptr mdWS = createExpectedMDWorkspace();

    // Mock View Factory Product
    auto pMockView = new NiceMock<MockPeakOverlayView>;
    auto mockView = boost::shared_ptr<NiceMock<MockPeakOverlayView> >(pMockView);

    // Mock View Factory for NON-integrated peaks. We expect that this will be used.
    auto pMockNonIntegratedViewFactory = new MockPeakOverlayFactory;
    PeakOverlayViewFactory_sptr mockNonIntegratedViewFactory(pMockNonIntegratedViewFactory);

    // Mock View Factory for integrated peaks. We expect that this will never be used.
    auto pMockIntegratedViewFactory = new MockPeakOverlayFactory;
    PeakOverlayViewFactory_sptr mockIntegratedViewFactory(pMockIntegratedViewFactory);
    EXPECT_CALL(*pMockIntegratedViewFactory, setPeakRadius(_,_,_)).Times(1);
    EXPECT_CALL(*pMockIntegratedViewFactory, setZRange(_,_)).Times(1);
    EXPECT_CALL(*pMockIntegratedViewFactory, createView(_)).Times(expectedNumberPeaks).WillRepeatedly(Return(mockView));
    EXPECT_CALL(*pMockIntegratedViewFactory, getPlotXLabel()).WillOnce(Return("H"));
    EXPECT_CALL(*pMockIntegratedViewFactory, getPlotYLabel()).WillOnce(Return("K"));

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

    // Construct the presenter.
    ConcretePeaksPresenter presenter(mockNonIntegratedViewFactory, mockIntegratedViewFactory, peaksWS, mdWS, peakTransformFactory);

    TSM_ASSERT("Non-Integrated View Factory has not been used as expected", Mock::VerifyAndClearExpectations(pMockNonIntegratedViewFactory));
    TSM_ASSERT("Integrated View Factory has not been used as expected", Mock::VerifyAndClearExpectations(pMockIntegratedViewFactory));
    TSM_ASSERT("MockView not used as expected.", Mock::VerifyAndClearExpectations(pMockView));
    TSM_ASSERT("MockTransformFactory not used as expected", Mock::VerifyAndClearExpectations(pMockTransformFactory));
    TSM_ASSERT("MockTransform not used as expected", Mock::VerifyAndClearExpectations(pMockTransform));

    auto ownedPeaksWorkspace = presenter.presentedWorkspaces();
    TS_ASSERT_EQUALS(1, ownedPeaksWorkspace.size());
  }

  void test_constructor_swaps_view_factory_if_peaks_workspace_not_integrated()
  {
    // Expected number of peaks to create
    const size_t expectedNumberPeaks = 3;

    // Peaks workspace IS NOT INTEGRATED.
    IPeaksWorkspace_sptr peaksWS = WorkspaceCreationHelper::createPeaksWorkspace(expectedNumberPeaks); 

    // Create an MDWorkspace
    MDGeometry_sptr mdWS = createExpectedMDWorkspace();

    // Mock View Factory Product
    auto pMockView = new NiceMock<MockPeakOverlayView>;
    auto mockView = boost::shared_ptr<NiceMock<MockPeakOverlayView> >(pMockView);

    // Mock View Factory for NON-INTEGRATED peaks. We expect that this will be used.
    auto pMockNonIntegratedViewFactory = new MockPeakOverlayFactory;
    PeakOverlayViewFactory_sptr mockNonIntegratedViewFactory(pMockNonIntegratedViewFactory);
    EXPECT_CALL(*pMockNonIntegratedViewFactory, setPeakRadius(_,_,_)).Times(1);
    EXPECT_CALL(*pMockNonIntegratedViewFactory, setZRange(_,_)).Times(1);
    EXPECT_CALL(*pMockNonIntegratedViewFactory, createView(_)).Times(expectedNumberPeaks).WillRepeatedly(Return(mockView));
    EXPECT_CALL(*pMockNonIntegratedViewFactory, getPlotXLabel()).WillOnce(Return("H"));
    EXPECT_CALL(*pMockNonIntegratedViewFactory, getPlotYLabel()).WillOnce(Return("K"));

    // Mock View Factory for integrated peaks. We expect that this will never be used.
    auto pMockIntegratedViewFactory = new MockPeakOverlayFactory;
    PeakOverlayViewFactory_sptr mockIntegratedViewFactory(pMockIntegratedViewFactory);

    // Mock Peaks transform
    auto pMockTransform = new MockPeakTransform;
    PeakTransform_sptr mockTransform(pMockTransform);  
    const int numberOfTimesCalled = static_cast<int>(peaksWS->rowCount());// Should be called for every peak i.e. row.
    EXPECT_CALL(*pMockTransform, transformPeak(_)).Times(numberOfTimesCalled).WillRepeatedly(Return(V3D()));
 
    // Mock Peak transform factory
    auto pMockTransformFactory = new NiceMock<MockPeakTransformFactory>;
    PeakTransformFactory_sptr peakTransformFactory(pMockTransformFactory);
    EXPECT_CALL(*pMockTransformFactory, createDefaultTransform()).WillRepeatedly(Return(mockTransform));
    EXPECT_CALL(*pMockTransformFactory, createDefaultTransform()).WillOnce(Return(mockTransform));
    EXPECT_CALL(*pMockTransformFactory, createTransform(_,_)).WillOnce(Return(mockTransform));

    // Construct the presenter.
    ConcretePeaksPresenter presenter(mockNonIntegratedViewFactory, mockIntegratedViewFactory, peaksWS, mdWS, peakTransformFactory);

    TSM_ASSERT("Non-Integrated View Factory has not been used as expected", Mock::VerifyAndClearExpectations(pMockNonIntegratedViewFactory));
    TSM_ASSERT("Integrated View Factory has not been used as expected", Mock::VerifyAndClearExpectations(pMockIntegratedViewFactory));
    TSM_ASSERT("MockTransformFactory not used as expected", Mock::VerifyAndClearExpectations(pMockTransformFactory));
    TSM_ASSERT("MockTransform not used as expected", Mock::VerifyAndClearExpectations(pMockTransform));
  }

  void test_update()
  {
    // Create a non-integrated widget factory mock.
    auto pMockNonIntegratedPeakViewFactory = new MockPeakOverlayFactory;
    PeakOverlayViewFactory_sptr mockNonIntegratedPeakViewFactory(pMockNonIntegratedPeakViewFactory);

    // Create a spherical widget factory mock
    auto pMockIntegratedPeakViewFactory = new MockPeakOverlayFactory;
    PeakOverlayViewFactory_sptr mockIntegratedPeakViewFactory(pMockIntegratedPeakViewFactory);

    // Set the expectation on the number of calls
    const int expectedNumberPeaks = 10;

    // Create a mock view object that will be returned by the mock factory.
    auto pMockView = new NiceMock<MockPeakOverlayView>;
    EXPECT_CALL(*pMockView, updateView()).Times(expectedNumberPeaks);
    auto mockView = boost::shared_ptr<NiceMock<MockPeakOverlayView> >(pMockView);
    
    EXPECT_CALL(*pMockIntegratedPeakViewFactory, setPeakRadius(_,_,_)).Times(1);
    EXPECT_CALL(*pMockIntegratedPeakViewFactory, setZRange(_,_)).Times(1);
    EXPECT_CALL(*pMockIntegratedPeakViewFactory, createView(_)).WillRepeatedly(Return(mockView));
    EXPECT_CALL(*pMockIntegratedPeakViewFactory, getPlotXLabel()).WillOnce(Return("H"));
    EXPECT_CALL(*pMockIntegratedPeakViewFactory, getPlotYLabel()).WillOnce(Return("K"));

    // Create an input MODEL Peaks workspace (INTEGRATED)
    Mantid::API::IPeaksWorkspace_sptr peaksWS = createPeaksWorkspace(expectedNumberPeaks);
    // Create an input MODEL IMDWorkspace (Geom)
    MDGeometry_sptr mdWS = createExpectedMDWorkspace();

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
    ConcretePeaksPresenter presenter(mockNonIntegratedPeakViewFactory, mockIntegratedPeakViewFactory, peaksWS, mdWS, peakTransformFactory);

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

    EXPECT_CALL(*mockViewFactory, setPeakRadius(_, _, _)).Times(1);
    EXPECT_CALL(*mockViewFactory, setZRange(_,_)).Times(1);
    EXPECT_CALL(*mockViewFactory, createView(_)).WillRepeatedly(Return(mockView));
    EXPECT_CALL(*mockViewFactory, getPlotXLabel()).WillOnce(Return("H"));
    EXPECT_CALL(*mockViewFactory, getPlotYLabel()).WillOnce(Return("K"));

    // Create an input MODEL Peaks workspace (INTEGRATED)
    Mantid::API::IPeaksWorkspace_sptr peaksWS = createPeaksWorkspace(expectedNumberPeaks);
    // Create an input MODEL IMDWorkspace (Geom)
    MDGeometry_sptr mdWS = createExpectedMDWorkspace();

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
    ConcretePeaksPresenter presenter(boost::make_shared<MockPeakOverlayFactory>(), PeakOverlayViewFactory_sptr(mockViewFactory), peaksWS, mdWS, peakTransformFactory);

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

    EXPECT_CALL(*mockViewFactory, setPeakRadius(_, _, _)).Times(1);
    EXPECT_CALL(*mockViewFactory, setZRange(_,_)).Times(1);
    EXPECT_CALL(*mockViewFactory, createView(_)).WillRepeatedly(Return(mockView));
    EXPECT_CALL(*mockViewFactory, getPlotXLabel()).WillOnce(Return("H"));
    EXPECT_CALL(*mockViewFactory, getPlotYLabel()).WillOnce(Return("K"));

    // Create an input MODEL Peaks workspace (INTEGRATED)
    Mantid::API::IPeaksWorkspace_sptr peaksWS = createPeaksWorkspace(expectedNumberPeaks);
    // Create an input MODEL IMDWorkspace (Geom)
    MDGeometry_sptr mdWS = createExpectedMDWorkspace();

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
      ConcretePeaksPresenter presenter(boost::make_shared<MockPeakOverlayFactory>(), PeakOverlayViewFactory_sptr(mockViewFactory), peaksWS, mdWS, peakTransformFactory);
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

    EXPECT_CALL(*mockViewFactory, setPeakRadius(_, _, _)).Times(1);
    EXPECT_CALL(*mockViewFactory, setZRange(_,_)).Times(1);
    EXPECT_CALL(*mockViewFactory, createView(_)).WillRepeatedly(Return(mockView));
    EXPECT_CALL(*mockViewFactory, getPlotXLabel()).WillOnce(Return("Qx")); // Not either H, K or L
    EXPECT_CALL(*mockViewFactory, getPlotYLabel()).WillOnce(Return("K"));
    // Create an input MODEL Peaks workspace (INTEGRATED)
    Mantid::API::IPeaksWorkspace_sptr peaksWS = createPeaksWorkspace(expectedNumberPeaks);
    // Create an input MODEL IMDWorkspace (Geom)
    MDGeometry_sptr mdWS = createExpectedMDWorkspace();

     // Create a mock transform object.
    auto pMockTransform = new NiceMock<MockPeakTransform>;
    PeakTransform_sptr mockTransform(pMockTransform);
    EXPECT_CALL(*pMockTransform, transformPeak(_)).WillRepeatedly(Return(V3D()));

    // Create a mock transform factory.
    auto pMockTransformFactory = new NiceMock<MockPeakTransformFactory>;
    PeakTransformFactory_sptr peakTransformFactory(pMockTransformFactory);
    EXPECT_CALL(*pMockTransformFactory, createDefaultTransform()).WillOnce(Return(mockTransform));
    EXPECT_CALL(*pMockTransformFactory, createTransform(_,_)).WillRepeatedly(Throw(PeakTransformException())); // The actual transform will throw if a mix of Qx and Qy were used.

    ConcretePeaksPresenter presenter(boost::make_shared<MockPeakOverlayFactory>(), PeakOverlayViewFactory_sptr(mockViewFactory), peaksWS, mdWS, peakTransformFactory);
    TSM_ASSERT("MockView not used as expected.", Mock::VerifyAndClearExpectations(pMockView));
    TSM_ASSERT("MockTransformFactory not used as expected", Mock::VerifyAndClearExpectations(pMockTransformFactory));
  }

  void test_setForegroundColour()
  {
    const int nPeaks = 2;
    const QColor colourToChangeTo = Qt::red;

    // Create a mock view object/product that will be returned by the mock factory.
    auto pMockView = new NiceMock<MockPeakOverlayView>;
    auto mockView = boost::shared_ptr<NiceMock<MockPeakOverlayView> >(pMockView); 
    EXPECT_CALL(*pMockView, changeForegroundColour(colourToChangeTo)).Times(nPeaks); // Expect that the foreground colour of each widget will be changed.
    EXPECT_CALL(*pMockView, updateView()).Times(nPeaks); // Expect that the background colour of each widget will be changed.
    // Create a widget factory mock
    auto pMockViewFactory = new MockPeakOverlayFactory;
    PeakOverlayViewFactory_sptr mockViewFactory = PeakOverlayViewFactory_sptr(pMockViewFactory);
    EXPECT_CALL(*pMockViewFactory, createView(_)).WillRepeatedly(Return(mockView));
    EXPECT_CALL(*pMockViewFactory, getPlotXLabel()).WillRepeatedly(Return("H"));
    EXPECT_CALL(*pMockViewFactory, getPlotYLabel()).WillRepeatedly(Return("K"));
    EXPECT_CALL(*pMockViewFactory, setPeakRadius(_,_,_)).Times(AtLeast(1));
    EXPECT_CALL(*pMockViewFactory, setZRange(_,_)).Times(AtLeast(1));

    auto presenterBuilder = createStandardBuild(nPeaks); // Creates a default Concrete presenter product.
    presenterBuilder.withIntegratedViewFactory(mockViewFactory); // Change the view factories to deliver the expected mock object
    presenterBuilder.withNonIntegratedViewFactory(mockViewFactory); // Change the view factories to deliver the expected mock object 
    auto concretePresenter = presenterBuilder.create();

    concretePresenter->setForegroundColour(colourToChangeTo); 

    TS_ASSERT(Mock::VerifyAndClearExpectations(pMockView));
  }

  void test_setBackgroundColour()
  {
    const int nPeaks = 2;
    const QColor colourToChangeTo = Qt::red;

    // Create a mock view object/product that will be returned by the mock factory.
    auto pMockView = new NiceMock<MockPeakOverlayView>;
    auto mockView = boost::shared_ptr<NiceMock<MockPeakOverlayView> >(pMockView);
    EXPECT_CALL(*pMockView, changeBackgroundColour(colourToChangeTo)).Times(nPeaks); // Expect that the background colour on each widget will be changed.
    EXPECT_CALL(*pMockView, updateView()).Times(nPeaks); // Expect that each widget will be updated.
    // Create a widget factory mock
    auto pMockViewFactory = new MockPeakOverlayFactory;
    PeakOverlayViewFactory_sptr mockViewFactory = PeakOverlayViewFactory_sptr(pMockViewFactory);
    EXPECT_CALL(*pMockViewFactory, createView(_)).WillRepeatedly(Return(mockView));
    EXPECT_CALL(*pMockViewFactory, getPlotXLabel()).WillRepeatedly(Return("H"));
    EXPECT_CALL(*pMockViewFactory, getPlotYLabel()).WillRepeatedly(Return("K"));
    EXPECT_CALL(*pMockViewFactory, setPeakRadius(_,_,_)).Times(AtLeast(1));
    EXPECT_CALL(*pMockViewFactory, setZRange(_,_)).Times(AtLeast(1));

    auto presenterBuilder = createStandardBuild(nPeaks); // Creates a default Concrete presenter product.
    presenterBuilder.withIntegratedViewFactory(mockViewFactory); // Change the view factories to deliver the expected mock object
    presenterBuilder.withNonIntegratedViewFactory(mockViewFactory); // Change the view factories to deliver the expected mock object 
    auto concretePresenter = presenterBuilder.create();

    concretePresenter->setBackgroundColour(colourToChangeTo); 

    TS_ASSERT(Mock::VerifyAndClearExpectations(pMockView));
  }

  void test_setShown()
  {
    const int expectedNumberOfPeaks = 5;
    auto concreteBuilder = createStandardBuild(expectedNumberOfPeaks);

    // Create a mock view object/product that will be returned by the mock factory.
    auto pMockView = new NiceMock<MockPeakOverlayView>;
    auto mockView = boost::shared_ptr<NiceMock<MockPeakOverlayView> >(pMockView);
    EXPECT_CALL(*pMockView, showView()).Times(expectedNumberOfPeaks); // Expect that the view will be forced to SHOW.
    EXPECT_CALL(*pMockView, hideView()).Times(expectedNumberOfPeaks); // Expect that the view will be forced to HIDE.
    EXPECT_CALL(*pMockView, updateView()).Times(2*expectedNumberOfPeaks); // Expect that each widget will be updated.
    // Create a widget factory mock
    auto pMockViewFactory = new MockPeakOverlayFactory;
    PeakOverlayViewFactory_sptr mockViewFactory = PeakOverlayViewFactory_sptr(pMockViewFactory);
    EXPECT_CALL(*pMockViewFactory, createView(_)).WillRepeatedly(Return(mockView));
    EXPECT_CALL(*pMockViewFactory, getPlotXLabel()).WillRepeatedly(Return("H"));
    EXPECT_CALL(*pMockViewFactory, getPlotYLabel()).WillRepeatedly(Return("K"));
    EXPECT_CALL(*pMockViewFactory, setPeakRadius(_,_,_)).Times(AtLeast(1));
    EXPECT_CALL(*pMockViewFactory, setZRange(_,_)).Times(AtLeast(1));

    concreteBuilder.withIntegratedViewFactory(mockViewFactory);

    auto presenter = concreteBuilder.create();
    presenter->setShown(true);
    presenter->setShown(false);

    TS_ASSERT(Mock::VerifyAndClearExpectations(pMockView));
  }

  void test_getBoundingBox_throws_if_index_too_low()
  {
    auto concreteBuilder = createStandardBuild();
    ConcretePeaksPresenter_sptr presenter = concreteBuilder.create();

    const int badIndex = -1;
    TSM_ASSERT_THROWS("Index is < 0, should throw", presenter->getBoundingBox(badIndex), std::out_of_range);
  }

  void test_getBoundingBox_throws_if_index_too_high()
  {
    auto concreteBuilder = createStandardBuild();
    ConcretePeaksPresenter_sptr presenter = concreteBuilder.create();

    const size_t numberOfPeaks = (*presenter->presentedWorkspaces().begin())->rowCount();
    const int badIndex = static_cast<int>(numberOfPeaks) + 1;
    TSM_ASSERT_THROWS("Index is < 0, should throw", presenter->getBoundingBox(badIndex), std::out_of_range);
  }

  void test_getBoundingBox()
  {
    const int expectedNumberOfPeaks = 1;
    auto concreteBuilder = createStandardBuild(expectedNumberOfPeaks);

    // Create a mock view object/product that will be returned by the mock factory.
    auto pMockView = new NiceMock<MockPeakOverlayView>;
    auto mockView = boost::shared_ptr<NiceMock<MockPeakOverlayView> >(pMockView);
    EXPECT_CALL(*pMockView, getBoundingBox()).Times(1).WillOnce(Return(PeakBoundingBox())); // Expect that the bounding box will be requested.

    // Create a widget factory mock
    auto pMockViewFactory = new MockPeakOverlayFactory;
    PeakOverlayViewFactory_sptr mockViewFactory = PeakOverlayViewFactory_sptr(pMockViewFactory);
    EXPECT_CALL(*pMockViewFactory, createView(_)).WillRepeatedly(Return(mockView));
    EXPECT_CALL(*pMockViewFactory, getPlotXLabel()).WillRepeatedly(Return("H"));
    EXPECT_CALL(*pMockViewFactory, getPlotYLabel()).WillRepeatedly(Return("K"));
    EXPECT_CALL(*pMockViewFactory, setPeakRadius(_,_,_)).Times(AtLeast(1));
    EXPECT_CALL(*pMockViewFactory, setZRange(_,_)).Times(AtLeast(1));

    concreteBuilder.withIntegratedViewFactory(mockViewFactory);

    auto presenter = concreteBuilder.create();
    presenter->getBoundingBox(0);
    TS_ASSERT(Mock::VerifyAndClearExpectations(pMockView));
  }

  void doTestSorting(const bool sortAscending)
  {
    FrameworkManager::Instance();

    const int expectedNumberOfPeaks = 1;
    auto concreteBuilder = createStandardBuild(expectedNumberOfPeaks);

    // Create a mock view object/product that will be returned by the mock factory.
    auto pMockView = new NiceMock<MockPeakOverlayView>;
    auto mockView = boost::shared_ptr<NiceMock<MockPeakOverlayView> >(pMockView);
    EXPECT_CALL(*pMockView, setSlicePoint(_)).Times(1); // Expect that the slice point will be re-set upon sorting.

    // Create a widget factory mock
    auto pMockViewFactory = new MockPeakOverlayFactory;
    PeakOverlayViewFactory_sptr mockViewFactory = PeakOverlayViewFactory_sptr(pMockViewFactory);
    EXPECT_CALL(*pMockViewFactory, createView(_)).WillRepeatedly(Return(mockView));
    EXPECT_CALL(*pMockViewFactory, getPlotXLabel()).WillRepeatedly(Return("H"));
    EXPECT_CALL(*pMockViewFactory, getPlotYLabel()).WillRepeatedly(Return("K"));
    EXPECT_CALL(*pMockViewFactory, setPeakRadius(_,_,_)).Times(AtLeast(1));
    EXPECT_CALL(*pMockViewFactory, setZRange(_,_)).Times(AtLeast(1));

    concreteBuilder.withIntegratedViewFactory(mockViewFactory);

    auto presenter = concreteBuilder.create();
    presenter->sortPeaksWorkspace("h", sortAscending); // Sort the presenter by the h column.
    TS_ASSERT(Mock::VerifyAndClearExpectations(pMockView));

    // Check that the workspace is sorted.
    IPeaksWorkspace_const_sptr sortedPeaksWS = *presenter->presentedWorkspaces().begin();
    std::vector<double> potentiallySortedHValues;
    for (int i = 0; i < sortedPeaksWS->getNumberPeaks(); ++i)
    {
      const IPeak& peak = sortedPeaksWS->getPeak(i);
      potentiallySortedHValues.push_back(peak.getH());
    }

    if (sortAscending == true)
    {
      TSM_ASSERT("The internal peaks workspace shuld have been internally sorted ASCENDING by H values",
          this->isSortedAscending(potentiallySortedHValues));
    }
    else
    {
      TSM_ASSERT("The internal peaks workspace shuld have been internally sorted DESCENDING by H values",
          this->isSortedDescending(potentiallySortedHValues));
    }

  }

  void test_sortPeaksWorkspace_by_H_Ascending()
  {
    bool sortAscending = true;
    doTestSorting(sortAscending);
  }

  void test_sortPeaksWorkspace_by_H_Descending()
  {
    bool sortAscending = false;
    doTestSorting(sortAscending);
  }

};


#endif
