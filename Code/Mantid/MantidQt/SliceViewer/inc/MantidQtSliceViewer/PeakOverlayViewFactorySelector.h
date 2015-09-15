#ifndef SLICE_VIEWER_PEAKOVERLAYVIEWFACTORYSELECTOR_H_
#define SLICE_VIEWER_PEAKOVERLAYVIEWFACTORYSELECTOR_H_

#include "MantidKernel/System.h"
#include "MantidQtSliceViewer/PeakOverlayViewFactory.h"
#include "MantidKernel/ClassMacros.h"
#include <set>
#include <boost/shared_ptr.hpp>

namespace MantidQt
{
  namespace SliceViewer
  {
    /**
     * @class PeakOverlayViewFactorySelector
     * Uses a figure of merit to determine which registered view factory would be
     * best to use.
     */
    class DLLExport PeakOverlayViewFactorySelector
    {
    private:
      typedef std::set<PeakOverlayViewFactory_sptr> PeakOverlayViewFactorySet;
      PeakOverlayViewFactorySet m_candidates;
      DISABLE_COPY_AND_ASSIGN(PeakOverlayViewFactorySelector)
    public:
      PeakOverlayViewFactorySelector();
      ~PeakOverlayViewFactorySelector();
      void registerCandidate(PeakOverlayViewFactory_sptr factory);
      PeakOverlayViewFactory_sptr makeSelection() const;
      size_t countCandidates() const;
    };

    typedef boost::shared_ptr<PeakOverlayViewFactorySelector> PeakOverlayViewFactorySelector_sptr;
    typedef boost::shared_ptr<const PeakOverlayViewFactorySelector> PeakOverlayViewFactorySelector_const_sptr;

  }
}

#endif
