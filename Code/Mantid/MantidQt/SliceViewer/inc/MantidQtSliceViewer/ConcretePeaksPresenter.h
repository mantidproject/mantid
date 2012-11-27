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

    typedef std::vector< boost::shared_ptr<PeakOverlayView> > VecPeakOverlayView;
    /*---------------------------------------------------------
    ConcretePeaksPresenter

    Concrete implmentation of the Peaks presenter. 
    ----------------------------------------------------------*/
    class DLLExport ConcretePeaksPresenter : public PeaksPresenter
    {
    public:
      ConcretePeaksPresenter(PeakOverlayViewFactory* factory, boost::shared_ptr<Mantid::API::IPeaksWorkspace> peaksWS);
      virtual ~ConcretePeaksPresenter();
      virtual void update();
      virtual void updateWithSlicePoint(const double& slicePoint);
      virtual void changeShownDim();
    private:
      /// Peak overlay views.
      VecPeakOverlayView m_viewPeaks;
      /// View factory
      boost::shared_ptr<PeakOverlayViewFactory> m_factory;
      /// Peak transformer
      PeakTransform m_transform;
      /// Configurre peak transformations
      void configureMappingTransform();
    };

  }
}

#endif /* MANTID_SLICEVIEWER_CONCRETEPEAKSPRESENTER_H_ */