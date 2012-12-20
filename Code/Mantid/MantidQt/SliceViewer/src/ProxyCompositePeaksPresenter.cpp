#include "MantidQtSliceViewer/ProxyCompositePeaksPresenter.h"

namespace MantidQt
{
  namespace SliceViewer
  {
    /**
    Constructor
    */
    ProxyCompositePeaksPresenter::ProxyCompositePeaksPresenter(boost::shared_ptr<CompositePeaksPresenter> composite) : m_compositePresenter(composite)
    {
    }

    /**
    Destructor
    */
    ProxyCompositePeaksPresenter::~ProxyCompositePeaksPresenter()
    {
    }

    /**
    Update method
    */
    void ProxyCompositePeaksPresenter::update()
    {
      m_compositePresenter->update();
    }

    /**
    @return the number of subjects in the composite
    */
    size_t ProxyCompositePeaksPresenter::size() const
    {
      return m_compositePresenter->size();
    }

    /**
    Set the foreground colour of the peaks.
    @ workspace containing the peaks to re-colour
    @ colour to use for re-colouring
    */
    void ProxyCompositePeaksPresenter::setForegroundColour(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws, Qt::GlobalColor colour)
    {
      m_compositePresenter->setForegroundColour(ws, colour);
    }

    /**
    Set the background colour of the peaks.
    @ workspace containing the peaks to re-colour
    @ colour to use for re-colouring
    */
    void ProxyCompositePeaksPresenter::setBackgroundColour(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws, Qt::GlobalColor colour)
    {
      m_compositePresenter->setBackgroundColour(ws, colour);
    }

    /*
    Get all the presented workspaces.
    */
    SetPeaksWorkspaces ProxyCompositePeaksPresenter::presentedWorkspaces() const
    {
      return m_compositePresenter->presentedWorkspaces();
    }
  }
}
