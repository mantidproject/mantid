#include "MantidQtSliceViewer/PeaksViewer.h"
#include "MantidQtSliceViewer/PeaksWorkspaceWidget.h"
#include "MantidQtSliceViewer/ProxyCompositePeaksPresenter.h"
#include <QBoxLayout>
#include <QLayoutItem>

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

    void removeLayout (QWidget* widget)
    {
      QLayout* layout = widget->layout ();
      if (layout != 0)
      {
        QLayoutItem *item;
        while ((item = layout->takeAt(0)) != 0)
          layout->removeItem (item);
        delete layout;
      }
    }

    void PeaksViewer::setPresenter(boost::shared_ptr<ProxyCompositePeaksPresenter> presenter) 
    {
      m_presenter = presenter;
      
      // Configure the entire control using the managed workspaces.
      auto workspaces = m_presenter->presentedWorkspaces();

      auto coordinateSystem = presenter->getTransformName();

      if(layout())
      {
        removeLayout(this);
      }
      this->setLayout(new QVBoxLayout);
      auto it = workspaces.begin();
      while(it != workspaces.end())
      {
        layout()->addWidget(new PeaksWorkspaceWidget(*it, coordinateSystem, this));
        ++it;
      }
    }

    void PeaksViewer::hide() 
    {
      QLayout* layout = this->layout();
      const int size = layout->count();
      for(int i = 0; i < size; ++i)
      {
        auto item = layout->itemAt(i);
        if(auto widget = item->widget())
        {
          widget->hide();
        }
      }
      QWidget::hide();
    }

    PeaksViewer::~PeaksViewer()
    {
    }

  } // namespace
}
