#ifndef MANTID_SLICEVIEWER_CONCRETEPEAKSPRESENTER_H_
#define MANTID_SLICEVIEWER_CONCRETEPEAKSPRESENTER_H_

#include "MantidQtSliceViewer/PeaksPresenter.h"
#include "MantidQtSliceViewer/PeakTransform.h"
#include "MantidAPI/MDGeometry.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/Logger.h"
#include <vector>
#include <boost/shared_ptr.hpp>

namespace MantidQt
{
  namespace SliceViewer
  {
    // Forward declarations.
    class PeakOverlayViewFactory;
    class PeakTransformFactory;

    /// Alias for Vector of Peak Overlay Views
    typedef std::vector< boost::shared_ptr<PeakOverlayView> > VecPeakOverlayView;

    /*---------------------------------------------------------
    ConcretePeaksPresenter

    Concrete implmentation of the Peaks presenter. 
    ----------------------------------------------------------*/
    class DLLExport ConcretePeaksPresenter : public PeaksPresenter
    {
    public:
      ConcretePeaksPresenter(boost::shared_ptr<PeakOverlayViewFactory> nonIntegratedViewFactory, boost::shared_ptr<PeakOverlayViewFactory> integratedViewFactory, boost::shared_ptr<Mantid::API::IPeaksWorkspace> peaksWS, boost::shared_ptr<Mantid::API::MDGeometry> mdWS, boost::shared_ptr<PeakTransformFactory> transformFactory);
      virtual ~ConcretePeaksPresenter();
      virtual void update();
      virtual void updateWithSlicePoint(const double& slicePoint);
      virtual bool changeShownDim();
      virtual bool isLabelOfFreeAxis(const std::string& label) const;
      SetPeaksWorkspaces presentedWorkspaces() const;
      void setForegroundColour(const QColor);
      void setBackgroundColour(const QColor);
      std::string getTransformName() const;
      void setShown(const bool shown);
      virtual PeakBoundingBox getBoundingBox(const int) const;
      virtual void sortPeaksWorkspace(const std::string& byColumnName, const bool ascending);
      virtual void setPeakSizeOnProjection(const double fraction);
      virtual void setPeakSizeIntoProjection(const double fraction);
      virtual double getPeakSizeOnProjection() const;
      virtual double getPeakSizeIntoProjection() const;
    private:
      /// Peak overlay views.
      VecPeakOverlayView m_viewPeaks;
      /// View factory
      boost::shared_ptr<PeakOverlayViewFactory> m_viewFactory;
      /// Peaks workspace.
      boost::shared_ptr<const Mantid::API::IPeaksWorkspace> m_peaksWS;
      /// Transform factory
      boost::shared_ptr<PeakTransformFactory> m_transformFactory;
      /// Peak transformer
      PeakTransform_sptr m_transform;
      /// current slicing point.
      double m_slicePoint;
      /// Logger object
      Mantid::Kernel::Logger & g_log;
      /// Configurre peak transformations
      bool configureMappingTransform();
      /// Hide all views
      void hideAll();
      /// Show all views
      void showAll();
      /// Determine wheter a dimension name corresponds to the free axis for the peaks workspace.
      bool isDimensionNameOfFreeAxis(const std::string& name) const;
      /// Switch between showing background radius or not
      void showBackgroundRadius(const bool show);
      /// Produce the views from the PeaksWorkspace
      void produceViews();
      void validateInputs( boost::shared_ptr<PeakOverlayViewFactory> integratedViewFactory,
           boost::shared_ptr<PeakOverlayViewFactory> nonIntegratedViewFactory,
           boost::shared_ptr<const Mantid::API::IPeaksWorkspace> peaksWS);

      void constructViewFactory(boost::shared_ptr<PeakOverlayViewFactory> nonIntegratedViewFactory,
          const boost::shared_ptr<const Mantid::API::MDGeometry> mdWS);
    };

  }
}

#endif /* MANTID_SLICEVIEWER_CONCRETEPEAKSPRESENTER_H_ */
