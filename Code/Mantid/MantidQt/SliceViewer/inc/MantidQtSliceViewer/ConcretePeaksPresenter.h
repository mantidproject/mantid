#ifndef MANTID_SLICEVIEWER_CONCRETEPEAKSPRESENTER_H_
#define MANTID_SLICEVIEWER_CONCRETEPEAKSPRESENTER_H_

#include "MantidQtSliceViewer/PeaksPresenter.h"
#include <vector>

namespace MantidQt
{
  namespace SliceViewer
  {
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
    private:
      typedef std::vector< boost::shared_ptr<PeakOverlayView> > VecPeakOverlayView;
      /// Peak overlay views.
      VecPeakOverlayView m_viewPeaks;
    };

  }
}

#endif /* MANTID_SLICEVIEWER_CONCRETEPEAKSPRESENTER_H_ */