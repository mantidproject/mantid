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
    , m_factory(factory), m_transform("H", "K"), m_slicePoint(0)
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
    
    const bool transformSucceeded = this->configureMappingTransform();
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
    if(!transformSucceeded)
    {
      hideAll();
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
    if(m_slicePoint != slicePoint) // only update if required.
    {
      for(VecPeakOverlayView::iterator it = m_viewPeaks.begin(); it != m_viewPeaks.end(); ++it)
      {
        (*it)->setSlicePoint(slicePoint);
      }
      m_slicePoint = slicePoint;
    }
  }

  /**
  Destructor. Hide all owned views.
  */
  ConcretePeaksPresenter::~ConcretePeaksPresenter()
  {
    hideAll();
  }

  /**
  Respond to changes in the shown dimension.
  @ return True only if this succeeds. 
  */
  bool ConcretePeaksPresenter::changeShownDim()
  {
    // Reconfigure the mapping tranform.
    const bool transformSucceeded = this->configureMappingTransform();
    // Apply the mapping tranform to move each peak overlay object.

    if(transformSucceeded)
    {
      for(VecPeakOverlayView::iterator it = m_viewPeaks.begin(); it != m_viewPeaks.end(); ++it)
      {
        (*it)->movePosition(m_transform);
      }
    }
    return transformSucceeded;
  }

  /**
  This method looks at the plotted dimensions (XY) , and work out what indexes into the vector HKL, these XYZ dimensions correpond to.
  The indexes can then be used for any future transformation, where the user changes the chosen dimensions to plot.
  @return True if the mapping has succeeded.
  */
  bool ConcretePeaksPresenter::configureMappingTransform()
  {
    bool transformSucceeded = false;
    try
    {
      std::string xLabel = m_factory->getPlotXLabel();
      std::string yLabel = m_factory->getPlotYLabel();
      m_transform = PeakTransformHKL(xLabel, yLabel);
      showAll();
      transformSucceeded = true;
    }
    catch(PeakTransformException&)
    {
      hideAll();
    }
    return transformSucceeded;
  }

  /**
  Determine whether the candidate label is the label of the free axis.
  @param label: The candidate axis label to consider.
  @return True if it matches the label of the free axis accoring to the current peaks transform.
  */
  bool ConcretePeaksPresenter::isLabelOfFreeAxis(const std::string& label) const
  {
    return boost::regex_match(label, m_transform.getFreePeakAxisRegex());
  }

  /**
  Request that each owned view makes iteself visible.
  */
  void ConcretePeaksPresenter::showAll()
  {
    // Show all views.
    for(VecPeakOverlayView::iterator it = m_viewPeaks.begin(); it != m_viewPeaks.end(); ++it)
    {
      if((*it) != NULL)
      {
        (*it)->showView();
      }
    }
  }

  /**
  Request that each owned view makes iteself  NOT visible.
  */
  void ConcretePeaksPresenter::hideAll()
  {
    // Hide all views.
    for(VecPeakOverlayView::iterator it = m_viewPeaks.begin(); it != m_viewPeaks.end(); ++it)
    {
      if((*it) != NULL)
      {
        (*it)->hideView();
      }
    }
  }

}
}