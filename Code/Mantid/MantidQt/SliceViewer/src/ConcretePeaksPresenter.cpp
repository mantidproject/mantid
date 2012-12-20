#include "MantidQtSliceViewer/ConcretePeaksPresenter.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/IPeak.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidQtSliceViewer/PeakOverlayViewFactory.h"
#include "MantidQtSliceViewer/PeakTransformFactory.h"
#include <boost/scoped_ptr.hpp>
#include <boost/regex.hpp>

using namespace Mantid::API;
using Mantid::Geometry::IMDDimension_const_sptr;

namespace MantidQt
{
namespace SliceViewer
{
  /**
  Constructor.

  1) First check that the arguments provided are valid.
  2) Then iterate over the MODEL and use it to construct VIEWs via the factory.
  3) A collection of views is stored internally 

  @param nonIntegratedViewFactory : View Factory (THE VIEW via factory)
  @param integratedViewFactory : View Factory (THE VIEW via factory)
  @param peaksWS : IPeaksWorkspace to visualise (THE MODEL)
  @param mdWS : IMDWorkspace also being visualised (THE MODEL)
  @param peaksTransform : Peak Transformation Factory. This is about interpreting the MODEL.
  */
  ConcretePeaksPresenter::ConcretePeaksPresenter(PeakOverlayViewFactory_sptr nonIntegratedViewFactory, PeakOverlayViewFactory_sptr integratedViewFactory, IPeaksWorkspace_sptr peaksWS, boost::shared_ptr<MDGeometry> mdWS, PeakTransformFactory_sptr transformFactory) : m_viewPeaks(peaksWS->getNumberPeaks())
    , m_viewFactory(integratedViewFactory), m_peaksWS(peaksWS), m_transformFactory(transformFactory), m_transform(transformFactory->createDefaultTransform()), m_slicePoint(0)
  {
    if(integratedViewFactory == NULL)
    {
      throw std::invalid_argument("Integrated PeakOverlayViewFactory is null");
    }
    if(nonIntegratedViewFactory == NULL)
    {
      throw std::invalid_argument("NonIntegrated PeakOverlayViewFactory is null");
    }
    if(peaksWS == NULL)
    {
      throw std::invalid_argument("PeaksWorkspace is null");
    }

    double peakIntegrationRadius = 0;
    double maxZ = 0;
    double minZ = 0;
    if(peaksWS->hasIntegratedPeaks())
    {
      peakIntegrationRadius = boost::lexical_cast<double>(peaksWS->run().getProperty("PeakRadius")->value());
    }
    else
    {
      // Swap the view factory. We are not plotting integrated peaks now.
      m_viewFactory.swap(nonIntegratedViewFactory);
      // Find the range for the z slider axis.
      for(size_t dimIndex = 0; dimIndex < mdWS->getNumDims(); ++dimIndex)
      {
        IMDDimension_const_sptr dimensionMappedToZ = mdWS->getDimension(dimIndex);
        if(this->isDimensionNameOfFreeAxis(dimensionMappedToZ->getName()))
        {
          maxZ = dimensionMappedToZ->getMaximum();
          minZ = dimensionMappedToZ->getMinimum();
        }
      } 
    }
    m_viewFactory->setRadius(peakIntegrationRadius);
    m_viewFactory->setZRange(maxZ, minZ);
    
    const bool transformSucceeded = this->configureMappingTransform();
    
    // Make and register each peak widget.
    for(int i = 0; i < peaksWS->getNumberPeaks(); ++i)
    {
      const IPeak& peak = peaksWS->getPeak(i);
      PeakOverlayView_sptr view = boost::shared_ptr<PeakOverlayView>( m_viewFactory->createView(m_transform->transformPeak(peak)) );
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
      std::string xLabel = m_viewFactory->getPlotXLabel();
      std::string yLabel = m_viewFactory->getPlotYLabel();
      auto temp = m_transformFactory->createTransform(xLabel, yLabel);
      m_transform = temp;
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
    return isDimensionNameOfFreeAxis(label);
  }

  /**
  Determine whether the candidate dimension name is the name of the free axis.
  @param name: The candidate dimension name to consider.
  @return True if it matches the label of the free axis accoring to the current peaks transform.
  */
  bool ConcretePeaksPresenter::isDimensionNameOfFreeAxis(const std::string& name) const
  {
    return boost::regex_match(name, m_transform->getFreePeakAxisRegex());
  }

  /**
  Request that each owned view makes its self visible.
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
  Request that each owned view makes its self  NOT visible.
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

  /**
  @return a reference to the held peaks workspace.
  */
  SetPeaksWorkspaces ConcretePeaksPresenter::presentedWorkspaces() const
  {
    // There is only one workspace to return.
    SetPeaksWorkspaces workspaces;
    workspaces.insert(m_peaksWS);
    return workspaces;
  }

  void ConcretePeaksPresenter::setForegroundColour(const Qt::GlobalColor colour)
  {
    // Change foreground colours
    for(VecPeakOverlayView::iterator it = m_viewPeaks.begin(); it != m_viewPeaks.end(); ++it)
    {
      if((*it) != NULL)
      {
        (*it)->changeForegroundColour(colour);
        (*it)->updateView();
      }
    }
  }

  void ConcretePeaksPresenter::setBackgroundColour(const Qt::GlobalColor colour) 
  {
    // Change background colours
    for(VecPeakOverlayView::iterator it = m_viewPeaks.begin(); it != m_viewPeaks.end(); ++it)
    {
      if((*it) != NULL)
      {
        (*it)->changeBackgroundColour(colour);
        (*it)->updateView();
      }
    }
  }

  std::string ConcretePeaksPresenter::getTransformName() const
  {
    return m_transform->getFriendlyName();
  }

}
}