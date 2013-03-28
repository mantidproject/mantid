#include "MantidQtSliceViewer/ConcretePeaksPresenter.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/IPeak.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidKernel/Logger.h"
#include <boost/scoped_ptr.hpp>
#include <boost/regex.hpp>

using namespace Mantid::API;
using Mantid::Geometry::IMDDimension_const_sptr;

namespace MantidQt
{
  namespace SliceViewer
  {

    /**
     * Convert from a SpecialCoordinateSystem enum to a correpsonding enum name.
     * @param coordSystem : enum option
     * @return coordinate system as a string
     */
    std::string coordinateToString(Mantid::API::SpecialCoordinateSystem coordSystem)
    {
      switch(coordSystem)
      {
      case Mantid::API::QLab:
        return "QLab";
      case Mantid::API::QSample:
        return "QSample";
      case Mantid::API::HKL:
        return "HKL";
      default:
        return "Unknown";
      }
    }


    /**
     * Produce the views for the internally held peaks workspace.
     * Indexes to peaks in the peaks workspace are used to reference the corresponding PeaksOverlayView, so when the PeaksWorkspace is reordered,
     * All the views must be recreated.
     */
    void ConcretePeaksPresenter::produceViews()
    {
      // Loop through the peaks workspace, and use the factory to create a view from each peaks.
      for (int i = 0; i < m_peaksWS->getNumberPeaks(); ++i)
      {
        m_viewPeaks[i] = m_viewFactory->createView(i, m_transform);
      }
    }

    /**
     * Check the work-space compatibilities.
     *
     * @param mdWS : MDWorkspace currently plotted.
     */
    void ConcretePeaksPresenter::checkWorkspaceCompatibilities(boost::shared_ptr<Mantid::API::MDGeometry>  mdWS)
    {
      if (auto imdWS = boost::dynamic_pointer_cast<Mantid::API::IMDWorkspace>(mdWS))
      {
        const SpecialCoordinateSystem coordSystMD = imdWS->getSpecialCoordinateSystem();
        const SpecialCoordinateSystem coordSystDim = m_transform->getCoordinateSystem();
        const SpecialCoordinateSystem coordSystPK = m_peaksWS->getSpecialCoordinateSystem();
        // Check that the MDWorkspace is self-consistent.
        if (coordSystMD != coordSystDim)
        {
          std::stringstream ss;
          ss << std::endl;
          ss << "According to the dimension names in your MDWorkspace, this work-space is determined to be in: ";
          ss << m_transform->getFriendlyName() << " in the PeaksViewer. ";
          ss << "However, the MDWorkspace has properties indicating that it's coordinates are in: " << coordinateToString(coordSystMD);
          ss << " To resolve the conflict, the MDWorkspace will be treated as though it has coordinates in: " << m_transform->getFriendlyName();
          g_log.notice(ss.str());
        }
        // If the peaks work-space has been integrated. check cross-work-space compatibility.
        if (coordSystDim != coordSystPK && m_peaksWS->hasIntegratedPeaks())
        {
          std::stringstream ss;
          ss << std::endl;
          ss << "You appear to be plotting your PeaksWorkspace in a different coordinate system from the one in which integration was performed. ";
          ss << "This will distort the integrated peak shape on the PeaksViewer. ";
          ss << "PeaksWorkspace was integrated against a MDWorkspace in the coordinate system: " << coordinateToString(m_peaksWS->getSpecialCoordinateSystem());
          ss << "MDWorkspace is displayed in coordinate system: " << m_transform->getFriendlyName();
          g_log.notice(ss.str());
        }
      }
    }

    /**
     Constructor.

     1 First check that the arguments provided are valid.
     2 Then iterate over the MODEL and use it to construct VIEWs via the factory.
     3 A collection of views is stored internally

     @param viewFactory : View Factory (THE VIEW via factory)
     @param peaksWS : IPeaksWorkspace to visualise (THE MODEL)
     @param mdWS : IMDWorkspace also being visualised (THE MODEL)
     @param transformFactory : Peak Transformation Factory. This is about interpreting the MODEL.
     */
    ConcretePeaksPresenter::ConcretePeaksPresenter(PeakOverlayViewFactory_sptr viewFactory, IPeaksWorkspace_sptr peaksWS,
        boost::shared_ptr<MDGeometry> mdWS, PeakTransformFactory_sptr transformFactory) :
        m_viewPeaks(peaksWS->getNumberPeaks()), m_viewFactory(viewFactory), m_peaksWS(peaksWS), m_transformFactory(
            transformFactory), m_transform(transformFactory->createDefaultTransform()), m_slicePoint(0),
            g_log(Mantid::Kernel::Logger::get("PeaksPresenter"))
    {
      // Check that the workspaces appear to be compatible. Log if otherwise.
      checkWorkspaceCompatibilities(mdWS);

      const bool transformSucceeded = this->configureMappingTransform();

      // Make and register each peak widget.
      produceViews();

      if (!transformSucceeded)
      {
        hideAll();
      }
    }

    /**
     Force each view to re-paint.
     */
    void ConcretePeaksPresenter::update()
    {
      for (VecPeakOverlayView::iterator it = m_viewPeaks.begin(); it != m_viewPeaks.end(); ++it)
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
      if (m_slicePoint != slicePoint) // only update if required.
      {
        for (VecPeakOverlayView::iterator it = m_viewPeaks.begin(); it != m_viewPeaks.end(); ++it)
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

      if (transformSucceeded)
      {
        for (VecPeakOverlayView::iterator it = m_viewPeaks.begin(); it != m_viewPeaks.end(); ++it)
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
      } catch (PeakTransformException&)
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
      for (VecPeakOverlayView::iterator it = m_viewPeaks.begin(); it != m_viewPeaks.end(); ++it)
      {
        if ((*it) != NULL)
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
      for (VecPeakOverlayView::iterator it = m_viewPeaks.begin(); it != m_viewPeaks.end(); ++it)
      {
        if ((*it) != NULL)
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

    void ConcretePeaksPresenter::setForegroundColour(const QColor colour)
    {
      // Change foreground colours
      for (VecPeakOverlayView::iterator it = m_viewPeaks.begin(); it != m_viewPeaks.end(); ++it)
      {
        if ((*it) != NULL)
        {
          (*it)->changeForegroundColour(colour);
          (*it)->updateView();
        }
      }
    }

    void ConcretePeaksPresenter::setBackgroundColour(const QColor colour)
    {
      // Change background colours
      for (VecPeakOverlayView::iterator it = m_viewPeaks.begin(); it != m_viewPeaks.end(); ++it)
      {
        if ((*it) != NULL)
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

    void ConcretePeaksPresenter::showBackgroundRadius(const bool show)
    {
      // Change background colours
      for (VecPeakOverlayView::iterator it = m_viewPeaks.begin(); it != m_viewPeaks.end(); ++it)
      {
        if ((*it) != NULL)
        {
          (*it)->showBackgroundRadius(show);
          (*it)->updateView();
        }
      }
    }

    void ConcretePeaksPresenter::setShown(const bool shown)
    {
      // Change background colours
      for (VecPeakOverlayView::iterator it = m_viewPeaks.begin(); it != m_viewPeaks.end(); ++it)
      {
        PeakOverlayView_sptr view = (*it);
        if ((*it) != NULL)
        {
          if (shown)
          {
            view->showView();
          }
          else
          {
            view->hideView();
          }
          view->updateView();
        }
      }
    }

    /**
     @param peakIndex: index into contained peaks workspace.
     @return the bounding box corresponding to the peakIndex.
     */
    PeakBoundingBox ConcretePeaksPresenter::getBoundingBox(const int peakIndex) const
    {
      if (peakIndex < 0 || peakIndex > static_cast<int>(m_viewPeaks.size()))
      {
        throw std::out_of_range("PeakIndex is out of range");
      }
      return this->m_viewPeaks[peakIndex]->getBoundingBox();
    }

    void ConcretePeaksPresenter::sortPeaksWorkspace(const std::string& byColumnName,
        const bool ascending)
    {
      Mantid::API::IPeaksWorkspace_sptr peaksWS =
          boost::const_pointer_cast<Mantid::API::IPeaksWorkspace>(this->m_peaksWS);

      // Sort the Peaks in-place.
      Mantid::API::IAlgorithm_sptr alg = AlgorithmManager::Instance().create("SortPeaksWorkspace");
      alg->setChild(true);
      alg->setRethrows(true);
      alg->initialize();
      alg->setProperty("InputWorkspace", peaksWS);
      alg->setPropertyValue("OutputWorkspace", "SortedPeaksWorkspace");
      alg->setProperty("OutputWorkspace", peaksWS);
      alg->setProperty("SortAscending", ascending);
      alg->setPropertyValue("ColumnNameToSortBy", byColumnName);
      alg->execute();

      // Reproduce the views.
      this->produceViews();

      // Give the new views the current slice point.
      for (VecPeakOverlayView::iterator it = m_viewPeaks.begin(); it != m_viewPeaks.end(); ++it)
      {
        (*it)->setSlicePoint(this->m_slicePoint);
      }
    }

    void ConcretePeaksPresenter::setPeakSizeOnProjection(const double fraction)
    {
      for (VecPeakOverlayView::iterator it = m_viewPeaks.begin(); it != m_viewPeaks.end(); ++it)
      {
        if ((*it) != NULL)
        {
          (*it)->changeOccupancyInView(fraction);
          (*it)->updateView();
        }
      }
    }

    void ConcretePeaksPresenter::setPeakSizeIntoProjection(const double fraction)
    {
      for (VecPeakOverlayView::iterator it = m_viewPeaks.begin(); it != m_viewPeaks.end(); ++it)
      {
        if ((*it) != NULL)
        {
          (*it)->changeOccupancyIntoView(fraction);
          (*it)->updateView();
        }
      }
    }

    double ConcretePeaksPresenter::getPeakSizeOnProjection() const
    {
      double result = 0;
      for (VecPeakOverlayView::const_iterator it = m_viewPeaks.begin(); it != m_viewPeaks.end(); ++it)
      {
        PeakOverlayView_sptr view = (*it);
        if (view != NULL && view->positionOnly())
        {
          result = m_viewPeaks.front()->getOccupancyInView();
          break;
        }
      }
      return result;
    }

    double ConcretePeaksPresenter::getPeakSizeIntoProjection() const
    {
      double result = 0;
      for (VecPeakOverlayView::const_iterator it = m_viewPeaks.begin(); it != m_viewPeaks.end(); ++it)
      {
        PeakOverlayView_sptr view = (*it);
        if (view != NULL && view->positionOnly())
        {
          result = m_viewPeaks.front()->getOccupancyIntoView();
          break;
        }
      }
      return result;
    }

  }
}


