#ifndef MANTID_SLICEVIEWER_NULLPEAKSPRESENTER_H_
#define MANTID_SLICEVIEWER_NULLPEAKSPRESENTER_H_

#include "MantidQtSliceViewer/PeaksPresenter.h"

namespace MantidQt {
namespace SliceViewer {
/*---------------------------------------------------------
NullPeaksPresenter

This implementation prevents the client code having to run Null checks on the
PeaksPresenter pointer before using it.
----------------------------------------------------------*/
class DLLExport NullPeaksPresenter : public PeaksPresenter {
public:
  virtual void update() {}
  virtual void updateWithSlicePoint(const PeakBoundingBox &) {}
  virtual bool changeShownDim() { return false; }
  virtual bool isLabelOfFreeAxis(const std::string &) const { return false; }
  SetPeaksWorkspaces presentedWorkspaces() const {
    SetPeaksWorkspaces empty;
    return empty;
  }
  void setForegroundColor(const QColor) { /*Do nothing*/
  }
  void setBackgroundColor(const QColor) { /*Do nothing*/
  }
  std::string getTransformName() const { return ""; }
  void showBackgroundRadius(const bool) { /*Do nothing*/
  }
  void setShown(const bool) { /*Do nothing*/
  }
  virtual PeakBoundingBox getBoundingBox(const int) const {
    return PeakBoundingBox();
  }
  virtual void sortPeaksWorkspace(const std::string &,
                                  const bool) { /*Do Nothing*/
  }
  virtual void setPeakSizeOnProjection(const double) { /*Do Nothing*/
  }
  virtual void setPeakSizeIntoProjection(const double) { /*Do Nothing*/
  }
  virtual double getPeakSizeOnProjection() const { return 0; }
  virtual double getPeakSizeIntoProjection() const { return 0; }
  virtual bool getShowBackground() const { return false; }
  virtual void registerOwningPresenter(UpdateableOnDemand *){};
  virtual void zoomToPeak(const int){};
  virtual bool isHidden() const { return true; }
  virtual void reInitialize(
      boost::shared_ptr<Mantid::API::IPeaksWorkspace> ) { /*Do nothing*/
  }
};
}
}

#endif /* MANTID_SLICEVIEWER_NULLPEAKSPRESENTER_H_ */
