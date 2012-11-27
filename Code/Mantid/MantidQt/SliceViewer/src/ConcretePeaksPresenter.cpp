#include "MantidQtSliceViewer/ConcretePeaksPresenter.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/IPeak.h"
#include "MantidQtSliceViewer/PeakOverlayViewFactory.h"
#include <boost/scoped_ptr.hpp>
#include <boost/regex.hpp>

namespace MantidQt
{
namespace SliceViewer
{

  /**
  Constructor.

  1) First check that the arguments provided are valid.
  2) Then iterate over the MODEL and use it to construct VIEWs via the factory.
  3) A collection of views is stored internally 

  @param factory : View factory (THE VIEW via factory)
  @param peaksWS : IPeaksWorkspace to visualise (THE MODEL)
  */
  ConcretePeaksPresenter::ConcretePeaksPresenter(PeakOverlayViewFactory* factory, Mantid::API::IPeaksWorkspace_sptr peaksWS) : m_viewPeaks(peaksWS->getNumberPeaks())
    , m_factory(factory), m_transform("H", "K")
  {
    if(factory == NULL)
    {
      throw std::invalid_argument("PeakOverlayViewFactory is null");
    }
    if(peaksWS == NULL)
    {
      throw std::invalid_argument("PeaksWorkspace is null");
    }
    if(!peaksWS->hasIntegratedPeaks())
    {
      throw std::invalid_argument("PeaksWorkspace does not contain integrated peaks."); // We might consider drawing these in the future anyway.
    }
    
    this->configureMappingTransform();
    // Extract the integration radius from the workspace.
    const double peakIntegrationRadius = boost::lexical_cast<double>(peaksWS->run().getProperty("PeakRadius")->value());
    factory->setRadius(peakIntegrationRadius);
    for(int i = 0; i < peaksWS->getNumberPeaks(); ++i)
    {
      const Mantid::API::IPeak& peak = peaksWS->getPeak(i);
      auto position = peak.getHKL();
      
      PeakOverlayView_sptr view = boost::shared_ptr<PeakOverlayView>( m_factory->createView(m_transform.transform(position)) );
      m_viewPeaks[i] = view;
    }
  }

  /**
  Force each view to re-paint.
  */
  void ConcretePeaksPresenter::update()
  {
    for(VecPeakOverlayView::iterator it = m_viewPeaks.begin(); it != m_viewPeaks.end(); ++it)
    {
      (*it)->updateView();
    }
  }

  /**
  Allow all view to redraw themselfs following an update to the slice point intersecting plane.
  @param slicePoint : The new slice position (z) against the x-y plot of data.
  */
  void ConcretePeaksPresenter::updateWithSlicePoint(const double& slicePoint)
  {
    for(VecPeakOverlayView::iterator it = m_viewPeaks.begin(); it != m_viewPeaks.end(); ++it)
    {
      (*it)->setSlicePoint(slicePoint);
    }
  }

  /**
  Destructor. Hide all owned views.
  */
  ConcretePeaksPresenter::~ConcretePeaksPresenter()
  {
    for(VecPeakOverlayView::iterator it = m_viewPeaks.begin(); it != m_viewPeaks.end(); ++it)
    {
      (*it)->hideView();
    }
  }

  /**
  Respond to changes in the shown dimension.
  */
  void ConcretePeaksPresenter::changeShownDim()
  {
    // Reconfigure the mapping tranform.
    this->configureMappingTransform();
    // Apply the mapping tranform to move each peak overlay object.

    for(VecPeakOverlayView::iterator it = m_viewPeaks.begin(); it != m_viewPeaks.end(); ++it)
    {
      (*it)->movePosition(m_transform);
    }
  }

  /**

  This method looks at the plotted dimensions (XY) , and work out what indexes into the vector HKL, these XYZ dimensions correpond to.

  The indexes can then be used for any future transformation, where the user changes the chosen dimensions to plot.
  */
  void ConcretePeaksPresenter::configureMappingTransform()
  {
    std::string xLabel = m_factory->getPlotXLabel();
    std::string yLabel = m_factory->getPlotYLabel();

    
  }

}
}