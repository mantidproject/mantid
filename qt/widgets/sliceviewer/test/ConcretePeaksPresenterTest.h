// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef SLICE_VIEWER_PEAKS_PRESENTER_TEST_H_
#define SLICE_VIEWER_PEAKS_PRESENTER_TEST_H_

#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeakShapeSpherical.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Crystal/IPeak.h"
#include "MantidGeometry/Crystal/PeakTransformFactory.h"
#include "MantidKernel/SpecialCoordinateSystem.h"
#include "MantidQtWidgets/SliceViewer/ConcretePeaksPresenter.h"
#include "MantidQtWidgets/SliceViewer/PeakOverlayViewFactory.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MockObjects.h"
#include <algorithm>
#include <boost/make_shared.hpp>
#include <cxxtest/TestSuite.h>
#include <string>

using namespace MantidQt::SliceViewer;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace testing;
using boost::regex;

// Alias.
using MDGeometry_sptr = boost::shared_ptr<Mantid::API::MDGeometry>;

class ConcretePeaksPresenterTest : public CxxTest::TestSuite {
  /// Alias.
  using ConcretePeaksPresenter_sptr =
      boost::shared_ptr<MantidQt::SliceViewer::ConcretePeaksPresenter>;

  /// Helper method to create a good 'Integrated' peaks workspace
  Mantid::API::IPeaksWorkspace_sptr
  createPeaksWorkspace(const int nPeaks, const double radius = 1) {
    Mantid::API::IPeaksWorkspace_sptr peaksWS =
        WorkspaceCreationHelper::createPeaksWorkspace(nPeaks);
    peaksWS->mutableRun().addProperty("PeaksIntegrated", true);
    peaksWS->mutableRun().addProperty("PeakRadius", radius);
    peaksWS->mutableRun().addProperty("BackgroundInnerRadius", radius + 1);
    peaksWS->mutableRun().addProperty("BackgroundOuterRadius", radius + 2);
    return peaksWS;
  }

  /// Helper method to create a mock MDDimension.
  IMDDimension_sptr createExpectedMDDimension(const std::string returnLabel) {
    auto *pDim = new NiceMock<MockIMDDimension>;
    IMDDimension_sptr dim(pDim);
    EXPECT_CALL(*pDim, getName()).WillRepeatedly(Return(returnLabel));
    return dim;
  }

  /// Helper method to create an expected MDGeometry (we call it MDWorkspace
  /// here).
  MDGeometry_sptr createExpectedMDWorkspace() {
    // Create a mock H Dim
    IMDDimension_sptr HDim = createExpectedMDDimension("H");
    // Create a mock K Dim
    IMDDimension_sptr KDim = createExpectedMDDimension("K");
    // Create a mock L Dim
    IMDDimension_sptr LDim = createExpectedMDDimension("L");

    // Create the mock MD geometry
    MockMDGeometry *pGeometry = new MockMDGeometry;
    EXPECT_CALL(*pGeometry, getNumDims()).WillRepeatedly(Return(3));
    EXPECT_CALL(*pGeometry, getDimension(0)).WillRepeatedly(Return(HDim));
    EXPECT_CALL(*pGeometry, getDimension(1)).WillRepeatedly(Return(KDim));
    EXPECT_CALL(*pGeometry, getDimension(2)).WillRepeatedly(Return(LDim));

    return boost::shared_ptr<MockMDGeometry>(pGeometry);
  }

  /** Make the tests easier to write and understand by utilising
  a builder. This means that we can create a standard product in one line of
  test code, but explicilty override
  constructor inputs as the test requires.
  */
  class ConcretePeaksPresenterBuilder {
  private:
    PeakOverlayViewFactory_sptr m_viewFactory;
    IPeaksWorkspace_sptr m_peaksWS;
    boost::shared_ptr<MDGeometry> m_mdWS;
    PeakTransformFactory_sptr m_transformFactory;

  public:
    ConcretePeaksPresenterBuilder() {}

    ConcretePeaksPresenterBuilder(const ConcretePeaksPresenterBuilder &other) {
      m_viewFactory = other.m_viewFactory;
      m_peaksWS = other.m_peaksWS;
      m_mdWS = other.m_mdWS;
      m_transformFactory = other.m_transformFactory;
    }

    void withViewFactory(PeakOverlayViewFactory_sptr val) {
      m_viewFactory = val;
    }
    void withPeaksWorkspace(IPeaksWorkspace_sptr val) { m_peaksWS = val; }
    void withMDWorkspace(boost::shared_ptr<MDGeometry> val) { m_mdWS = val; }
    void withTransformFactory(PeakTransformFactory_sptr val) {
      m_transformFactory = val;
    }

    ConcretePeaksPresenter_sptr create() {
      return boost::make_shared<ConcretePeaksPresenter>(
          m_viewFactory, m_peaksWS, m_mdWS, m_transformFactory);
    }
  };

  /**
  Helper method that will produce a customisable object (builder) for making
  ConcretePeaks presenter.
  1) All constructor parameters can be overriden using methods with....() on the
  returned builder object
  2) The default builder has been set up to create a ubiquitious
  ConcretePeaksPresenter product.
  */
  ConcretePeaksPresenterBuilder
  createStandardBuild(const int expectedNumberPeaks = 5,
                      const double radius = 0.0,
                      const SpecialCoordinateSystem frame = QLab) {
    // Map enum to string.
    std::string frame_str = "Q (lab frame)";
    if (frame == Mantid::Kernel::HKL) {
      frame_str = "HKL";
    } else if (frame == QSample) {
      frame_str = "Q (sample frame)";
    }

    // Create a mock view object that will be returned by the mock factory.
    auto mockView = boost::make_shared<NiceMock<MockPeakOverlayView>>();
    EXPECT_CALL(*mockView.get(), getRadius()).WillRepeatedly(Return(radius));

    // Create a widget factory mock
    auto pMockViewFactory = new MockPeakOverlayFactory;

    PeakOverlayViewFactory_sptr mockViewFactory =
        PeakOverlayViewFactory_sptr(pMockViewFactory);
    EXPECT_CALL(*pMockViewFactory, createView(_, _))
        .WillRepeatedly(Return(mockView));
    EXPECT_CALL(*pMockViewFactory, getPlotXLabel()).WillRepeatedly(Return("H"));
    EXPECT_CALL(*pMockViewFactory, getPlotYLabel()).WillRepeatedly(Return("K"));

    // Create an input MODEL Peaks workspace (INTEGRATED)
    Mantid::API::IPeaksWorkspace_sptr peaksWS =
        createPeaksWorkspace(expectedNumberPeaks);
    // Create an input MODEL IMDWorkspace (Geom)
    MDGeometry_sptr mdWS = createExpectedMDWorkspace();

    // Create a mock transform object.
    auto pMockTransform = new NiceMock<MockPeakTransform>;
    PeakTransform_sptr mockTransform(pMockTransform);
    EXPECT_CALL(*pMockTransform, transformPeak(_))
        .WillRepeatedly(Return(V3D()));
    EXPECT_CALL(*pMockTransform, getFriendlyName())
        .WillRepeatedly(Return(frame_str));
    EXPECT_CALL(*pMockTransform, getCoordinateSystem())
        .WillRepeatedly(Return(frame));

    // Create a mock transform factory.
    auto pMockTransformFactory = new NiceMock<MockPeakTransformFactory>;
    PeakTransformFactory_sptr peakTransformFactory(pMockTransformFactory);
    EXPECT_CALL(*pMockTransformFactory, createDefaultTransform())
        .WillRepeatedly(Return(mockTransform));
    EXPECT_CALL(*pMockTransformFactory, createTransform(_, _))
        .WillRepeatedly(Return(mockTransform));

    // Create and return a configurable builder.
    ConcretePeaksPresenterBuilder builder;
    builder.withViewFactory(mockViewFactory);
    builder.withPeaksWorkspace(peaksWS);
    builder.withMDWorkspace(mdWS);
    builder.withTransformFactory(peakTransformFactory);
    return builder;
  }

public:
  void setUp() override { FrameworkManager::Instance(); }

  void test_construction() {
    // Expected number of peaks to create
    const size_t expectedNumberPeaks = 1;

    // Peaks workspace IS INTEGRATED.
    IPeaksWorkspace_sptr peaksWS = createPeaksWorkspace(expectedNumberPeaks);

    // Create an MDWorkspace
    MDGeometry_sptr mdWS = createExpectedMDWorkspace();

    // Mock View Factory Product
    auto pMockView = new NiceMock<MockPeakOverlayView>;
    auto mockView = boost::shared_ptr<NiceMock<MockPeakOverlayView>>(pMockView);

    // Mock View Factory for integrated peaks. We expect that this will never be
    // used.
    auto pMockViewFactory = new MockPeakOverlayFactory;
    PeakOverlayViewFactory_sptr mockViewFactory(pMockViewFactory);
    EXPECT_CALL(*pMockViewFactory, createView(_, _))
        .Times(1)
        .WillRepeatedly(
            Return(mockView)); // Create a single widget/view for all peaks
    EXPECT_CALL(*pMockViewFactory, getPlotXLabel()).WillOnce(Return("H"));
    EXPECT_CALL(*pMockViewFactory, getPlotYLabel()).WillOnce(Return("K"));

    // Create a mock transform object.
    auto pMockTransform = new NiceMock<MockPeakTransform>;
    PeakTransform_sptr mockTransform(pMockTransform);
    EXPECT_CALL(*pMockTransform, getCoordinateSystem())
        .WillRepeatedly(Return(Mantid::Kernel::QLab));

    // Create a mock transform factory.
    auto pMockTransformFactory = new NiceMock<MockPeakTransformFactory>;
    PeakTransformFactory_sptr peakTransformFactory(pMockTransformFactory);
    EXPECT_CALL(*pMockTransformFactory, createDefaultTransform())
        .WillOnce(Return(mockTransform));
    EXPECT_CALL(*pMockTransformFactory, createTransform(_, _))
        .WillOnce(Return(mockTransform));

    // Construct the presenter.
    ConcretePeaksPresenter presenter(mockViewFactory, peaksWS, mdWS,
                                     peakTransformFactory);

    TSM_ASSERT("View Factory has not been used as expected",
               Mock::VerifyAndClearExpectations(pMockViewFactory));
    TSM_ASSERT("MockView not used as expected.",
               Mock::VerifyAndClearExpectations(pMockView));
    TSM_ASSERT("MockTransformFactory not used as expected",
               Mock::VerifyAndClearExpectations(pMockTransformFactory));
    TSM_ASSERT("MockTransform not used as expected",
               Mock::VerifyAndClearExpectations(pMockTransform));

    auto ownedPeaksWorkspace = presenter.presentedWorkspaces();
    TS_ASSERT_EQUALS(1, ownedPeaksWorkspace.size());
  }

  void test_update() {

    auto pMockViewFactory = new MockPeakOverlayFactory;
    PeakOverlayViewFactory_sptr mockViewFactory(pMockViewFactory);

    // Set the expectation on the number of calls
    const int expectedNumberPeaks = 10;

    // Create a mock view object that will be returned by the mock factory.
    auto pMockView = new NiceMock<MockPeakOverlayView>;
    EXPECT_CALL(*pMockView, updateView())
        .Times(1); // Single view, for this presenter, will only update once.
    auto mockView = boost::shared_ptr<NiceMock<MockPeakOverlayView>>(pMockView);

    EXPECT_CALL(*pMockViewFactory, createView(_, _))
        .WillRepeatedly(Return(mockView));
    EXPECT_CALL(*pMockViewFactory, getPlotXLabel()).WillOnce(Return("H"));
    EXPECT_CALL(*pMockViewFactory, getPlotYLabel()).WillOnce(Return("K"));

    // Create an input MODEL Peaks workspace (INTEGRATED)
    Mantid::API::IPeaksWorkspace_sptr peaksWS =
        createPeaksWorkspace(expectedNumberPeaks);
    // Create an input MODEL IMDWorkspace (Geom)
    MDGeometry_sptr mdWS = createExpectedMDWorkspace();

    // Create a mock transform object.
    auto pMockTransform = new NiceMock<MockPeakTransform>;
    PeakTransform_sptr mockTransform(pMockTransform);
    EXPECT_CALL(*pMockTransform, transformPeak(_))
        .WillRepeatedly(Return(V3D()));
    EXPECT_CALL(*pMockTransform, getCoordinateSystem())
        .WillRepeatedly(Return(Mantid::Kernel::QLab));

    // Create a mock transform factory.
    auto pMockTransformFactory = new NiceMock<MockPeakTransformFactory>;
    PeakTransformFactory_sptr peakTransformFactory(pMockTransformFactory);
    EXPECT_CALL(*pMockTransformFactory, createDefaultTransform())
        .WillOnce(Return(mockTransform));
    EXPECT_CALL(*pMockTransformFactory, createTransform(_, _))
        .WillOnce(Return(mockTransform));

    // Construction should cause the widget factory to be used to generate peak
    // overlay objects.
    ConcretePeaksPresenter presenter(mockViewFactory, peaksWS, mdWS,
                                     peakTransformFactory);

    // Updating should cause all of the held views to be updated too.
    presenter.update();

    TSM_ASSERT("MockView not used as expected.",
               Mock::VerifyAndClearExpectations(pMockView));
  }

  void test_set_slice_point() {
    // Create a widget factory mock
    auto mockViewFactory = new MockPeakOverlayFactory;

    const double slicePoint = 0.1;
    const int expectedNumberPeaks = 10;

    // Create a mock view object that will be returned by the mock factory.
    auto pMockView = new NiceMock<MockPeakOverlayView>;
    EXPECT_CALL(*pMockView, setSlicePoint(slicePoint, _))
        .Times(1); // Only one widget for this presenter
    auto mockView = boost::shared_ptr<NiceMock<MockPeakOverlayView>>(pMockView);

    EXPECT_CALL(*mockViewFactory, createView(_, _))
        .WillRepeatedly(Return(mockView));
    EXPECT_CALL(*mockViewFactory, getPlotXLabel()).WillOnce(Return("H"));
    EXPECT_CALL(*mockViewFactory, getPlotYLabel()).WillOnce(Return("K"));

    // Create an input MODEL Peaks workspace (INTEGRATED)
    Mantid::API::IPeaksWorkspace_sptr peaksWS =
        createPeaksWorkspace(expectedNumberPeaks);
    // Create an input MODEL IMDWorkspace (Geom)
    MDGeometry_sptr mdWS = createExpectedMDWorkspace();

    // Create a mock transform object.
    auto pMockTransform = new NiceMock<MockPeakTransform>;
    PeakTransform_sptr mockTransform(pMockTransform);
    EXPECT_CALL(*pMockTransform, getCoordinateSystem())
        .WillRepeatedly(Return(Mantid::Kernel::HKL));
    EXPECT_CALL(*pMockTransform, getFriendlyName()).WillOnce(Return("HKL"));
    EXPECT_CALL(*pMockTransform, transformPeak(_))
        .WillRepeatedly(Return(V3D()));

    // Create a mock transform factory.
    auto pMockTransformFactory = new NiceMock<MockPeakTransformFactory>;
    PeakTransformFactory_sptr peakTransformFactory(pMockTransformFactory);
    EXPECT_CALL(*pMockTransformFactory, createDefaultTransform())
        .WillOnce(Return(mockTransform));
    EXPECT_CALL(*pMockTransformFactory, createTransform(_, _))
        .WillOnce(Return(mockTransform));

    // Construction should cause the widget factory to be used to generate peak
    // overlay objects.
    ConcretePeaksPresenter presenter(
        PeakOverlayViewFactory_sptr(mockViewFactory), peaksWS, mdWS,
        peakTransformFactory);

    // Updating should cause all of the held views to be updated too.
    PeakBoundingBox region(Left(-1), Right(1), Top(1), Bottom(-1),
                           SlicePoint(slicePoint));
    presenter.updateWithSlicePoint(region);

    TSM_ASSERT("MockView not used as expected.",
               Mock::VerifyAndClearExpectations(pMockView));
    TSM_ASSERT("MockTransformFactory not used as expected",
               Mock::VerifyAndClearExpectations(pMockTransformFactory));
  }

  void test_hide_owned_views_on_death() {
    // Create a widget factory mock
    auto mockViewFactory = new MockPeakOverlayFactory;

    const int expectedNumberPeaks = 1;

    // Create a mock view object that will be returned by the mock factory.
    auto pMockView = new NiceMock<MockPeakOverlayView>;
    EXPECT_CALL(*pMockView, hideView()).Times(expectedNumberPeaks);
    auto mockView = boost::shared_ptr<NiceMock<MockPeakOverlayView>>(pMockView);

    EXPECT_CALL(*mockViewFactory, createView(_, _))
        .WillRepeatedly(Return(mockView));
    EXPECT_CALL(*mockViewFactory, getPlotXLabel()).WillOnce(Return("H"));
    EXPECT_CALL(*mockViewFactory, getPlotYLabel()).WillOnce(Return("K"));

    // Create an input MODEL Peaks workspace (INTEGRATED)
    Mantid::API::IPeaksWorkspace_sptr peaksWS =
        createPeaksWorkspace(expectedNumberPeaks);
    // Create an input MODEL IMDWorkspace (Geom)
    MDGeometry_sptr mdWS = createExpectedMDWorkspace();

    // Create a mock transform object.
    auto pMockTransform = new NiceMock<MockPeakTransform>;
    PeakTransform_sptr mockTransform(pMockTransform);
    EXPECT_CALL(*pMockTransform, transformPeak(_))
        .WillRepeatedly(Return(V3D()));

    // Create a mock transform factory.
    auto pMockTransformFactory = new NiceMock<MockPeakTransformFactory>;
    PeakTransformFactory_sptr peakTransformFactory(pMockTransformFactory);
    EXPECT_CALL(*pMockTransform, getCoordinateSystem())
        .WillRepeatedly(Return(Mantid::Kernel::QLab));
    EXPECT_CALL(*pMockTransformFactory, createDefaultTransform())
        .WillOnce(Return(mockTransform));
    EXPECT_CALL(*pMockTransformFactory, createTransform(_, _))
        .WillOnce(Return(mockTransform));

    {
      ConcretePeaksPresenter presenter(
          PeakOverlayViewFactory_sptr(mockViewFactory), peaksWS, mdWS,
          peakTransformFactory);
    } // Guaranteed destruction at this point. Destructor should trigger hide on
    // all owned views.

    TSM_ASSERT("MockView not used as expected.",
               Mock::VerifyAndClearExpectations(pMockView));
    TSM_ASSERT("MockTransformFactory not used as expected",
               Mock::VerifyAndClearExpectations(pMockTransformFactory));
  }

  void test_handle_non_hkl_xy_mappings() {
    // Create a widget factory mock
    auto mockViewFactory = new MockPeakOverlayFactory;

    const int expectedNumberPeaks = 1;

    // Create a mock view object that will be returned by the mock factory.
    auto pMockView = new NiceMock<MockPeakOverlayView>;
    EXPECT_CALL(*pMockView, hideView())
        .Times(expectedNumberPeaks); // This will be called automatically
                                     // because the presenter won't be able to
                                     // map Qx (below).
    auto mockView = boost::shared_ptr<NiceMock<MockPeakOverlayView>>(pMockView);

    EXPECT_CALL(*mockViewFactory, createView(_, _))
        .WillRepeatedly(Return(mockView));
    EXPECT_CALL(*mockViewFactory, getPlotXLabel())
        .WillOnce(Return("Qx")); // Not either H, K or L
    EXPECT_CALL(*mockViewFactory, getPlotYLabel()).WillOnce(Return("K"));
    // Create an input MODEL Peaks workspace (INTEGRATED)
    Mantid::API::IPeaksWorkspace_sptr peaksWS =
        createPeaksWorkspace(expectedNumberPeaks);
    // Create an input MODEL IMDWorkspace (Geom)
    MDGeometry_sptr mdWS = createExpectedMDWorkspace();

    // Create a mock transform object.
    auto pMockTransform = new NiceMock<MockPeakTransform>;
    PeakTransform_sptr mockTransform(pMockTransform);
    EXPECT_CALL(*pMockTransform, getCoordinateSystem())
        .WillRepeatedly(Return(Mantid::Kernel::QLab));
    EXPECT_CALL(*pMockTransform, transformPeak(_))
        .WillRepeatedly(Return(V3D()));

    // Create a mock transform factory.
    auto pMockTransformFactory = new NiceMock<MockPeakTransformFactory>;
    PeakTransformFactory_sptr peakTransformFactory(pMockTransformFactory);
    EXPECT_CALL(*pMockTransformFactory, createDefaultTransform())
        .WillOnce(Return(mockTransform));
    EXPECT_CALL(*pMockTransformFactory, createTransform(_, _))
        .WillRepeatedly(Throw(PeakTransformException())); // The actual
                                                          // transform will
                                                          // throw if a mix of
                                                          // Qx and Qy were
                                                          // used.

    ConcretePeaksPresenter presenter(
        PeakOverlayViewFactory_sptr(mockViewFactory), peaksWS, mdWS,
        peakTransformFactory);
    TSM_ASSERT("MockView not used as expected.",
               Mock::VerifyAndClearExpectations(pMockView));
    TSM_ASSERT("MockTransformFactory not used as expected",
               Mock::VerifyAndClearExpectations(pMockTransformFactory));
  }

  void test_setForegroundColour() {
    const int nPeaks = 2;
    const PeakViewColor colorToChangeTo(Qt::red, Qt::red, Qt::red);

    // Create a mock view object/product that will be returned by the mock
    // factory.
    auto pMockView = new NiceMock<MockPeakOverlayView>;
    auto mockView = boost::shared_ptr<NiceMock<MockPeakOverlayView>>(pMockView);
    EXPECT_CALL(*pMockView, changeForegroundColour(colorToChangeTo))
        .Times(1); // Expect that the foreground colour will be changed.
    EXPECT_CALL(*pMockView, updateView())
        .Times(1); // Only one view for this presenter.
    // Create a widget factory mock
    auto pMockViewFactory = new MockPeakOverlayFactory;
    PeakOverlayViewFactory_sptr mockViewFactory =
        PeakOverlayViewFactory_sptr(pMockViewFactory);
    EXPECT_CALL(*pMockViewFactory, createView(_, _))
        .WillRepeatedly(Return(mockView));
    EXPECT_CALL(*pMockViewFactory, getPlotXLabel()).WillRepeatedly(Return("H"));
    EXPECT_CALL(*pMockViewFactory, getPlotYLabel()).WillRepeatedly(Return("K"));

    auto presenterBuilder = createStandardBuild(
        nPeaks); // Creates a default Concrete presenter product.
    presenterBuilder.withViewFactory(mockViewFactory); // Change the view
                                                       // factories to deliver
                                                       // the expected mock
                                                       // object
    auto concretePresenter = presenterBuilder.create();

    concretePresenter->setForegroundColor(colorToChangeTo);

    TS_ASSERT(Mock::VerifyAndClearExpectations(pMockView));
  }

  void test_setBackgroundColour() {

    const int nPeaks = 2;
    const PeakViewColor colorToChangeTo;

    // Create a mock view object/product that will be returned by the mock
    // factory.
    auto pMockView = new NiceMock<MockPeakOverlayView>;
    auto mockView = boost::shared_ptr<NiceMock<MockPeakOverlayView>>(pMockView);

    EXPECT_CALL(*pMockView, changeBackgroundColour(colorToChangeTo))
        .Times(1); // Expect that the background colour will be changed.
    EXPECT_CALL(*pMockView, updateView())
        .Times(1); // Expect that each widget will be updated.

    // Create a widget factory mock
    auto pMockViewFactory = new MockPeakOverlayFactory;
    PeakOverlayViewFactory_sptr mockViewFactory =
        PeakOverlayViewFactory_sptr(pMockViewFactory);
    EXPECT_CALL(*pMockViewFactory, createView(_, _))
        .WillRepeatedly(Return(mockView));
    EXPECT_CALL(*pMockViewFactory, getPlotXLabel()).WillRepeatedly(Return("H"));
    EXPECT_CALL(*pMockViewFactory, getPlotYLabel()).WillRepeatedly(Return("K"));

    auto presenterBuilder = createStandardBuild(
        nPeaks); // Creates a default Concrete presenter product.
    presenterBuilder.withViewFactory(mockViewFactory); // Change the view
                                                       // factories to deliver
                                                       // the expected mock
                                                       // object
    auto concretePresenter = presenterBuilder.create();

    concretePresenter->setBackgroundColor(colorToChangeTo);

    TS_ASSERT(Mock::VerifyAndClearExpectations(pMockView));
  }

  void test_setShown() {
    const int expectedNumberOfPeaks = 5;
    auto concreteBuilder = createStandardBuild(expectedNumberOfPeaks);

    // Create a mock view object/product that will be returned by the mock
    // factory.
    auto pMockView = new NiceMock<MockPeakOverlayView>;
    auto mockView = boost::shared_ptr<NiceMock<MockPeakOverlayView>>(pMockView);
    EXPECT_CALL(*pMockView, showView())
        .Times(2); // Expect that the view will be forced to SHOW.
    EXPECT_CALL(*pMockView, hideView())
        .Times(1); // Expect that the view will be forced to HIDE.
    EXPECT_CALL(*pMockView, updateView())
        .Times(2); // Expect that each widget will be updated.
    // Create a widget factory mock
    auto pMockViewFactory = new MockPeakOverlayFactory;
    PeakOverlayViewFactory_sptr mockViewFactory =
        PeakOverlayViewFactory_sptr(pMockViewFactory);
    EXPECT_CALL(*pMockViewFactory, createView(_, _))
        .WillRepeatedly(Return(mockView));
    EXPECT_CALL(*pMockViewFactory, getPlotXLabel()).WillRepeatedly(Return("H"));
    EXPECT_CALL(*pMockViewFactory, getPlotYLabel()).WillRepeatedly(Return("K"));

    concreteBuilder.withViewFactory(mockViewFactory);

    auto presenter = concreteBuilder.create();
    presenter->setShown(true);
    presenter->setShown(false);

    TS_ASSERT(Mock::VerifyAndClearExpectations(pMockView));
  }

  void test_getBoundingBox_throws_if_index_too_low() {
    auto concreteBuilder = createStandardBuild();
    ConcretePeaksPresenter_sptr presenter = concreteBuilder.create();

    const int badIndex = -1;
    TSM_ASSERT_THROWS("Index is < 0, should throw",
                      presenter->getBoundingBox(badIndex), std::out_of_range &);
  }

  void test_getBoundingBox_throws_if_index_too_high() {
    auto concreteBuilder = createStandardBuild();
    ConcretePeaksPresenter_sptr presenter = concreteBuilder.create();

    const size_t numberOfPeaks =
        (*presenter->presentedWorkspaces().begin())->rowCount();
    const int badIndex = static_cast<int>(numberOfPeaks) + 1;
    TSM_ASSERT_THROWS("Index is < 0, should throw",
                      presenter->getBoundingBox(badIndex), std::out_of_range &);
  }

  void test_getBoundingBox() {
    const int expectedNumberOfPeaks = 1;
    auto concreteBuilder = createStandardBuild(expectedNumberOfPeaks);

    // Create a mock view object/product that will be returned by the mock
    // factory.
    auto pMockView = new NiceMock<MockPeakOverlayView>;
    auto mockView = boost::shared_ptr<NiceMock<MockPeakOverlayView>>(pMockView);
    EXPECT_CALL(*pMockView, getBoundingBox(_))
        .Times(1)
        .WillOnce(Return(PeakBoundingBox())); // Expect that the bounding box
                                              // will be requested.

    // Create a widget factory mock
    auto pMockViewFactory = new MockPeakOverlayFactory;
    PeakOverlayViewFactory_sptr mockViewFactory =
        PeakOverlayViewFactory_sptr(pMockViewFactory);
    EXPECT_CALL(*pMockViewFactory, createView(_, _))
        .WillRepeatedly(Return(mockView));
    EXPECT_CALL(*pMockViewFactory, getPlotXLabel()).WillRepeatedly(Return("H"));
    EXPECT_CALL(*pMockViewFactory, getPlotYLabel()).WillRepeatedly(Return("K"));

    concreteBuilder.withViewFactory(mockViewFactory);

    auto presenter = concreteBuilder.create();
    presenter->getBoundingBox(0);
    TS_ASSERT(Mock::VerifyAndClearExpectations(pMockView));
  }

  void test_coordinateToString() {
    TS_ASSERT_EQUALS("HKL", coordinateToString(Mantid::Kernel::HKL));
    TS_ASSERT_EQUALS("QLab", coordinateToString(Mantid::Kernel::QLab));
    TS_ASSERT_EQUALS("QSample", coordinateToString(Mantid::Kernel::QSample));
  }

  void test_getPeaksSizeOnProjection() {
    const int nPeaks = 1;
    const double occupancyInView = 0.07;

    // Create a mock view object/product that will be returned by the mock
    // factory.
    auto pMockView = new NiceMock<MockPeakOverlayView>;
    auto mockView = boost::shared_ptr<NiceMock<MockPeakOverlayView>>(pMockView);
    EXPECT_CALL(*pMockView, getOccupancyInView())
        .WillOnce(
            Return(occupancyInView)); // The occupancy that the VIEW returns.
    // Create a widget factory mock
    auto pMockViewFactory = new MockPeakOverlayFactory;
    PeakOverlayViewFactory_sptr mockViewFactory =
        PeakOverlayViewFactory_sptr(pMockViewFactory);
    EXPECT_CALL(*pMockViewFactory, createView(_, _))
        .WillRepeatedly(Return(mockView));
    EXPECT_CALL(*pMockViewFactory, getPlotXLabel()).WillRepeatedly(Return("H"));
    EXPECT_CALL(*pMockViewFactory, getPlotYLabel()).WillRepeatedly(Return("K"));

    auto presenterBuilder = createStandardBuild(
        nPeaks); // Creates a default Concrete presenter product.
    presenterBuilder.withViewFactory(mockViewFactory); // Change the view
                                                       // factories to deliver
                                                       // the expected mock
                                                       // object
    auto concretePresenter = presenterBuilder.create();

    TS_ASSERT_EQUALS(occupancyInView,
                     concretePresenter->getPeakSizeOnProjection());

    TS_ASSERT(Mock::VerifyAndClearExpectations(pMockView));
  }

  void test_getPeaksSizeIntoProjection() {
    const int nPeaks = 1;
    const double occupancyIntoView = 0.05;

    // Create a mock view object/product that will be returned by the mock
    // factory.
    auto pMockView = new NiceMock<MockPeakOverlayView>;
    auto mockView = boost::shared_ptr<NiceMock<MockPeakOverlayView>>(pMockView);
    EXPECT_CALL(*pMockView, getOccupancyIntoView())
        .WillOnce(
            Return(occupancyIntoView)); // The occupancy that the VIEW returns.
    // Create a widget factory mock
    auto pMockViewFactory = new MockPeakOverlayFactory;
    PeakOverlayViewFactory_sptr mockViewFactory =
        PeakOverlayViewFactory_sptr(pMockViewFactory);
    EXPECT_CALL(*pMockViewFactory, createView(_, _))
        .WillRepeatedly(Return(mockView));
    EXPECT_CALL(*pMockViewFactory, getPlotXLabel()).WillRepeatedly(Return("H"));
    EXPECT_CALL(*pMockViewFactory, getPlotYLabel()).WillRepeatedly(Return("K"));

    auto presenterBuilder = createStandardBuild(
        nPeaks); // Creates a default Concrete presenter product.
    presenterBuilder.withViewFactory(mockViewFactory); // Change the view
                                                       // factories to deliver
                                                       // the expected mock
                                                       // object
    auto concretePresenter = presenterBuilder.create();

    TS_ASSERT_EQUALS(occupancyIntoView,
                     concretePresenter->getPeakSizeIntoProjection());

    TS_ASSERT(Mock::VerifyAndClearExpectations(pMockView));
  }

  void test_reInitalize() {
    const int nPeaks = 3;

    // Create a mock view object/product that will be returned by the mock
    // factory.

    auto pMockView = new NiceMock<MockPeakOverlayView>;
    auto mockView = boost::shared_ptr<NiceMock<MockPeakOverlayView>>(pMockView);

    // Create a widget factory mock
    auto pMockViewFactory = new MockPeakOverlayFactory;
    PeakOverlayViewFactory_sptr mockViewFactory =
        PeakOverlayViewFactory_sptr(pMockViewFactory);
    EXPECT_CALL(*pMockViewFactory, createView(_, _)).WillOnce(Return(mockView));
    EXPECT_CALL(*pMockViewFactory, getPlotXLabel()).WillRepeatedly(Return("H"));
    EXPECT_CALL(*pMockViewFactory, getPlotYLabel()).WillRepeatedly(Return("K"));

    auto presenterBuilder = createStandardBuild(
        nPeaks); // Creates a default Concrete presenter product.
    presenterBuilder.withViewFactory(mockViewFactory); // Change the view
                                                       // factories to deliver
                                                       // the expected
                                                       // ViewFactory mock
                                                       // object
    auto concretePresenter = presenterBuilder.create();
    presenterBuilder.withViewFactory(mockViewFactory);

    // We now create a new peaks workspace
    const double radius = 1;
    auto newPeaksWorkspace = createPeaksWorkspace(nPeaks + 1, radius);

    // We expect the peaks workspace object to be swapped.
    EXPECT_CALL(*pMockViewFactory, swapPeaksWorkspace(_)).Times(1);
    // We expect that createViews will be called again, because we'll have to
    // create new representations for each peak
    EXPECT_CALL(*pMockViewFactory, createView(_, _)).WillOnce(Return(mockView));

    // We force this concrete presenter to take a new peaks workspace to
    // represent
    concretePresenter->reInitialize(newPeaksWorkspace);

    TS_ASSERT(Mock::VerifyAndClearExpectations(pMockViewFactory));
    TS_ASSERT(Mock::VerifyAndClearExpectations(pMockView));
  }

  void test_contentsDifferent_different() {
    ConcretePeaksPresenter_sptr a = createStandardBuild(2).create();
    ConcretePeaksPresenter_sptr b = createStandardBuild(2).create();

    TSM_ASSERT("Each presenter has it's own unique peaks workspace",
               a->contentsDifferent(b.get()));
    TSM_ASSERT("Each presenter has it's own unique peaks workspace",
               b->contentsDifferent(a.get()));
  }

  void test_contentsDifferent_same() {
    ConcretePeaksPresenterBuilder builder = createStandardBuild();
    // Set a common peaks workspace.
    builder.withPeaksWorkspace(WorkspaceCreationHelper::createPeaksWorkspace());

    ConcretePeaksPresenter_sptr a = builder.create();
    ConcretePeaksPresenter_sptr b = builder.create();

    TSM_ASSERT("Each presenter uses the same peaks workspace",
               !a->contentsDifferent(b.get()));
    TSM_ASSERT("Each presenter uses the same peaks peaks workspace",
               !b->contentsDifferent(a.get()));
  }

  void test_contentsDifferent_mixed() {
    auto a = WorkspaceCreationHelper::createPeaksWorkspace();
    auto b = WorkspaceCreationHelper::createPeaksWorkspace();
    auto c = WorkspaceCreationHelper::createPeaksWorkspace();

    // We are creating another comparison peaks presenter, which will deliver a
    // set of peaks workspaces.
    auto other = boost::make_shared<MockPeaksPresenter>();
    SetPeaksWorkspaces result;
    result.insert(a);
    result.insert(b);
    result.insert(c);
    EXPECT_CALL(*other, presentedWorkspaces()).WillRepeatedly(Return(result));

    ConcretePeaksPresenterBuilder builder = createStandardBuild();
    // Set a peaks workspace.
    builder.withPeaksWorkspace(c);
    ConcretePeaksPresenter_sptr presenter = builder.create();

    TSM_ASSERT("Presenter is managing one of these workspaces already",
               !presenter->contentsDifferent(other.get()));
  }

  void test_test_apply_edit_mode_to_views() {
    const int nPeaks = 1;

    // Create a mock view object/product that will be returned by the mock
    // factory.
    auto pMockView = new NiceMock<MockPeakOverlayView>;
    auto mockView = boost::shared_ptr<NiceMock<MockPeakOverlayView>>(pMockView);

    // Create a widget factory mock
    auto pMockViewFactory = new MockPeakOverlayFactory;
    PeakOverlayViewFactory_sptr mockViewFactory =
        PeakOverlayViewFactory_sptr(pMockViewFactory);
    EXPECT_CALL(*pMockViewFactory, createView(_, _))
        .WillRepeatedly(Return(mockView));
    EXPECT_CALL(*pMockViewFactory, getPlotXLabel()).WillRepeatedly(Return("H"));
    EXPECT_CALL(*pMockViewFactory, getPlotYLabel()).WillRepeatedly(Return("K"));

    auto presenterBuilder = createStandardBuild(
        nPeaks); // Creates a default Concrete presenter product.
    presenterBuilder.withViewFactory(mockViewFactory); // Change the view
                                                       // factories to deliver
                                                       // the expected mock
                                                       // object
    auto concretePresenter = presenterBuilder.create();

    // Deletion mode tests
    EXPECT_CALL(*pMockView, peakDeletionMode()).Times(1);
    concretePresenter->peakEditMode(DeletePeaks);

    // Addition mode tests
    EXPECT_CALL(*pMockView, peakAdditionMode()).Times(1);
    concretePresenter->peakEditMode(AddPeaks);

    // Neutral mode tests
    EXPECT_CALL(*pMockView, peakDisplayMode()).Times(1);
    concretePresenter->peakEditMode(MantidQt::SliceViewer::None);

    TSM_ASSERT("MockView not used as expected.",
               Mock::VerifyAndClearExpectations(pMockView));
  }

  // void test_deletePeaksIn(){TSM_ASSERT("Missing test", false);}

  void makeSphericallyIntegrated(IPeak &peak, const double radius) {
    using namespace Mantid::DataObjects;
    Peak *pPeak = dynamic_cast<Peak *>(&peak);
    pPeak->setPeakShape(
        boost::make_shared<PeakShapeSpherical>(radius, Mantid::Kernel::HKL));
  }

  void test_delete_in() {
    using namespace Mantid::DataObjects;

    const int nPeaks = 3;
    const double radius = 0.1;
    auto concreteBuilder =
        createStandardBuild(nPeaks, radius, Mantid::Kernel::HKL);

    // Custom peaks workspace
    IPeaksWorkspace_sptr peaksWS = createPeaksWorkspace(nPeaks, radius);
    // Customize a peak
    IPeak &peakToDelete1 = peaksWS->getPeak(0);
    peakToDelete1.setHKL(0, 0, 0);
    makeSphericallyIntegrated(peakToDelete1, radius);
    // Customize another peak
    IPeak &peakToDelete2 = peaksWS->getPeak(0);
    peakToDelete2.setHKL(1, 0, 0);
    makeSphericallyIntegrated(peakToDelete2, radius);
    // Customize another peak
    peaksWS->getPeak(2).setHKL(
        10, 10, 10); // Not integrated, will be well out of range anyway.

    /******************

                            x (10,10,10)

    -----------------------   cursor region = 0 to 1 and -0.5 to 0.5
    |  x(0,0,0) x(1,0,0)  |
    -----------------------

    ******************/

    concreteBuilder.withPeaksWorkspace(peaksWS); // Customise builder

    ConcretePeaksPresenter_sptr presenter = concreteBuilder.create();

    /*
     * Create a Cursor region.
     */
    Top top(0.5);
    Bottom bottom(-0.5);
    Left left(0);
    Right right(1);
    SlicePoint slicePoint(0.0);
    PeakBoundingBox cursorRegion(left, right, top, bottom,
                                 slicePoint); // psudo viewing frustrum.

    TSM_ASSERT_EQUALS("No peaks should have been removed yet", 3,
                      peaksWS->getNumberPeaks());
    TSM_ASSERT("Point sits on peak radius. We should delete peak.",
               presenter->deletePeaksIn(cursorRegion));
    TSM_ASSERT_EQUALS("One peaks should remain", 1, peaksWS->getNumberPeaks());
  }
};

#endif
