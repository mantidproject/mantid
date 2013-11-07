#include "MantidQtSliceViewer/CompositePeaksPresenter.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include <stdexcept>

namespace MantidQt
{
  namespace SliceViewer
  {
    /**
    Constructor
    */
    CompositePeaksPresenter::CompositePeaksPresenter(ZoomablePeaksView* const zoomablePlottingWidget, PeaksPresenter_sptr defaultPresenter) : m_zoomablePlottingWidget(zoomablePlottingWidget),  
      m_default(defaultPresenter), m_owner(NULL)
    {
      if(m_zoomablePlottingWidget == NULL)
      {
        throw std::runtime_error("Zoomable Plotting Widget is NULL");
      }
    }

    /**
    Destructor
    */
    CompositePeaksPresenter::~CompositePeaksPresenter()
    {
    }

    /**
    Overrriden update method
    */
    void CompositePeaksPresenter::update()
    {
      if(useDefault())
      {
        m_default->update();
        return;
      }
      for(auto it = m_subjects.begin(); it != m_subjects.end(); ++it)
      {
        (*it)->update();
      }
    }

    /**
    Overriden updateWithSlicePoint
    @param point : Slice point to update with 
    */
    void CompositePeaksPresenter::updateWithSlicePoint(const PeakBoundingBox& point)
    {
      if(useDefault())
      {
        m_default->updateWithSlicePoint(point);
        return;
      }
      for(auto it = m_subjects.begin(); it != m_subjects.end(); ++it)
      {
        (*it)->updateWithSlicePoint(point);
      }
    }

    /**
    Handle dimension display changing.
    */
    bool CompositePeaksPresenter::changeShownDim()
    {
      if(useDefault())
      {
        return m_default->changeShownDim();
      }
      bool result = true;
      for(auto it = m_subjects.begin(); it != m_subjects.end(); ++it)
      {
        result &= (*it)->changeShownDim();
      }
      return result;
    }

    /**
    Determine wheter a given axis label correponds to the free peak axis.
    @return True only if the label is that of the free peak axis.
    */
    bool CompositePeaksPresenter::isLabelOfFreeAxis(const std::string& label) const
    {
      if(useDefault())
      {
        return m_default->isLabelOfFreeAxis(label);
      }
      bool result = true;
      for(auto it = m_subjects.begin(); it != m_subjects.end(); ++it)
      {
        result &= (*it)->isLabelOfFreeAxis(label);
      }
      return result;
    }

    /**
    Clear all peaks
    */
    void CompositePeaksPresenter::clear()
    {
      m_subjects.clear();
      PeakPalette temp;
      m_palette = temp;
    }

    /**
    Add peaks presenter
    @param presenter : Subject peaks presenter
    */
    void CompositePeaksPresenter::addPeaksPresenter(PeaksPresenter_sptr presenter)
    {
      if(this->size() == 10)
      {
        throw std::invalid_argument("Maximum number of PeaksWorkspaces that can be simultaneously displayed is 10.");
      }

      auto result_it = std::find(m_subjects.begin(), m_subjects.end(), presenter);
      if(result_it == m_subjects.end())
      {
        m_subjects.push_back(presenter);
        presenter->registerOwningPresenter(this);
      }
    }

    /**
    @return the number of subjects in the composite
    */
    size_t CompositePeaksPresenter::size() const
    {
      return m_subjects.size();
    }

    /**
    Return a collection of all referenced workspaces on demand.
    */
    SetPeaksWorkspaces CompositePeaksPresenter::presentedWorkspaces() const
    {
      SetPeaksWorkspaces allWorkspaces;
      for(auto it = m_subjects.begin(); it != m_subjects.end(); ++it)
      {
        auto workspacesToAppend = (*it)->presentedWorkspaces();
        allWorkspaces.insert(workspacesToAppend.begin(), workspacesToAppend.end());
      }
      return allWorkspaces;
    }

    /**
    @param ws : Peaks Workspace to look for on sub-presenters.
    @return the identified sub-presenter for the workspace, or a NullPeaksPresenter.
    */
    CompositePeaksPresenter::SubjectContainer::iterator CompositePeaksPresenter::getPresenterIteratorFromWorkspace(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws)
    {
      SubjectContainer::iterator presenterFound = m_subjects.end();
      for(auto presenterIterator = m_subjects.begin(); presenterIterator != m_subjects.end(); ++presenterIterator)
      {
        auto workspacesOfSubject = (*presenterIterator)->presentedWorkspaces();
        SetPeaksWorkspaces::iterator iteratorFound =  workspacesOfSubject.find(ws);
        if(iteratorFound != workspacesOfSubject.end())
        {
          presenterFound = presenterIterator;
          break;
        }
      }
      return presenterFound;
    }

    /**
    @param ws : Peaks Workspace to look for on sub-presenters.
    @return the identified sub-presenter for the workspace, or a NullPeaksPresenter.
    */
    CompositePeaksPresenter::SubjectContainer::const_iterator CompositePeaksPresenter::getPresenterIteratorFromWorkspace(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws) const
    {
      SubjectContainer::const_iterator presenterFound = m_subjects.end();
      for(auto presenterIterator = m_subjects.begin(); presenterIterator != m_subjects.end(); ++presenterIterator)
      {
        auto workspacesOfSubject = (*presenterIterator)->presentedWorkspaces();
        SetPeaksWorkspaces::iterator iteratorFound =  workspacesOfSubject.find(ws);
        if(iteratorFound != workspacesOfSubject.end())
        {
          presenterFound = presenterIterator;
          break;
        }
      }
      return presenterFound;
    }

    /**
    Set the foreground colour of the peaks.
    @ workspace containing the peaks to re-colour
    @ colour to use for re-colouring
    */
    void CompositePeaksPresenter::setForegroundColour(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws, const QColor colour)
    {
      SubjectContainer::iterator iterator = getPresenterIteratorFromWorkspace(ws);
      
      // Update the palette the foreground colour
      const int pos = static_cast<int>(std::distance(m_subjects.begin(), iterator));
      m_palette.setForegroundColour(pos, colour);

      // Apply the foreground colour
      (*iterator)->setForegroundColour(colour);
    }

    /**
    Set the background colour of the peaks.
    @ workspace containing the peaks to re-colour
    @ colour to use for re-colouring
    */
    void CompositePeaksPresenter::setBackgroundColour(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws,const  QColor colour)
    {
      SubjectContainer::iterator iterator = getPresenterIteratorFromWorkspace(ws);

      // Update the palette background colour.
      const int pos = static_cast<int>(std::distance(m_subjects.begin(), iterator));
      m_palette.setBackgroundColour(pos, colour);

      // Apply the background colour
      (*iterator)->setBackgroundColour(colour);
    }

    /**
    Getter for the name of the transform.
    @return transform name.
    */
    std::string CompositePeaksPresenter::getTransformName() const
    {
      if(useDefault())
      {
        return m_default->getTransformName();
      }
      return (*m_subjects.begin())->getTransformName();
    }

    /**
    @return a copy of the peaks palette.
    */
    PeakPalette CompositePeaksPresenter::getPalette() const
    {
      return this->m_palette;
    }

    /**
    @param ws: PeakWorkspace to get the colour for.
    @return the foreground colour corresponding to the peaks workspace.
    */
    QColor CompositePeaksPresenter::getForegroundColour(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws) const
    {
      if(useDefault())
      {
        throw std::runtime_error("Foreground colours from palette cannot be fetched until nested presenters are added.");
      }
      SubjectContainer::const_iterator iterator = getPresenterIteratorFromWorkspace(ws);
      const int pos = static_cast<int>(std::distance(m_subjects.begin(), iterator));
      return m_palette.foregroundIndexToColour(pos);
    }

    /**
    @param ws: PeakWorkspace to get the colour for.
    @return the background colour corresponding to the peaks workspace.
    */
    QColor CompositePeaksPresenter::getBackgroundColour(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws) const
    {
      if(useDefault())
      {
        throw std::runtime_error("Background colours from palette cannot be fetched until nested presenters are added.");
      }
      SubjectContainer::const_iterator iterator = getPresenterIteratorFromWorkspace(ws);
      const int pos = static_cast<int>(std::distance(m_subjects.begin(), iterator));
      return m_palette.backgroundIndexToColour(pos);
    }

    /**
     * Set to show the background radius.
     * @param ws : Workspace upon which the backgoround radius should be shown/hidden.
     * @param shown : True to show.
     */
    void CompositePeaksPresenter::setBackgroundRadiusShown(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws, const bool shown)
    {
      if(useDefault())
      {
        return m_default->showBackgroundRadius(shown);
      }
      auto iterator = getPresenterIteratorFromWorkspace(ws);
      (*iterator)->showBackgroundRadius(shown);
    }

    /**
     * Remove a peaks list altogether from the reporting and peaks overlays.
     * @param peaksWS : Peaks list to remove.
     */
    void CompositePeaksPresenter::remove(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> peaksWS)
    {
      if(useDefault())
      {
        return;
      }
      auto iterator = getPresenterIteratorFromWorkspace(peaksWS);
      m_subjects.erase(iterator);
    }

    /**
     * Allow the peaks list to be hidden or visible.
     * @param peaksWS : Peaks list to show/hide.
     * @param shown : True to show.
     */
    void CompositePeaksPresenter::setShown(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> peaksWS, const bool shown)
    {
      if(useDefault())
      {
        return m_default->setShown(shown);
      }
      auto iterator = getPresenterIteratorFromWorkspace(peaksWS);
      (*iterator)->setShown(shown);
    }

    /**
     * Zoom in on a given peak in a given peaks list according to the current viewing dimensions.
     * @param peaksWS : Peaks list from which a choosen peak will be zoomed into.
     * @param peakIndex : Index of the peak in the peaks list to zoom into.
     */
    void CompositePeaksPresenter::zoomToPeak(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> peaksWS, const int peakIndex)
    {
      auto iterator = getPresenterIteratorFromWorkspace(peaksWS);
      auto subjectPresenter = *iterator;
      auto boundingBox = subjectPresenter->getBoundingBox(peakIndex);
      m_zoomablePlottingWidget->zoomToRectangle(boundingBox);
    }

    /**
     * Sort the peaks workspace.
     * @param peaksWS : Peaks list to sort.
     * @param columnToSortBy : Column to sort by.
     * @param sortedAscending : Direction of the sort. True for Ascending.
     */
    void CompositePeaksPresenter::sortPeaksWorkspace(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> peaksWS, const std::string& columnToSortBy, const bool sortedAscending)
    {
      auto iterator = getPresenterIteratorFromWorkspace(peaksWS);
      auto subjectPresenter = *iterator;
      subjectPresenter->sortPeaksWorkspace(columnToSortBy, sortedAscending);
      // We want to zoom out now, because any currently selected peak will be wrong.
      m_zoomablePlottingWidget->resetView();
    }

    /**
     * Set the peaks size on the current projection using the supplied fraction.
     * @param fraction of the view width to use as the peak radius.
     */
    void CompositePeaksPresenter::setPeakSizeOnProjection(const double fraction)
    {
      if(useDefault())
      {
        return m_default->setPeakSizeOnProjection(fraction);
      }
      for(auto presenterIterator = m_subjects.begin(); presenterIterator != m_subjects.end(); ++presenterIterator)
      {
        (*presenterIterator)->setPeakSizeOnProjection(fraction);
      }
    }

    /**
     * Fraction of the z-range to use as the peak radius.
     * @param fraction to use as the peak radius
     */
    void CompositePeaksPresenter::setPeakSizeIntoProjection(const double fraction)
    {
      if (useDefault())
      {
        return m_default->setPeakSizeIntoProjection(fraction);
      }
      for (auto presenterIterator = m_subjects.begin(); presenterIterator != m_subjects.end();
          ++presenterIterator)
      {
        (*presenterIterator)->setPeakSizeIntoProjection(fraction);
      }
    }

    double CompositePeaksPresenter::getPeakSizeOnProjection() const
    {
      if (useDefault())
      {
        return m_default->getPeakSizeOnProjection();
      }
      double result = 0;
      for (auto it = m_subjects.begin(); it != m_subjects.end(); ++it)
      {
        double temp = (*it)->getPeakSizeOnProjection();
        if (temp > 0)
        {
          result = temp;
          break;
        }
      }
      return result;
    }

    double CompositePeaksPresenter::getPeakSizeIntoProjection() const
    {
      if (useDefault())
      {
        return m_default->getPeakSizeIntoProjection();
      }
      double result = 0;
      for (auto it = m_subjects.begin(); it != m_subjects.end(); ++it)
      {
        double temp  = (*it)->getPeakSizeIntoProjection();
        if(temp > 0)
        {
          result = temp;
          break;
        }
      }
      return result;
    }

    bool CompositePeaksPresenter::getShowBackground(
        boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws) const
    {
      if (useDefault())
      {
        throw std::runtime_error(
            "Get show background cannot be fetched until nested presenters are added.");
      }
      SubjectContainer::const_iterator iterator = getPresenterIteratorFromWorkspace(ws);
      return (*iterator)->getShowBackground();
    }

    class MatchWorkspaceName : public std::unary_function<SetPeaksWorkspaces::value_type, bool>
    {
    private:
      const QString m_wsName;
    public:
      MatchWorkspaceName(const QString& name) : m_wsName(name)
      {
      }
      bool operator() (SetPeaksWorkspaces::value_type ws)
      {
        const std::string wsName = ws->name();
        const std::string toMatch = m_wsName.toStdString();
        const bool result = (wsName == toMatch);
        return result;
      }
    };

    PeaksPresenter* CompositePeaksPresenter::getPeaksPresenter(const QString& name)
    {
      MatchWorkspaceName comparitor(name);
      SubjectContainer::iterator presenterFound = m_subjects.end();
      for (auto presenterIterator = m_subjects.begin(); presenterIterator != m_subjects.end();
          ++presenterIterator)
      {
        auto wsOfSubject = (*presenterIterator)->presentedWorkspaces();
        SetPeaksWorkspaces::iterator iteratorFound = std::find_if(wsOfSubject.begin(), wsOfSubject.end(), comparitor);
        if (iteratorFound != wsOfSubject.end())
        {
          presenterFound = presenterIterator;
          break;
        }
      }
      if(presenterFound == m_subjects.end())
      {
        throw std::invalid_argument("Cannot find peaks workspace called :" + name.toStdString());
      }
      return (*presenterFound).get();
    }

    void CompositePeaksPresenter::registerOwningPresenter(UpdateableOnDemand* owner)
    {
      m_owner = owner;
    }

    void CompositePeaksPresenter::performUpdate()
    {
      for (auto presenterIterator = m_subjects.begin(); presenterIterator != m_subjects.end();
          ++presenterIterator)
      {
        auto presenter = (*presenterIterator);
        const int pos = static_cast<int>(std::distance(m_subjects.begin(), presenterIterator));
        m_palette.setBackgroundColour(pos, presenter->getBackgroundColor());
        m_palette.setForegroundColour(pos, presenter->getForegroundColor());

        if (m_owner)
        {
          m_owner->performUpdate();
        }
      }
    }

  }
}
