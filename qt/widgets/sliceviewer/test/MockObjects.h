#ifndef SLICE_VIEWER_MOCKOBJECTS_H_
#define SLICE_VIEWER_MOCKOBJECTS_H_

#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidGeometry/Crystal/IPeak.h"
#include "MantidGeometry/Crystal/PeakTransform.h"
#include "MantidGeometry/Crystal/PeakTransformFactory.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidKernel/UnitLabel.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/SliceViewer/PeakOverlayView.h"
#include "MantidQtWidgets/SliceViewer/PeakOverlayViewFactory.h"
#include "MantidQtWidgets/SliceViewer/PeakViewColor.h"
#include "MantidQtWidgets/SliceViewer/PeaksPresenter.h"
#include "MantidQtWidgets/SliceViewer/UpdateableOnDemand.h"
#include "MantidQtWidgets/SliceViewer/ZoomablePeaksView.h"
#include <QColor>
#include <boost/regex.hpp>
#include <gmock/gmock.h>

using namespace MantidQt::SliceViewer;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid;
using boost::regex;

GCC_DIAG_OFF_SUGGEST_OVERRIDE

namespace {
/*------------------------------------------------------------
Zoomable Peaks View
------------------------------------------------------------*/
class MockZoomablePeaksView : public ZoomablePeaksView {
public:
  MOCK_METHOD1(zoomToRectangle, void(const PeakBoundingBox &));
  MOCK_METHOD0(resetView, void());
  MOCK_METHOD0(detach, void());
  ~MockZoomablePeaksView() override {}
};

/*------------------------------------------------------------
Mock Peaks Presenter
------------------------------------------------------------*/
class MockPeaksPresenter : public PeaksPresenter {
public:
  MOCK_METHOD0(update, void());
  MOCK_METHOD1(updateWithSlicePoint, void(const PeakBoundingBox &));
  MOCK_METHOD2(changeShownDim, bool(size_t, size_t));
  MOCK_CONST_METHOD1(isLabelOfFreeAxis, bool(const std::string &));
  MOCK_CONST_METHOD0(presentedWorkspaces, SetPeaksWorkspaces());
  MOCK_METHOD1(setForegroundColor, void(const PeakViewColor));
  MOCK_METHOD1(setBackgroundColor, void(const PeakViewColor));
  MOCK_CONST_METHOD0(getBackgroundPeakViewColor, PeakViewColor());
  MOCK_CONST_METHOD0(getForegroundPeakViewColor, PeakViewColor());
  MOCK_CONST_METHOD0(getTransformName, std::string());
  MOCK_METHOD1(showBackgroundRadius, void(const bool));
  MOCK_METHOD1(setShown, void(const bool));
  MOCK_CONST_METHOD1(getBoundingBox, PeakBoundingBox(const int peakIndex));
  MOCK_METHOD2(sortPeaksWorkspace, void(const std::string &, const bool));
  MOCK_METHOD1(setPeakSizeOnProjection, void(const double));
  MOCK_METHOD1(setPeakSizeIntoProjection, void(const double));
  MOCK_METHOD1(setNonOrthogonal, void(bool));
  MOCK_CONST_METHOD0(getPeakSizeOnProjection, double());
  MOCK_CONST_METHOD0(getPeakSizeIntoProjection, double());
  MOCK_METHOD1(registerOwningPresenter, void(UpdateableOnDemand *));
  MOCK_CONST_METHOD0(getShowBackground, bool());
  MOCK_METHOD1(zoomToPeak, void(const int));
  MOCK_CONST_METHOD0(isHidden, bool());
  MOCK_METHOD1(reInitialize,
               void(boost::shared_ptr<Mantid::API::IPeaksWorkspace> peaksWS));
  MOCK_CONST_METHOD1(contentsDifferent, bool(const PeaksPresenter *other));
  MOCK_METHOD1(deletePeaksIn, bool(PeakBoundingBox));
  MOCK_METHOD1(peakEditMode, void(EditMode));
  MOCK_METHOD2(addPeakAt, bool(double, double));
  MOCK_CONST_METHOD0(hasPeakAddMode, bool());
  ~MockPeaksPresenter() override {}
};

/*------------------------------------------------------------
Mock Peaks Presenter, with additional hooks for verifying destruction.
------------------------------------------------------------*/
class DyingMockPeaksPresenter : public MockPeaksPresenter {
public:
  MOCK_METHOD0(die, void());
  ~DyingMockPeaksPresenter() override { die(); }
};

/*------------------------------------------------------------
Mock Peak Transform
------------------------------------------------------------*/
class MockPeakTransform : public Geometry::PeakTransform {
public:
  MockPeakTransform()
      : PeakTransform("H (Lattice)", "K (Lattice)", regex("^H.*$"),
                      regex("^K.*$"), regex("^L.*$")) {}
  ~MockPeakTransform() override {}
  MOCK_CONST_METHOD0(clone, PeakTransform_sptr());
  MOCK_CONST_METHOD1(transform,
                     Mantid::Kernel::V3D(const Mantid::Kernel::V3D &));
  MOCK_CONST_METHOD1(transformPeak,
                     Mantid::Kernel::V3D(const Mantid::Geometry::IPeak &));
  MOCK_CONST_METHOD0(getFriendlyName, std::string());
  MOCK_CONST_METHOD0(getCoordinateSystem,
                     Mantid::Kernel::SpecialCoordinateSystem());
};

/*------------------------------------------------------------
Mock Peak Transform Factory
------------------------------------------------------------*/
class MockPeakTransformFactory : public Geometry::PeakTransformFactory {
public:
  MOCK_CONST_METHOD0(createDefaultTransform, PeakTransform_sptr());
  MOCK_CONST_METHOD2(createTransform, PeakTransform_sptr(const std::string &,
                                                         const std::string &));
};

/*------------------------------------------------------------
Mock Peak Overlay View
------------------------------------------------------------*/
class MockPeakOverlayView : public PeakOverlayView {
public:
  MOCK_METHOD1(setPlaneDistance, void(const double &));
  MOCK_METHOD0(updateView, void());
  MOCK_METHOD2(setSlicePoint, void(const double &, const std::vector<bool> &));
  MOCK_METHOD0(hideView, void());
  MOCK_METHOD0(showView, void());
  MOCK_METHOD1(movePosition, void(PeakTransform_sptr));
  MOCK_METHOD2(movePositionNonOrthogonal,
               void(PeakTransform_sptr, NonOrthogonalAxis &));
  MOCK_METHOD1(showBackgroundRadius, void(const bool));
  MOCK_CONST_METHOD1(getBoundingBox, PeakBoundingBox(const int));
  MOCK_METHOD1(changeOccupancyInView, void(const double));
  MOCK_METHOD1(changeOccupancyIntoView, void(const double));
  MOCK_CONST_METHOD0(getOccupancyInView, double());
  MOCK_CONST_METHOD0(getOccupancyIntoView, double());
  MOCK_CONST_METHOD0(positionOnly, bool());
  MOCK_CONST_METHOD0(getRadius, double());
  MOCK_CONST_METHOD0(isBackgroundShown, bool());
  MOCK_METHOD1(changeForegroundColour, void(PeakViewColor));
  MOCK_METHOD1(changeBackgroundColour, void(PeakViewColor));
  MOCK_CONST_METHOD0(getBackgroundPeakViewColor, PeakViewColor());
  MOCK_CONST_METHOD0(getForegroundPeakViewColor, PeakViewColor());
  MOCK_METHOD0(peakDeletionMode, void());
  MOCK_METHOD0(peakAdditionMode, void());
  MOCK_METHOD0(peakDisplayMode, void());
  MOCK_METHOD1(takeSettingsFrom, void(PeakOverlayView const *const));
  ~MockPeakOverlayView() override {}
};

/*------------------------------------------------------------
Mock Widget Factory.
------------------------------------------------------------*/
class MockPeakOverlayFactory : public PeakOverlayViewFactory {
public:
  MOCK_CONST_METHOD2(
      createView, boost::shared_ptr<PeakOverlayView>(PeaksPresenter *,
                                                     PeakTransform_const_sptr));
  MOCK_CONST_METHOD0(getPlotXLabel, std::string());
  MOCK_CONST_METHOD0(getPlotYLabel, std::string());
  MOCK_METHOD0(updateView, void());
  MOCK_METHOD1(swapPeaksWorkspace,
               void(boost::shared_ptr<Mantid::API::IPeaksWorkspace> &));
  MOCK_METHOD1(getNonOrthogonalInfo, void(NonOrthogonalAxis &));
};

/*------------------------------------------------------------
Mock MDGeometry
------------------------------------------------------------*/
class MockMDGeometry : public Mantid::API::MDGeometry {
public:
  MOCK_CONST_METHOD0(getNumDims, size_t());
  MOCK_CONST_METHOD1(
      getDimension,
      boost::shared_ptr<const Mantid::Geometry::IMDDimension>(size_t));
  ~MockMDGeometry() override {}
};

/*------------------------------------------------------------
Mock IMDDimension
------------------------------------------------------------*/
class MockIMDDimension : public Mantid::Geometry::IMDDimension {
public:
  MOCK_CONST_METHOD0(getName, std::string());
  MOCK_CONST_METHOD0(getUnits, const Mantid::Kernel::UnitLabel());
  MOCK_CONST_METHOD0(getMDFrame, const Mantid::Geometry::MDFrame &());
  MOCK_CONST_METHOD0(getMDUnits, const Mantid::Kernel::MDUnit &());
  MOCK_CONST_METHOD0(getDimensionId, const std::string &());
  MOCK_CONST_METHOD0(getMaximum, coord_t());
  MOCK_CONST_METHOD0(getMinimum, coord_t());
  MOCK_CONST_METHOD0(getNBins, size_t());
  MOCK_CONST_METHOD0(getNBoundaries, size_t());
  MOCK_CONST_METHOD0(toXMLString, std::string());
  MOCK_CONST_METHOD0(getIsIntegrated, bool());
  MOCK_CONST_METHOD1(getX, coord_t(size_t ind));
  MOCK_METHOD3(setRange, void(size_t nBins, coord_t min, coord_t max));
};
}

GCC_DIAG_ON_SUGGEST_OVERRIDE

#endif
