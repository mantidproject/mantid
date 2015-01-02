#ifndef MANTID_SLICEVIEWER_COMPOSITEPEAKSPRESENTER_H_
#define MANTID_SLICEVIEWER_COMPOSITEPEAKSPRESENTER_H_

#include "MantidQtSliceViewer/PeaksPresenter.h"
#include "MantidQtSliceViewer/NullPeaksPresenter.h"
#include "MantidQtSliceViewer/PeakPalette.h"
#include "MantidQtSliceViewer/ZoomablePeaksView.h"
#include "MantidQtSliceViewer/UpdateableOnDemand.h"
#include "MantidQtSliceViewer/ZoomableOnDemand.h"
#include <vector>
#include <stdexcept>
#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>

namespace Mantid
{
namespace API
{
// Forward declaration
class IPeaksWorkspace;
}
}

namespace MantidQt
{
  namespace SliceViewer
  {
    /*---------------------------------------------------------
    CompositePeaksPresenter

    Composite implmentation of the Peaks presenter. Holds 0 - N nested PeaksPresenters.
    Note that it's default behaviour is identical to that of the NullPeaksPresenter.
    ----------------------------------------------------------*/
    class DLLExport CompositePeaksPresenter : public PeaksPresenter, public UpdateableOnDemand, public ZoomableOnDemand
    {
    public:
      
      // Overrriden methods from Peaks Presenter
      virtual void update();
      virtual void updateWithSlicePoint(const PeakBoundingBox&);
      virtual bool changeShownDim();
      virtual bool isLabelOfFreeAxis(const std::string& label) const;
      SetPeaksWorkspaces presentedWorkspaces() const;
      void setForegroundColor(const QColor){/*Do nothing*/}
      void setBackgroundColor(const QColor){/*Do nothing*/}
      void showBackgroundRadius(const bool){/*Do nothing*/}
      void setShown(const bool){/*Do nothing*/}
      virtual PeakBoundingBox getBoundingBox(const int peakIndex) const {return m_default->getBoundingBox(peakIndex);}
      virtual void sortPeaksWorkspace(const std::string&, const bool){ /*Do Nothing*/}
      virtual bool getShowBackground() const {return m_default->getShowBackground();}
      virtual void zoomToPeak(const int) {/* Do nothing */ }
      virtual std::string getTransformName() const;
      virtual bool isHidden() const {return m_default->isHidden();}
      virtual void reInitialize(boost::shared_ptr<Mantid::API::IPeaksWorkspace>& peaksWS) { /*Do nothing*/ }
      
      /// Constructor
      CompositePeaksPresenter(ZoomablePeaksView* const zoomablePlottingWidget,  PeaksPresenter_sptr defaultPresenter = PeaksPresenter_sptr(new NullPeaksPresenter));
      /// Destructor
      virtual ~CompositePeaksPresenter();
      /// Add a peaks presenter onto the composite.
      void addPeaksPresenter(PeaksPresenter_sptr presenter);
      /// Get the number of subjects.
      size_t size() const;
      /// Clear the owned presenters.
      void clear();
      /// Set the peaks size within the current projection
      virtual void setPeakSizeOnProjection(const double fraction);
      /// Set the peaks size into the current projection
      virtual void setPeakSizeIntoProjection(const double fraction);
      /// Get the peaks size onto the current projection
      virtual double getPeakSizeOnProjection() const;
      /// Get the peaks size into the current projection
      virtual double getPeakSizeIntoProjection() const;
      /// Change the foreground representation for the peaks of this workspace
      void setForegroundColour(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws, const QColor);
      /// Change the background representation for the peaks of this workspace
      void setBackgroundColour(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws, const QColor);
      /// Get the foreground colour corresponding to the workspace
      QColor getForegroundColour(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws) const;
      /// Get the background colour corresponding to the workspace
      QColor getBackgroundColour(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws) const;
      /// Determine if the background is shown or not.
      bool getShowBackground(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws) const;
      /// Get a copy of the palette in its current state.
      PeakPalette getPalette() const;
      /// Setter for indicating whether the background radius will be shown.
      void setBackgroundRadiusShown(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws, const bool shown);
      /// Remove the workspace and corresponding presenter.
      void remove(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> peaksWS);
      /// Hide these peaks in the plot.
      void setShown(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> peaksWS, const bool shown);
      /// zoom in on a peak.
      void zoomToPeak(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> peaksWS, const int peakIndex);
      /// Sort the peaks workspace.
      void sortPeaksWorkspace(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> peaksWS, const std::string& columnToSortBy, const bool sortedAscending);
      /// Get the named peaks presenter.
      PeaksPresenter* getPeaksPresenter(const QString& name);
      /// Register any owning presenter
      virtual void registerOwningPresenter(UpdateableOnDemand* owner);
      /// Is the presenter hidden.
      bool getIsHidden(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> peaksWS) const;
      /// Perform update on demand
      virtual void performUpdate();
      /// Zoom to the rectangle
      virtual void zoomToPeak(PeaksPresenter* const presenter, const int peakIndex);
      /// Forget zoom
      void resetZoom();
      /// Get optional zoomed peak presenter.
      boost::optional<PeaksPresenter_sptr> getZoomedPeakPresenter() const;
      /// Get optional zoomed peak index.
      int getZoomedPeakIndex() const;
      /// Make notification that some workspace has been removed.
      void notifyWorkspaceRemoved(const std::string &wsName, Mantid::API::IPeaksWorkspace const * const removedPeaksWS);
      /// Make notification that some workspace has been changed.
      void notifyWorkspaceChanged(const std::string &wsName, boost::shared_ptr<Mantid::API::IPeaksWorkspace>& changedPeaksWS);


    private:
      /// Updateable on demand method.
      void updatePeaksWorkspace(const std::string &toName, boost::shared_ptr<const Mantid::API::IPeaksWorkspace> toWorkspace);
      /// Alias for container of subjects type.
      typedef std::vector<PeaksPresenter_sptr> SubjectContainer;
      /// Subject presenters.
      SubjectContainer m_subjects;
      /// Use default
      bool useDefault() const { return m_subjects.size() == 0; }
      /// Get the presenter for a given workspace.
      SubjectContainer::iterator getPresenterIteratorFromWorkspace(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws);
      /// Get the presenter for a given workspace.
      SubjectContainer::const_iterator getPresenterIteratorFromWorkspace(boost::shared_ptr<const Mantid::API::IPeaksWorkspace> ws) const;
      /// Get the presenter from a workspace name.
      SubjectContainer::iterator getPresenterIteratorFromName(const QString &name);
      /// Colour pallette.
      PeakPalette m_palette;
      /// Zoomable peaks view.
      ZoomablePeaksView* const m_zoomablePlottingWidget;
      /// Default behaviour 
      PeaksPresenter_sptr m_default;
      /// Owning presenter
      UpdateableOnDemand* m_owner;
      /// Presenter zoomed in on.
      boost::optional<PeaksPresenter_sptr> m_zoomedPresenter;
      /// index of peak zoomed in on.
      int m_zoomedPeakIndex;

    };
  }
}

#endif 
