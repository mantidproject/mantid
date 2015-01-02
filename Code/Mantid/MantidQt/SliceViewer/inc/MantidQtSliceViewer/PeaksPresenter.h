#ifndef MANTID_SLICEVIEWER_PEAKSPRESENTER_H_
#define MANTID_SLICEVIEWER_PEAKSPRESENTER_H_

#include "DllOption.h"
#include <boost/shared_ptr.hpp>
#include "MantidQtSliceViewer/PeakPalette.h"
#include "MantidQtSliceViewer/PeakBoundingBox.h"
#include <set>
#include <QObject>

namespace Mantid
{
  namespace API
  {
    // Forward dec.
    class IPeaksWorkspace;
    class PeakTransform;
  }
}

namespace MantidQt
{
namespace SliceViewer
{
  // Forward dec.
  class PeakOverlayViewFactory;

  class PeakOverlayView;
  class UpdateableOnDemand;

  // Alias
  typedef std::set<boost::shared_ptr<const Mantid::API::IPeaksWorkspace> > SetPeaksWorkspaces;

  /*---------------------------------------------------------
  Abstract PeaksPresenter.

  This is abstract to allow usage of the NULL object pattern. This allows the ConcreteViewPresenter to be conctructed in an atomic sense after the constrution of the owning object,
  whithout having to perform fragile null checks.

  ----------------------------------------------------------*/
  class EXPORT_OPT_MANTIDQT_SLICEVIEWER PeaksPresenter: public QObject
  {
  public:
    virtual void update() = 0;
    virtual void updateWithSlicePoint(const PeakBoundingBox&) = 0;
    virtual bool changeShownDim() = 0;
    virtual bool isLabelOfFreeAxis(const std::string& label) const = 0;
    virtual SetPeaksWorkspaces presentedWorkspaces() const = 0;
    virtual void setForegroundColor(const QColor) = 0;
    virtual void setBackgroundColor(const QColor) = 0;
    virtual std::string getTransformName() const = 0;
    virtual void showBackgroundRadius(const bool shown) = 0;
    virtual void setShown(const bool shown) = 0;
    virtual PeakBoundingBox getBoundingBox(const int peakIndex) const = 0;
    virtual void sortPeaksWorkspace(const std::string& byColumnName, const bool ascending) = 0;
    virtual void setPeakSizeOnProjection(const double fraction) = 0;
    virtual void setPeakSizeIntoProjection(const double fraction) = 0;
    virtual double getPeakSizeOnProjection() const = 0;
    virtual double getPeakSizeIntoProjection() const = 0;
    virtual bool getShowBackground() const = 0;
    virtual void registerOwningPresenter(UpdateableOnDemand* owner) = 0;
    virtual QColor getBackgroundColor() const {throw std::runtime_error("PeaksPresenter getBackgroundColour() is not implemented");}
    virtual QColor getForegroundColor() const {throw std::runtime_error("PeaksPresenter getForegroundColour() is not implemented");}
    virtual void zoomToPeak(const int peakIndex) = 0;
    virtual bool isHidden() const = 0;
    virtual ~PeaksPresenter(){};
    virtual void reInitialize(boost::shared_ptr<Mantid::API::IPeaksWorkspace> peaksWS) = 0;
  };


  typedef boost::shared_ptr<PeaksPresenter> PeaksPresenter_sptr;
  typedef boost::shared_ptr<const PeaksPresenter> PeaksPresenter_const_sptr;
}
}

#endif /* MANTID_SLICEVIEWER_PEAKSPRESENTER_H_ */
