#include "MantidQtSliceViewer/PeakOverlayViewFactorySelector.h"

namespace MantidQt
{
  namespace SliceViewer
  {
      PeakOverlayViewFactorySelector::PeakOverlayViewFactorySelector()
      {

      }

      PeakOverlayViewFactorySelector::~PeakOverlayViewFactorySelector()
      {

      }

      void PeakOverlayViewFactorySelector::registerCandidate(PeakOverlayViewFactory_sptr factory)
      {
        m_candidates.insert(factory);
      }

      PeakOverlayViewFactory_sptr PeakOverlayViewFactorySelector::makeSelection() const
      {
        PeakOverlayViewFactorySet::iterator best = m_candidates.begin();
        int maxFOM = 0;
        for(auto it = m_candidates.begin(); it != m_candidates.end(); ++it)
        {
          int temp = (*it)->FOM();
          if(maxFOM < temp )
          {
            maxFOM = temp;
            best = it;
          }
        }
        if(maxFOM <= 0)
        {
          // We have been unable to find any candidate with a suitable FOM.
          throw std::logic_error("No PeakOverlayViewFactory is capable of rendering this peaks workspace.");
        }
        return *best;
      }

      size_t PeakOverlayViewFactorySelector::countCandidates() const
      {
        return m_candidates.size();
      }

  }
}
