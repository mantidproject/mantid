#include "MantidQtSliceViewer/CompositePeaksPresenter.h"
#include <stdexcept>

namespace MantidQt
{
  namespace SliceViewer
  {
    /**
    Constructor
    */
    CompositePeaksPresenter::CompositePeaksPresenter(PeaksPresenter_sptr defaultPresenter) : m_default(defaultPresenter)
    {
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
    void CompositePeaksPresenter::updateWithSlicePoint(const double& point)
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
      m_subjects.insert(presenter);
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
    PeaksPresenter_sptr CompositePeaksPresenter::getPresenterFromWorkspace(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws)
    {
      PeaksPresenter_sptr presenterFound = PeaksPresenter_sptr(new NullPeaksPresenter);
      for(auto presenterIterator = m_subjects.begin(); presenterIterator != m_subjects.end(); ++presenterIterator)
      {
        auto workspacesOfSubject = (*presenterIterator)->presentedWorkspaces();
        SetPeaksWorkspaces::iterator iteratorFound =  workspacesOfSubject.find(ws);
        if(iteratorFound != workspacesOfSubject.end())
        {
          presenterFound = *presenterIterator;
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
    void CompositePeaksPresenter::setForegroundColour(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws, Qt::GlobalColor colour)
    {
      getPresenterFromWorkspace(ws)->setForegroundColour(colour);
    }

    /**
    Set the background colour of the peaks.
    @ workspace containing the peaks to re-colour
    @ colour to use for re-colouring
    */
    void CompositePeaksPresenter::setBackgroundColour(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws, Qt::GlobalColor colour)
    {
      getPresenterFromWorkspace(ws)->setBackgroundColour(colour);
    }

    std::string CompositePeaksPresenter::getTransformName() const
    {
      if(useDefault())
      {
        return m_default->getTransformName();
      }
      return (*m_subjects.begin())->getTransformName();
    }
  }
}
