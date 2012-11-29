#include "MantidQtSliceViewer/CompositePeaksPresenter.h"

namespace MantidQt
{
  namespace SliceViewer
  {
    CompositePeaksPresenter::CompositePeaksPresenter(PeaksPresenter_sptr defaultPresenter) : m_default(defaultPresenter)
    {
      
    }

    CompositePeaksPresenter::~CompositePeaksPresenter()
    {
    }

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

    void CompositePeaksPresenter::clear()
    {
      m_subjects.clear();
    }

    void CompositePeaksPresenter::addPeaksPresenter(PeaksPresenter_sptr presenter)
    {
      m_subjects.insert(presenter);
    }

    size_t CompositePeaksPresenter::size() const
    {
      return m_subjects.size();
    }
}
}
