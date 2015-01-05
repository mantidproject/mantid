#include "MantidQtSliceViewer/PeaksViewer.h"
#include "MantidQtSliceViewer/PeaksWorkspaceWidget.h"
#include "MantidQtSliceViewer/ProxyCompositePeaksPresenter.h"
#include "MantidQtSliceViewer/PeaksTableColumnsDialog.h"
#include <QBoxLayout>
#include <QLayoutItem>

namespace MantidQt
{
  namespace SliceViewer
  {
    /// Constructor
    PeaksViewer::PeaksViewer(QWidget *parent)
      : QWidget(parent)
    {
      this->setMinimumWidth(500);
    }

    void PeaksViewer::setPeaksWorkspaces(const SetPeaksWorkspaces&)
    {
    }

    /**
     * Remove the layout
     * @param widget
     */
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

    /**
     * Set the peaks presenter. This allows the peaks workspace reporting controls to talk to the outside world.
     * @param presenter : Proxy through which all information can be fetched.
     */
    void PeaksViewer::setPresenter(boost::shared_ptr<ProxyCompositePeaksPresenter> presenter) 
    {
      m_presenter = presenter;
      m_presenter->registerView(this);
      
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
        connect(widget, SIGNAL(peaksSorted(const std::string&, const bool, Mantid::API::IPeaksWorkspace_const_sptr)), this, SLOT(onPeaksSorted(const std::string&, const bool, Mantid::API::IPeaksWorkspace_const_sptr)));
        layout()->addWidget(widget);
        ++it;
      }
    }

    /**
     * Hide this view.
     */
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

    /// Destructor
    PeaksViewer::~PeaksViewer()
    {
    }


    /**
     * @brief PeaksViewer::hasThingsToShow
     * @return True if there are workspaces to present.
     */
    bool PeaksViewer::hasThingsToShow() const
    {
        return m_presenter->size() >= 1;
    }

    /**
     * Handler for changing the peak radius colour.
     * @param peaksWS : Peaks workspace to change the foreground colour on.
     * @param newColour : New colour to apply.
     */
    void PeaksViewer::onPeakColourChanged(Mantid::API::IPeaksWorkspace_const_sptr peaksWS, QColor newColour)
    {
      m_presenter->setForegroundColour(peaksWS, newColour);
    }

    /**
     * Handler for Changing the background colour on a peak.
     * @param peaksWS : Peaks workspace to change the background colours on.
     * @param newColour : New colour to apply to the background.
     */
    void PeaksViewer::onBackgroundColourChanged(Mantid::API::IPeaksWorkspace_const_sptr peaksWS, QColor newColour)
    {
      m_presenter->setBackgroundColour(peaksWS, newColour);
    }

    /**
     * Event hander for showing the background radius.
     * @param peaksWS : Workspace to show the background on.
     * @param show : Flag to indicate that the background should be shown/hidden.
     */
    void PeaksViewer::onBackgroundRadiusShown(Mantid::API::IPeaksWorkspace_const_sptr peaksWS, bool show)
    {
      m_presenter->setBackgroundRadiusShown(peaksWS, show);
    }

    /**
     * Event hander for removal of a workspace from the plot.
     * @param peaksWS : Workspace to remove
     */
    void PeaksViewer::onRemoveWorkspace(Mantid::API::IPeaksWorkspace_const_sptr peaksWS)
    {
      this->removePeaksWorkspace(peaksWS);
    }

    /**
     * Event hander for hiding a set of peaks in the plot.
     * @param peaksWS : Peaks workspace to hide.
     * @param hide : boolean toggle for hide/unhide
     */
    void PeaksViewer::onHideInPlot(Mantid::API::IPeaksWorkspace_const_sptr peaksWS, bool hide)
    {
      m_presenter->hideInPlot(peaksWS, hide);
    }

    /**
     * Handler for dealing with zooming actions onto a peak.
     * @param peaksWS : Workspace to zoom in on.
     * @param peakIndex : Index of the peak to zoom in on.
     */
    void PeaksViewer::onZoomToPeak(Mantid::API::IPeaksWorkspace_const_sptr peaksWS, int peakIndex)
    {
      m_presenter->zoomToPeak(peaksWS, peakIndex);
    }

    /**
     * Handler for sorting of a peaks workspace.
     * @param columnToSortBy : Column to sort by
     * @param sortedAscending : Sort direction
     * @param peaksWS : Workspace to be sorted
     */
    void PeaksViewer::onPeaksSorted(const std::string& columnToSortBy, const bool sortedAscending, Mantid::API::IPeaksWorkspace_const_sptr peaksWS)
    {
      m_presenter->sortPeaksWorkspace(peaksWS, columnToSortBy, sortedAscending);
    }

    /**
     * Perform an update based on the proxy composite. Re-fetch data.
     */
    void PeaksViewer::performUpdate()
    {
      auto allWS = m_presenter->presentedWorkspaces();
      for(auto it = allWS.begin(); it != allWS.end(); ++it)
      {
        auto ws = *it;
        QColor backgroundColor = m_presenter->getBackgroundColour(ws);
        QColor foregroundColor = m_presenter->getForegroundColour(ws);
        bool showBackground = m_presenter->getShowBackground(ws);
        bool isHidden = m_presenter->getIsHidden(ws);
        auto optionalZoomedPresenter = m_presenter->getZoomedPeakPresenter();
        int optionalZoomedIndex = m_presenter->getZoomedPeakIndex();

        // Now find the PeaksWorkspaceWidget corresponding to this workspace name.
        QList<PeaksWorkspaceWidget*> children = qFindChildren<PeaksWorkspaceWidget*>(this);
        Mantid::API::IPeaksWorkspace_sptr targetPeaksWorkspace;
        for(int i = 0; i < children.size(); ++i)
        {
          PeaksWorkspaceWidget* candidateWidget = children.at(i);
          Mantid::API::IPeaksWorkspace_const_sptr candidateWorkspace = candidateWidget->getPeaksWorkspace();
          if(candidateWorkspace == ws)
          {
            // We have the right widget to update.
            candidateWidget->setBackgroundColor(backgroundColor);
            candidateWidget->setForegroundColor(foregroundColor);
            candidateWidget->setShowBackground(showBackground);
            candidateWidget->setHidden(isHidden);
            if( optionalZoomedPresenter.is_initialized() )
            {
              // Is the zoomed peaks workspace the current workspace.
              if (optionalZoomedPresenter.get().get() == m_presenter->getPeaksPresenter(ws->name().c_str()))
              {
                candidateWidget->setSelectedPeak(optionalZoomedIndex);
              }
            }
          }
        }
      }
    }

    void PeaksViewer::updatePeaksWorkspace(const std::string &toName, boost::shared_ptr<const Mantid::API::IPeaksWorkspace> toWorkspace)
    {
        /* Any widget with *toWorkspace*  peaks workspace being wrapped. Although, if that's the case, we don't need to perform
         * a replacement, we only need to prompt the widget to update itself around the existing reference.
         *
         * Alternately, if the name is the same, but the workspace has changed, then we need to replace the workspace first.
        */

        // Now find the PeaksWorkspaceWidget corresponding to this workspace name.
        QList<PeaksWorkspaceWidget*> children = qFindChildren<PeaksWorkspaceWidget*>(this);

        for(int i = 0; i < children.size(); ++i)
        {
          PeaksWorkspaceWidget* candidateWidget = children.at(i);
          if(candidateWidget->getWSName() == toWorkspace->getName())
          {
            // We have the right widget to update. Swap the workspace and redraw the table
            candidateWidget->workspaceUpdate(toWorkspace);
            return;
          }
        }
        for(int i = 0; i < children.size(); ++i)
        {
          PeaksWorkspaceWidget* candidateWidget = children.at(i);
          Mantid::API::IPeaksWorkspace_const_sptr candidateWorkspace = candidateWidget->getPeaksWorkspace();
          if(candidateWorkspace == toWorkspace)
          {
            // We have the right widget to update. Workspace is the same, just redraw the table.
            candidateWidget->workspaceUpdate();
            return;
          }
        }

    }

    bool PeaksViewer::removePeaksWorkspace(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> toRemove)
    {
        bool somethingToRemove = false;
        QList<PeaksWorkspaceWidget*> children = qFindChildren<PeaksWorkspaceWidget*>(this);

        for(int i = 0; i < children.size(); ++i)
        {
          PeaksWorkspaceWidget* candidateWidget = children.at(i);
          Mantid::API::IPeaksWorkspace_const_sptr candidateWorkspace = candidateWidget->getPeaksWorkspace();
          somethingToRemove = (candidateWorkspace == toRemove);
          if(somethingToRemove)
          {
            // We have the right widget to update. Workspace is the same, just redraw the table.
            candidateWidget->hide();
            children.removeAt(i);
            break;
          }
        }
        m_presenter->remove(toRemove);
        return somethingToRemove;

    }


    /**
     * Slot called when the user wants to see the dialog for selecting
     * what columns are visible in the tables of peaks.
     */
    void PeaksViewer::showPeaksTableColumnOptions()
    {
      std::set<QString> areShown;

      // get the list of all the columns that are already shown
      QLayout* layout = this->layout();
      const int size = layout->count();
      for(int i = 0; i < size; ++i)
      {
        auto item = layout->itemAt(i);
        if(auto widget = item->widget())
        {
          if (auto table = dynamic_cast<PeaksWorkspaceWidget *>(widget))
          {
            auto shown = table->getShownColumns();
            areShown.insert(shown.begin(), shown.end());
          }
        }
      }

      // show the dialog box
      PeaksTableColumnsDialog dialog(this);
      dialog.setVisibleColumns(areShown);
      dialog.exec();
      auto result = static_cast<QDialog::DialogCode>(dialog.result());
      if (result != QDialog::DialogCode::Accepted)
        return;

      // set what columns to show
      auto toShow = dialog.getVisibleColumns();
      for(int i = 0; i < size; ++i)
      {
        auto item = layout->itemAt(i);
        if(auto widget = item->widget())
        {
          if (auto table = dynamic_cast<PeaksWorkspaceWidget *>(widget))
          {
            table->setShownColumns(toShow);
          }
        }
      }
    }
  } // namespace
}
