#include "MantidQtSliceViewer/PeaksViewer.h"
#include "MantidQtSliceViewer/PeaksWorkspaceWidget.h"
#include "MantidQtSliceViewer/ProxyCompositePeaksPresenter.h"
#include <QBoxLayout>

namespace MantidQt
{
  namespace SliceViewer
  {
    PeaksViewer::PeaksViewer(QWidget *parent)
      : QWidget(parent)
    {
      this->setMinimumWidth(500);
    }

    void PeaksViewer::setPeaksWorkspaces(const SetPeaksWorkspaces& workspaces)
    {
      
    }

    void PeaksViewer::setPresenter(boost::shared_ptr<ProxyCompositePeaksPresenter> presenter) 
    {
      m_presenter = presenter;
      
      // Configure the entire control using the managed workspaces.
      auto workspaces = m_presenter->presentedWorkspaces();

     

      auto _layout = layout();
      if(_layout)
      {
        delete _layout;
      }
      this->setLayout(new QVBoxLayout);
      auto it = workspaces.begin();
      while(it != workspaces.end())
      {
        layout()->addWidget(new PeaksWorkspaceWidget(*it, this));
        ++it;
      }
    }

    PeaksViewer::~PeaksViewer()
    {
    }

  } // namespace
}
