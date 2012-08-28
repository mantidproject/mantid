#include "MantidQtSliceViewer/PeaksPresenter.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/IPeak.h"
#include "MantidQtSliceViewer/PeakOverlayViewFactory.h"
#include <boost/scoped_ptr.hpp>

namespace MantidQt
{
namespace SliceViewer
{
  ConcretePeaksPresenter::ConcretePeaksPresenter(PeakOverlayViewFactory* factory, Mantid::API::IPeaksWorkspace_sptr peaksWS)
  {
    if(factory == NULL)
    {
      throw std::invalid_argument("PeakOverlayViewFactory is null");
    }

    // Create views for every peak in the workspace.
    boost::scoped_ptr<PeakOverlayViewFactory> factory_scptr(factory);
    for(int i = 0; i < peaksWS->getNumberPeaks(); ++i)
    {
      const Mantid::API::IPeak& peak = peaksWS->getPeak(i);
      auto view = boost::shared_ptr<PeakOverlayView>( factory_scptr->createView(peak) );
      m_viewPeaks.push_back( view );
    }
  }

  void ConcretePeaksPresenter::update()
  {
    for(VecPeakOverlayView::iterator it = m_viewPeaks.begin(); it != m_viewPeaks.end(); ++it)
    {
      (*it)->updateView();
    }
  }

  void ConcretePeaksPresenter::updateWithSlicePoint(const double& slicePoint)
  {
    for(VecPeakOverlayView::iterator it = m_viewPeaks.begin(); it != m_viewPeaks.end(); ++it)
    {
      (*it)->setSlicePoint(slicePoint);
    }
  }

  ConcretePeaksPresenter::~ConcretePeaksPresenter()
  {
    for(VecPeakOverlayView::iterator it = m_viewPeaks.begin(); it != m_viewPeaks.end(); ++it)
    {
      (*it)->hideView();
    }
  }
}
}