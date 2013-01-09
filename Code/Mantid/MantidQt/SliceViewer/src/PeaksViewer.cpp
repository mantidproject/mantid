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

    void PeaksViewer::setPeaksWorkspaces(const SetPeaksWorkspaces&)
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
        Mantid::API::IPeaksWorkspace_const_sptr ws = *it;
        auto backgroundColour = m_presenter->getBackgroundColour(ws);
        auto foregroundColour = m_presenter->getForegroundColour(ws);

        auto widget = new PeaksWorkspaceWidget(ws, coordinateSystem, foregroundColour, backgroundColour, this);

        connect(widget, SIGNAL(peakColourChanged(Mantid::API::IPeaksWorkspace_const_sptr, QColor)), this, SLOT(onPeakColourChanged(Mantid::API::IPeaksWorkspace_const_sptr, QColor)));
        connect(widget, SIGNAL(backgroundColourChanged(Mantid::API::IPeaksWorkspace_const_sptr, QColor)), this, SLOT(onBackgroundColourChanged(Mantid::API::IPeaksWorkspace_const_sptr, QColor)));
        connect(widget, SIGNAL(backgroundRadiusShown(Mantid::API::IPeaksWorkspace_const_sptr, bool)), this, SLOT(onBackgroundRadiusShown(Mantid::API::IPeaksWorkspace_const_sptr, bool)));
        connect(widget, SIGNAL(removeWorkspace(Mantid::API::IPeaksWorkspace_const_sptr)), this, SLOT(onRemoveWorkspace(Mantid::API::IPeaksWorkspace_const_sptr)));
        connect(widget, SIGNAL(hideInPlot(Mantid::API::IPeaksWorkspace_const_sptr, bool)), this, SLOT(onHideInPlot(Mantid::API::IPeaksWorkspace_const_sptr, bool)));
        connect(widget, SIGNAL(zoomToPeak(Mantid::API::IPeaksWorkspace_const_sptr, int)), this, SLOT(onZoomToPeak(Mantid::API::IPeaksWorkspace_const_sptr, int)));

        layout()->addWidget(widget);
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
          // This is important, otherwise the removed widgets sit around on the layout.
          widget->hide();
        }
      }
      QWidget::hide();
    }

    PeaksViewer::~PeaksViewer()
    {
    }

    void PeaksViewer::onPeakColourChanged(Mantid::API::IPeaksWorkspace_const_sptr peaksWS, QColor newColour)
    {
      m_presenter->setForegroundColour(peaksWS, newColour);
    }

    void PeaksViewer::onBackgroundColourChanged(Mantid::API::IPeaksWorkspace_const_sptr peaksWS, QColor newColour)
    {
      m_presenter->setBackgroundColour(peaksWS, newColour);
    }

    void PeaksViewer::onBackgroundRadiusShown(Mantid::API::IPeaksWorkspace_const_sptr peaksWS, bool show)
    {
      m_presenter->setBackgroundRadiusShown(peaksWS, show);
    }

    void PeaksViewer::onRemoveWorkspace(Mantid::API::IPeaksWorkspace_const_sptr peaksWS)
    {
      m_presenter->remove(peaksWS);
    }

    void PeaksViewer::onHideInPlot(Mantid::API::IPeaksWorkspace_const_sptr peaksWS, bool hide)
    {
      m_presenter->hideInPlot(peaksWS, hide);
    }

    void PeaksViewer::onZoomToPeak(Mantid::API::IPeaksWorkspace_const_sptr peaksWS, int peakIndex)
    {
      m_presenter->zoomToPeak(peaksWS, peakIndex);
    }

  } // namespace
}
