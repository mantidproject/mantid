#ifndef MANTID_SLICEVIEWER_PROXYCOMPOSITEPEAKSPRESENTER_H_
#define MANTID_SLICEVIEWER_PROXYCOMPOSITEPEAKSPRESENTER_H_


#include "MantidKernel/System.h"
#include "MantidQtSliceViewer/CompositePeaksPresenter.h"
#include <boost/shared_ptr.hpp>

namespace MantidQt
{
  namespace SliceViewer
  {
    /*---------------------------------------------------------
    ProxyCompositePeaksPresenter

    Proxy wrapper of the CompositePeaksPresenter. Allows the CompositePeaksPresenter to 
    be used in suituations where diluted power, via a restricted API is required.
    ----------------------------------------------------------*/
    class DLLExport ProxyCompositePeaksPresenter 
    {
    public:

      ProxyCompositePeaksPresenter(boost::shared_ptr<CompositePeaksPresenter> compositePresenter);
      ~ProxyCompositePeaksPresenter();
      size_t size() const;
      void update();

      void setForegroundColour(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws, Qt::GlobalColor);
      /// Change the background representation for the peaks of this workspace
      void setBackgroundColour(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws, Qt::GlobalColor);
      /// Get references to all presented workspaces.
      SetPeaksWorkspaces presentedWorkspaces() const;

    private:
      /// Wrapped composite to delegate to.
      boost::shared_ptr<CompositePeaksPresenter> m_compositePresenter;
    };
  }
}

#endif 