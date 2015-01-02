#ifndef SLICE_VIEWER_MOCKOBJECTS_H_
#define SLICE_VIEWER_MOCKOBJECTS_H_

#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/PeakTransform.h"
#include "MantidAPI/PeakTransformFactory.h"
#include "MantidQtSliceViewer/PeaksPresenter.h"
#include "MantidQtSliceViewer/PeakOverlayView.h"
#include "MantidQtSliceViewer/PeakOverlayViewFactory.h"
#include "MantidQtSliceViewer/ZoomablePeaksView.h"
#include "MantidQtSliceViewer/UpdateableOnDemand.h"
#include "MantidAPI/IPeak.h"
#include "MantidKernel/UnitLabel.h"
#include <boost/regex.hpp>
#include <gmock/gmock.h>
#include <QColor>

using namespace MantidQt::SliceViewer;
using namespace Mantid::API;
using namespace Mantid;
using boost::regex;

namespace
{

  /*------------------------------------------------------------
  Zoomable Peaks View
  ------------------------------------------------------------*/
  class MockZoomablePeaksView : public ZoomablePeaksView
  {
  public:
    MOCK_METHOD1(zoomToRectangle, void(const PeakBoundingBox&));
    MOCK_METHOD0(resetView, void());
    virtual ~MockZoomablePeaksView(){}
  };

  /*------------------------------------------------------------
  Mock Peaks Presenter
  ------------------------------------------------------------*/
  class MockPeaksPresenter : public PeaksPresenter
  {
  public:
    MOCK_METHOD0(update, void());
    MOCK_METHOD1(updateWithSlicePoint, void(const PeakBoundingBox&));
    MOCK_METHOD0(changeShownDim, bool());
    MOCK_CONST_METHOD1(isLabelOfFreeAxis, bool(const std::string&));
    MOCK_CONST_METHOD0(presentedWorkspaces, SetPeaksWorkspaces());
    MOCK_METHOD1(setForegroundColor, void(const QColor));
    MOCK_METHOD1(setBackgroundColor, void(const QColor));
    MOCK_CONST_METHOD0(getTransformName, std::string());
    MOCK_METHOD1(showBackgroundRadius, void(const bool));
    MOCK_METHOD1(setShown, void(const bool));
    MOCK_CONST_METHOD1(getBoundingBox, PeakBoundingBox(const int peakIndex));
    MOCK_METHOD2(sortPeaksWorkspace, void(const std::string&, const bool));
    MOCK_METHOD1(setPeakSizeOnProjection, void(const double));
    MOCK_METHOD1(setPeakSizeIntoProjection, void(const double));
    MOCK_CONST_METHOD0(getPeakSizeOnProjection, double());
    MOCK_CONST_METHOD0(getPeakSizeIntoProjection, double());
    MOCK_METHOD1(registerOwningPresenter, void(UpdateableOnDemand*));
    MOCK_CONST_METHOD0(getShowBackground, bool());
    MOCK_METHOD1(zoomToPeak, void(const int));
    MOCK_CONST_METHOD0(isHidden, bool());
    MOCK_METHOD1(reInitialize, void(boost::shared_ptr<Mantid::API::IPeaksWorkspace> peaksWS));
    virtual ~MockPeaksPresenter(){}
  };

  /*------------------------------------------------------------
  Mock Peaks Presenter, with additional hooks for verifying destruction.
  ------------------------------------------------------------*/
  class DyingMockPeaksPresenter : public MockPeaksPresenter
  {
  public:
    MOCK_METHOD0(die, void());
    virtual ~DyingMockPeaksPresenter(){die();}
  };

  /*------------------------------------------------------------
  Mock Peak Transform
  ------------------------------------------------------------*/
  class MockPeakTransform : public PeakTransform 
  {
  public:
    MockPeakTransform()
      :PeakTransform("H (Lattice)", "K (Lattice)", regex("^H.*$"), regex("^K.*$"), regex("^L.*$"))
    {
    }
    ~MockPeakTransform()
    {
    }
    MOCK_CONST_METHOD0(clone, PeakTransform_sptr());
    MOCK_CONST_METHOD1(transform, Mantid::Kernel::V3D(const Mantid::Kernel::V3D&));
    MOCK_CONST_METHOD1(transformPeak, Mantid::Kernel::V3D(const Mantid::API::IPeak&)); 
    MOCK_CONST_METHOD0(getFriendlyName, std::string());
    MOCK_CONST_METHOD0(getCoordinateSystem, Mantid::API::SpecialCoordinateSystem());
  };

  /*------------------------------------------------------------
  Mock Peak Transform Factory
  ------------------------------------------------------------*/
class MockPeakTransformFactory : public PeakTransformFactory 
{
 public:
  MOCK_CONST_METHOD0(createDefaultTransform, PeakTransform_sptr());
  MOCK_CONST_METHOD2(createTransform, PeakTransform_sptr(const std::string&, const std::string&));
};

  /*------------------------------------------------------------
  Mock Peak Overlay View
  ------------------------------------------------------------*/
  class MockPeakOverlayView : public PeakOverlayView
  {
  public:
    MOCK_METHOD1(setPlaneDistance, void(const double&));
    MOCK_METHOD0(updateView, void());
    MOCK_METHOD2(setSlicePoint, void(const double&, const std::vector<bool>&));
    MOCK_METHOD0(hideView, void());
    MOCK_METHOD0(showView, void());
    MOCK_METHOD1(movePosition, void(PeakTransform_sptr));
    MOCK_METHOD1(changeForegroundColour, void(const QColor));
    MOCK_METHOD1(changeBackgroundColour, void(const QColor));
    MOCK_METHOD1(showBackgroundRadius, void(const bool));
    MOCK_CONST_METHOD1(getBoundingBox, PeakBoundingBox(const int));
    MOCK_METHOD1(changeOccupancyInView, void(const double));
    MOCK_METHOD1(changeOccupancyIntoView, void(const double));
    MOCK_CONST_METHOD0(getOccupancyInView, double());
    MOCK_CONST_METHOD0(getOccupancyIntoView, double());
    MOCK_CONST_METHOD0(positionOnly, bool());
    MOCK_CONST_METHOD0(getRadius, double());
    MOCK_CONST_METHOD0(isBackgroundShown, bool());
    MOCK_CONST_METHOD0(getForegroundColour, QColor());
    MOCK_CONST_METHOD0(getBackgroundColour, QColor());
    ~MockPeakOverlayView(){}
  };

  /*------------------------------------------------------------
  Mock Widget Factory.
  ------------------------------------------------------------*/
  class MockPeakOverlayFactory : public PeakOverlayViewFactory
  {
  public:
    MOCK_CONST_METHOD1(createView, boost::shared_ptr<PeakOverlayView>(PeakTransform_const_sptr));
    MOCK_CONST_METHOD0(getPlotXLabel, std::string());
    MOCK_CONST_METHOD0(getPlotYLabel, std::string());
    MOCK_METHOD0(updateView, void());
    MOCK_CONST_METHOD0(FOM, int());
    MOCK_METHOD1(swapPeaksWorkspace, void(boost::shared_ptr<Mantid::API::IPeaksWorkspace>&));
  };
  
  
  /*------------------------------------------------------------
  Mock IPeak
  ------------------------------------------------------------*/
  class MockIPeak : public Mantid::API::IPeak 
  {
  public:
    MOCK_METHOD1(setInstrument,
      void(Geometry::Instrument_const_sptr inst));
    MOCK_CONST_METHOD0(getDetectorID,
      int());
    MOCK_METHOD1(setDetectorID,
      void(int m_DetectorID));
    MOCK_CONST_METHOD0(getDetector,
      Geometry::IDetector_const_sptr());
    MOCK_CONST_METHOD0(getInstrument,
      Geometry::Instrument_const_sptr());
    MOCK_CONST_METHOD0(getRunNumber,
      int());
    MOCK_METHOD1(setRunNumber,
      void(int m_RunNumber));
    MOCK_CONST_METHOD0(getMonitorCount,
      double());
    MOCK_METHOD1(setMonitorCount,
      void(double m_MonitorCount));
    MOCK_CONST_METHOD0(getH,
      double());
    MOCK_CONST_METHOD0(getK,
      double());
    MOCK_CONST_METHOD0(getL,
      double());
    MOCK_CONST_METHOD0(getHKL,
      Mantid::Kernel::V3D());
    MOCK_METHOD1(setH,
      void(double m_H));
    MOCK_METHOD1(setK,
      void(double m_K));
    MOCK_METHOD1(setL,
      void(double m_L));
    MOCK_METHOD3(setHKL,
      void(double H, double K, double L));
    MOCK_METHOD1(setHKL,
      void(Mantid::Kernel::V3D HKL));
    MOCK_CONST_METHOD0(getQLabFrame,
      Mantid::Kernel::V3D());
    MOCK_CONST_METHOD0(getQSampleFrame,
      Mantid::Kernel::V3D());
    MOCK_METHOD0(findDetector,
      bool());
    MOCK_METHOD2(setQSampleFrame,
      void(Mantid::Kernel::V3D QSampleFrame, double detectorDistance));
    MOCK_METHOD2(setQLabFrame,
      void(Mantid::Kernel::V3D QLabFrame, double detectorDistance));
    MOCK_METHOD1(setWavelength,
      void(double wavelength));
    MOCK_CONST_METHOD0(getWavelength,
      double());
    MOCK_CONST_METHOD0(getScattering,
      double());
    MOCK_CONST_METHOD0(getDSpacing,
      double());
    MOCK_CONST_METHOD0(getTOF,
      double());
    MOCK_CONST_METHOD0(getInitialEnergy,
      double());
    MOCK_CONST_METHOD0(getFinalEnergy,
      double());
    MOCK_METHOD1(setInitialEnergy,
      void(double m_InitialEnergy));
    MOCK_METHOD1(setFinalEnergy,
      void(double m_FinalEnergy));
    MOCK_CONST_METHOD0(getIntensity,
      double());
    MOCK_CONST_METHOD0(getSigmaIntensity,
      double());
    MOCK_METHOD1(setIntensity,
      void(double m_Intensity));
    MOCK_METHOD1(setSigmaIntensity,
      void(double m_SigmaIntensity));
    MOCK_CONST_METHOD0(getBinCount,
      double());
    MOCK_METHOD1(setBinCount,
      void(double m_BinCount));
    MOCK_CONST_METHOD0(getGoniometerMatrix,
      Mantid::Kernel::Matrix<double>());
    MOCK_METHOD1(setGoniometerMatrix,
      void(Mantid::Kernel::Matrix<double> m_GoniometerMatrix));
    MOCK_CONST_METHOD0(getBankName,
      std::string());
    MOCK_CONST_METHOD0(getRow,
      int());
    MOCK_CONST_METHOD0(getCol,
      int());
    MOCK_CONST_METHOD0(getDetPos,
      Mantid::Kernel::V3D());
    MOCK_CONST_METHOD0(getL1,
      double());
    MOCK_CONST_METHOD0(getL2,
      double());
    MOCK_CONST_METHOD0(getDetectorPosition,
      Mantid::Kernel::V3D());
    MOCK_CONST_METHOD0(getDetectorPositionNoCheck,
          Mantid::Kernel::V3D());
  };

  /*------------------------------------------------------------
  Mock MDGeometry
  ------------------------------------------------------------*/
  class MockMDGeometry : public Mantid::API::MDGeometry
  {
  public:
    MOCK_CONST_METHOD0(getNumDims, size_t());
    MOCK_CONST_METHOD1(getDimension, boost::shared_ptr<const Mantid::Geometry::IMDDimension>(size_t));
    virtual ~MockMDGeometry() {}
  };

  /*------------------------------------------------------------
  Mock IMDDimension
  ------------------------------------------------------------*/
  class MockIMDDimension : public Mantid::Geometry::IMDDimension 
  {
  public:
    MOCK_CONST_METHOD0(getName,
      std::string());
    MOCK_CONST_METHOD0(getUnits,
      const Mantid::Kernel::UnitLabel());
    MOCK_CONST_METHOD0(getDimensionId,
      std::string());
    MOCK_CONST_METHOD0(getMaximum,
      coord_t());
    MOCK_CONST_METHOD0(getMinimum,
      coord_t());
    MOCK_CONST_METHOD0(getNBins,
      size_t());
    MOCK_CONST_METHOD0(toXMLString,
      std::string());
    MOCK_CONST_METHOD0(getIsIntegrated,
      bool());
    MOCK_CONST_METHOD1(getX,
      coord_t(size_t ind));
    MOCK_METHOD3(setRange,
      void(size_t nBins, coord_t min, coord_t max));
  };

}

#endif
