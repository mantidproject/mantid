#ifndef MANTID_SLICEVIEWER_PEAKSPRESENTER_H_
#define MANTID_SLICEVIEWER_PEAKSPRESENTER_H_

#include "MantidKernel/System.h"
#include <boost/shared_ptr.hpp>

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
  class DLLExport PeaksPresenter
  {
  public:
    PeaksPresenter(PeakOverlayViewFactory* factory, boost::shared_ptr<Mantid::API::IPeaksWorkspace> peaksWS);
  };
}
}

#endif /* MANTID_SLICEVIEWER_PEAKSPRESENTER_H_ */