#ifndef MANTID_SLICEVIEWER_NULLPEAKSPRESENTER_H_
#define MANTID_SLICEVIEWER_NULLPEAKSPRESENTER_H_

#include "MantidQtWidgets/SliceViewer/PeaksPresenter.h"
#include "MantidQtWidgets/SliceViewer/PeakBoundingBox.h"

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
  void updateWithSlicePoint(const PeakBoundingBox &) override {}
  bool changeShownDim() override { return false; }
  bool isLabelOfFreeAxis(const std::string &) const override { return false; }
  SetPeaksWorkspaces presentedWorkspaces() const override {
    SetPeaksWorkspaces empty;
    return empty;
  }
  void setForegroundColor(const PeakViewColor) override { /*Do nothing*/
  }
  void setBackgroundColor(const PeakViewColor) override { /*Do nothing*/
  }
  PeakViewColor getBackgroundPeakViewColor() const override {
    return PeakViewColor();
  }
  PeakViewColor getForegroundPeakViewColor() const override {
    return PeakViewColor();
  }
  std::string getTransformName() const override { return ""; }
  void showBackgroundRadius(const bool) override { /*Do nothing*/
  }
  void setShown(const bool) override { /*Do nothing*/
  }
  PeakBoundingBox getBoundingBox(const int) const override {
    return PeakBoundingBox();
  }
  void sortPeaksWorkspace(const std::string &,
                          const bool) override { /*Do Nothing*/
  }
  void setPeakSizeOnProjection(const double) override { /*Do Nothing*/
  }
  void setPeakSizeIntoProjection(const double) override { /*Do Nothing*/
  }
  double getPeakSizeOnProjection() const override { return 0; }
  double getPeakSizeIntoProjection() const override { return 0; }
  bool getShowBackground() const override { return false; }
  void registerOwningPresenter(UpdateableOnDemand *) override {}
  void zoomToPeak(const int) override {}
  bool isHidden() const override { return true; }
  void reInitialize(
      boost::shared_ptr<Mantid::API::IPeaksWorkspace>) override { /*Do nothing*/
  }
  bool contentsDifferent(const PeaksPresenter *) const override { return true; }

  void peakEditMode(EditMode) override { /*Do nothing*/
  }
  bool deletePeaksIn(PeakBoundingBox) override {
    return false; /*Do nothing. Delete nothing.*/
  }
  bool addPeakAt(double, double) override {
    return false; /*Do nothing. Add nothing.*/
  }
  bool hasPeakAddMode() const override { return false; /*Do nothing.*/ }
};
}
}

#endif /* MANTID_SLICEVIEWER_NULLPEAKSPRESENTER_H_ */
