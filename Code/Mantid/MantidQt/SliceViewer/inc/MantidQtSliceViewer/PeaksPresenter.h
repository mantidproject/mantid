#ifndef MANTID_SLICEVIEWER_PEAKSPRESENTER_H_
#define MANTID_SLICEVIEWER_PEAKSPRESENTER_H_

#include "MantidKernel/System.h"
#include <vector>
#include <boost/shared_ptr.hpp>
#include "MantidKernel/System.h"

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
  class PeakOverlayView;

  class DLLExport PeaksPresenter
  {
  public:
    virtual void update() = 0;
    virtual void updateWithSlicePoint(const double& slicePoint) = 0;
  };

  class DLLExport NullPeaksPresenter : public PeaksPresenter
  {
  public:
    virtual void update(){};
    virtual void updateWithSlicePoint(const double& slicePoint)
    {
      UNUSED_ARG(slicePoint);
    };
  };

  class DLLExport ConcretePeaksPresenter : public PeaksPresenter
  {
  public:
    ConcretePeaksPresenter(PeakOverlayViewFactory* factory, boost::shared_ptr<Mantid::API::IPeaksWorkspace> peaksWS);
    virtual void update();
    virtual void updateWithSlicePoint(const double& slicePoint);
  private:
    typedef std::vector< boost::shared_ptr<PeakOverlayView> > VecPeakOverlayView;
    /// Peak overlay views.
    VecPeakOverlayView m_viewPeaks;
  };

  typedef boost::shared_ptr<PeaksPresenter> PeaksPresenter_sptr;
  typedef boost::shared_ptr<const PeaksPresenter> PeaksPresenter_const_sptr;
}
}

#endif /* MANTID_SLICEVIEWER_PEAKSPRESENTER_H_ */