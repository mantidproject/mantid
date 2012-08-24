#include "MantidQtSliceViewer/PeaksPresenter.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/IPeak.h"
#include "MantidQtSliceViewer/PeakOverlayViewFactory.h"
#include <boost/scoped_ptr.hpp>

namespace MantidQt
{
namespace SliceViewer
{
  PeaksPresenter::PeaksPresenter(PeakOverlayViewFactory* factory, Mantid::API::IPeaksWorkspace_sptr peaksWS)
  {
    if(factory == NULL)
    {
      throw std::invalid_argument("PeakOverlayViewFactory is null");
    }
    boost::scoped_ptr<PeakOverlayViewFactory> factory_scptr(factory);
    for(int i = 0; i < peaksWS->getNumberPeaks(); ++i)
    {
      const Mantid::API::IPeak& peak = peaksWS->getPeak(i);
      //peak.getHKL();
      //TODO: translate the peak into an origin & radius.

      factory_scptr->createView(QPointF(0,0), QPointF(1,1));
    }
  }
}
}