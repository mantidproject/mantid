#ifndef SLICE_VIEWER_PEAKOVERLAYVIEWFACTORYSELECTOR_H_
#define SLICE_VIEWER_PEAKOVERLAYVIEWFACTORYSELECTOR_H_

#include "MantidKernel/System.h"
#include "MantidQtSliceViewer/PeakOverlayViewFactory.h"
#include "MantidKernel/ClassMacros.h"
#include <set>

namespace MantidQt
{
  namespace SliceViewer
  {
    class DLLExport PeakOverlayViewFactorySelector
    {
    private:
      typedef std::set<PeakOverlayViewFactory_sptr> PeakOverlayViewFactorySet;
      PeakOverlayViewFactorySet m_candidates;
      DISABLE_COPY_AND_ASSIGN(PeakOverlayViewFactorySelector);
    public:
      PeakOverlayViewFactorySelector();
      ~PeakOverlayViewFactorySelector();
      void registerCandidate(PeakOverlayViewFactory_sptr factory);
      PeakOverlayViewFactory_sptr makeSelection() const;
    };

  }
}

#endif SLICE_VIEWER_PEAKOVERLAYVIEWFACTORYSELECTOR_H_
