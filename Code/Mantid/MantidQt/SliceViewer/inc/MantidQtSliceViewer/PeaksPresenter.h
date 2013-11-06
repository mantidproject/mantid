#ifndef MANTID_SLICEVIEWER_PEAKSPRESENTER_H_
#define MANTID_SLICEVIEWER_PEAKSPRESENTER_H_

#include "MantidKernel/System.h"
#include <boost/shared_ptr.hpp>
#include "MantidKernel/System.h"
#include "MantidQtSliceViewer/PeakPalette.h"
#include "MantidQtSliceViewer/PeakBoundingBox.h"
#include <set>

namespace Mantid
{
  namespace API
  {
    // Forward dec.
    class IPeaksWorkspace;
  }
}

namespace MantidQt
{
namespace SliceViewer
{
  // Forward dec.
  class PeakOverlayViewFactory;
  class PeakTransform;
  class PeakOverlayView;
  class UpdateableOnDemand;

  // Alias
  typedef std::set<boost::shared_ptr<const Mantid::API::IPeaksWorkspace> > SetPeaksWorkspaces;

  /*---------------------------------------------------------
  Abstract PeaksPresenter.

  This is abstract to allow usage of the NULL object pattern. This allows the ConcreteViewPresenter to be conctructed in an atomic sense after the constrution of the owning object,
  whithout having to perform fragile null checks.

  ----------------------------------------------------------*/
  class DLLExport PeaksPresenter
  {
  public:
    virtual void update() = 0;
    virtual void updateWithSlicePoint(const PeakBoundingBox&) = 0;
    virtual bool changeShownDim() = 0;
    virtual bool isLabelOfFreeAxis(const std::string& label) const = 0;
    virtual SetPeaksWorkspaces presentedWorkspaces() const = 0;
    virtual void setForegroundColour(const QColor) = 0;
    virtual void setBackgroundColour(const QColor) = 0;
    virtual std::string getTransformName() const = 0;
    virtual void showBackgroundRadius(const bool shown) = 0;
    virtual void setShown(const bool shown) = 0;
    virtual PeakBoundingBox getBoundingBox(const int peakIndex) const = 0;
    virtual void sortPeaksWorkspace(const std::string& byColumnName, const bool ascending) = 0;
    virtual void setPeakSizeOnProjection(const double fraction) = 0;
    virtual void setPeakSizeIntoProjection(const double fraction) = 0;
    virtual double getPeakSizeOnProjection() const = 0;
    virtual double getPeakSizeIntoProjection() const = 0;
    virtual void registerOwningPresenter(UpdateableOnDemand* owner) = 0;
    virtual ~PeaksPresenter(){};
  };


  typedef boost::shared_ptr<PeaksPresenter> PeaksPresenter_sptr;
  typedef boost::shared_ptr<const PeaksPresenter> PeaksPresenter_const_sptr;
}
}

#endif /* MANTID_SLICEVIEWER_PEAKSPRESENTER_H_ */
