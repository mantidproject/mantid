#ifndef MANTID_SLICEVIEWER_PEAKSPRESENTER_H_
#define MANTID_SLICEVIEWER_PEAKSPRESENTER_H_

#include "DllOption.h"
#include "MantidQtWidgets/SliceViewer/PeakBoundingBox.h"
#include "MantidQtWidgets/SliceViewer/PeakEditMode.h"
#include "MantidQtWidgets/SliceViewer/PeakPalette.h"
#include <boost/shared_ptr.hpp>

#include <QObject>
#include <set>

namespace Mantid {
namespace Kernel {
// Forward dec
class V3D;
} // namespace Kernel

namespace Geometry {
// Forward dec.
class PeakTransform;
} // namespace Geometry
namespace API {
// Forward dec.
class IPeaksWorkspace;
} // namespace API
} // namespace Mantid

namespace MantidQt {
namespace SliceViewer {
// Forward dec.
class PeakOverlayViewFactory;

class PeakOverlayView;
class UpdateableOnDemand;

// Alias
using SetPeaksWorkspaces =
    std::set<boost::shared_ptr<const Mantid::API::IPeaksWorkspace>>;

/*---------------------------------------------------------
Abstract PeaksPresenter.

This is abstract to allow usage of the NULL object pattern. This allows the
ConcreteViewPresenter to be conctructed in an atomic sense after the constrution
of the owning object,
whithout having to perform fragile null checks.

----------------------------------------------------------*/
class EXPORT_OPT_MANTIDQT_SLICEVIEWER PeaksPresenter : public QObject {
public:
  virtual void update() = 0;
  virtual void updateWithSlicePoint(const PeakBoundingBox &) = 0;
  virtual bool changeShownDim() = 0;
  virtual bool isLabelOfFreeAxis(const std::string &label) const = 0;
  virtual SetPeaksWorkspaces presentedWorkspaces() const = 0;
  virtual void setForegroundColor(const PeakViewColor) = 0;
  virtual void setBackgroundColor(const PeakViewColor) = 0;
  virtual std::string getTransformName() const = 0;
  virtual void showBackgroundRadius(const bool shown) = 0;
  virtual void setShown(const bool shown) = 0;
  virtual PeakBoundingBox getBoundingBox(const int peakIndex) const = 0;
  virtual void setPeakSizeOnProjection(const double fraction) = 0;
  virtual void setPeakSizeIntoProjection(const double fraction) = 0;
  virtual double getPeakSizeOnProjection() const = 0;
  virtual double getPeakSizeIntoProjection() const = 0;
  virtual bool getShowBackground() const = 0;
  virtual void registerOwningPresenter(UpdateableOnDemand *owner) = 0;
  virtual PeakViewColor getBackgroundPeakViewColor() const = 0;
  virtual PeakViewColor getForegroundPeakViewColor() const = 0;
  virtual void zoomToPeak(const int peakIndex) = 0;
  virtual bool isHidden() const = 0;
  virtual bool contentsDifferent(PeaksPresenter const *other) const = 0;
  virtual void
  reInitialize(boost::shared_ptr<Mantid::API::IPeaksWorkspace> peaksWS) = 0;
  virtual void peakEditMode(EditMode mode) = 0;
  virtual bool deletePeaksIn(PeakBoundingBox plotCoordsBox) = 0;
  virtual bool addPeakAt(double plotCoordsPointX, double plotCoordsPointY) = 0;
  ~PeaksPresenter() override{};
};

using PeaksPresenter_sptr = boost::shared_ptr<PeaksPresenter>;
using PeaksPresenter_const_sptr = boost::shared_ptr<const PeaksPresenter>;
} // namespace SliceViewer
} // namespace MantidQt

#endif /* MANTID_SLICEVIEWER_PEAKSPRESENTER_H_ */
