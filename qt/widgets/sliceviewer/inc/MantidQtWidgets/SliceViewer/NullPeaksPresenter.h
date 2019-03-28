// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_SLICEVIEWER_NULLPEAKSPRESENTER_H_
#define MANTID_SLICEVIEWER_NULLPEAKSPRESENTER_H_

#include "MantidQtWidgets/SliceViewer/PeakBoundingBox.h"
#include "MantidQtWidgets/SliceViewer/PeaksPresenter.h"

namespace MantidQt {
namespace SliceViewer {
/*---------------------------------------------------------
NullPeaksPresenter

This implementation prevents the client code having to run Null checks on the
PeaksPresenter pointer before using it.
----------------------------------------------------------*/
class EXPORT_OPT_MANTIDQT_SLICEVIEWER NullPeaksPresenter
    : public PeaksPresenter {
public:
  void update() override {}
  void updateWithSlicePoint(const PeakBoundingBox & /*unused*/) override {}
  bool changeShownDim(size_t dimX, size_t dimY) override {
    (void)dimX;
    (void)dimY;
    return false;
  }
  void setNonOrthogonal(bool nonOrthogonalEnabled) override {
    (void)nonOrthogonalEnabled;
  }
  bool isLabelOfFreeAxis(const std::string & /*label*/) const override {
    return false;
  }
  SetPeaksWorkspaces presentedWorkspaces() const override {
    SetPeaksWorkspaces empty;
    return empty;
  }
  void
  setForegroundColor(const PeakViewColor /*unused*/) override { /*Do nothing*/
  }
  void
  setBackgroundColor(const PeakViewColor /*unused*/) override { /*Do nothing*/
  }
  PeakViewColor getBackgroundPeakViewColor() const override {
    return PeakViewColor();
  }
  PeakViewColor getForegroundPeakViewColor() const override {
    return PeakViewColor();
  }
  std::string getTransformName() const override { return ""; }
  void showBackgroundRadius(const bool /*shown*/) override { /*Do nothing*/
  }
  void setShown(const bool /*shown*/) override { /*Do nothing*/
  }
  PeakBoundingBox getBoundingBox(const int /*peakIndex*/) const override {
    return PeakBoundingBox();
  }
  void
  setPeakSizeOnProjection(const double /*fraction*/) override { /*Do Nothing*/
  }
  void
  setPeakSizeIntoProjection(const double /*fraction*/) override { /*Do Nothing*/
  }
  double getPeakSizeOnProjection() const override { return 0; }
  double getPeakSizeIntoProjection() const override { return 0; }
  bool getShowBackground() const override { return false; }
  void registerOwningPresenter(UpdateableOnDemand * /*owner*/) override {}
  void zoomToPeak(const int /*peakIndex*/) override {}
  bool isHidden() const override { return true; }
  void reInitialize(boost::shared_ptr<Mantid::API::IPeaksWorkspace> /*peaksWS*/)
      override { /*Do nothing*/
  }
  bool contentsDifferent(const PeaksPresenter * /*other*/) const override {
    return true;
  }

  void peakEditMode(EditMode /*mode*/) override { /*Do nothing*/
  }
  bool deletePeaksIn(PeakBoundingBox /*plotCoordsBox*/) override {
    return false; /*Do nothing. Delete nothing.*/
  }
  bool addPeakAt(double /*plotCoordsPointX*/,
                 double /*plotCoordsPointY*/) override {
    return false; /*Do nothing. Add nothing.*/
  }
};
} // namespace SliceViewer
} // namespace MantidQt

#endif /* MANTID_SLICEVIEWER_NULLPEAKSPRESENTER_H_ */
