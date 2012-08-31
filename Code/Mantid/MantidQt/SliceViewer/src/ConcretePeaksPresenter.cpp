#include "MantidQtSliceViewer/ConcretePeaksPresenter.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/IPeak.h"
#include "MantidQtSliceViewer/PeakOverlayViewFactory.h"
#include <boost/scoped_ptr.hpp>

namespace MantidQt
{
namespace SliceViewer
{

  ConcretePeaksPresenter::ConcretePeaksPresenter(PeakOverlayViewFactory* factory, Mantid::API::IPeaksWorkspace_sptr peaksWS) : m_viewPeaks(peaksWS->getNumberPeaks())
  {
    if(factory == NULL)
    {
      throw std::invalid_argument("PeakOverlayViewFactory is null");
    }

    // Create views for every peak in the workspace.
    boost::scoped_ptr<PeakOverlayViewFactory> factory_scptr(factory);
    double maxIntensity = peaksWS->getPeak(0).getIntensity();
    for(int i = 0; i < peaksWS->getNumberPeaks(); ++i)
    {
      const Mantid::API::IPeak& peak = peaksWS->getPeak(i);
      maxIntensity = peak.getIntensity() > maxIntensity ? peak.getIntensity() : maxIntensity;
      m_viewPeaks[i] = boost::shared_ptr<PeakOverlayView>( factory_scptr->createView(peak) );
    }

    // Set the normalisation. Applies to all peaks with intensity.
    for(VecPeakOverlayView::iterator it = m_viewPeaks.begin(); it != m_viewPeaks.end(); ++it)
    {
      (*it)->setNormalisation(maxIntensity);
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