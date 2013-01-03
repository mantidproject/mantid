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

      void setForegroundColour(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws, QColor);
      /// Change the background representation for the peaks of this workspace
      void setBackgroundColour(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws, QColor);
      /// Get the foreground colour corresponding to the workspace
      QColor getForegroundColour(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws) const;
      /// Get the background colour corresponding to the workspace
      QColor getBackgroundColour(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws) const;
      /// Get references to all presented workspaces.
      SetPeaksWorkspaces presentedWorkspaces() const;
      /// Gets the transform name.
      std::string getTransformName() const;
      /// Change whether the background radius is shown.
      void setBackgroundRadiusShown(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws, const bool shown);
      /// Remove the workspace and corresponding presenter.
      void remove(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> peaksWS);

    private:
      /// Wrapped composite to delegate to.
      boost::shared_ptr<CompositePeaksPresenter> m_compositePresenter;
    };
  }
}

#endif 