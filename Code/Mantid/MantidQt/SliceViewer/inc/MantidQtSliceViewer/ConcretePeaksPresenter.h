#ifndef MANTID_SLICEVIEWER_CONCRETEPEAKSPRESENTER_H_
#define MANTID_SLICEVIEWER_CONCRETEPEAKSPRESENTER_H_

#include "MantidQtSliceViewer/PeaksPresenter.h"
#include "MantidQtSliceViewer/PeakTransform.h"
#include "MantidKernel/V3D.h"
#include <vector>
#include <boost/shared_ptr.hpp>

namespace MantidQt
{
  namespace SliceViewer
  {
    // Forward declaration.
    class PeakOverlayViewFactory;
    class PeakTransformFactory;

    typedef std::vector< boost::shared_ptr<PeakOverlayView> > VecPeakOverlayView;
    /*---------------------------------------------------------
    ConcretePeaksPresenter

    Concrete implmentation of the Peaks presenter. 
    ----------------------------------------------------------*/
    class DLLExport ConcretePeaksPresenter : public PeaksPresenter
    {
    public:
      ConcretePeaksPresenter(boost::shared_ptr<PeakOverlayViewFactory> viewFactory, boost::shared_ptr<Mantid::API::IPeaksWorkspace> peaksWS, boost::shared_ptr<PeakTransformFactory> transformFactory);
      virtual ~ConcretePeaksPresenter();
      virtual void update();
      virtual void updateWithSlicePoint(const double& slicePoint);
      virtual bool changeShownDim();
      virtual bool isLabelOfFreeAxis(const std::string& label) const;
    private:
      /// Peak overlay views.
      VecPeakOverlayView m_viewPeaks;
      /// View factory
      boost::shared_ptr<PeakOverlayViewFactory> m_viewFactory;
      /// Transform factory
      boost::shared_ptr<PeakTransformFactory> m_transformFactory;
      /// Peak transformer
      PeakTransform_sptr m_transform;
      /// current slicing point.
      double m_slicePoint;
      /// Configurre peak transformations
      bool configureMappingTransform();
      /// Hide all views
      void hideAll();
      /// Show all views
      void showAll();
    };

  }
}

#endif /* MANTID_SLICEVIEWER_CONCRETEPEAKSPRESENTER_H_ */