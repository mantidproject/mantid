#ifndef MANTID_SLICEVIEWER_PEAKOVERLAY_FACTORY_BASE_H_
#define MANTID_SLICEVIEWER_PEAKOVERLAY_FACTORY_BASE_H_

#include "MantidKernel/System.h"
#include "MantidQtSliceViewer/PeakOverlayViewFactory.h"

namespace Mantid
{
  namespace Kernel
  {
    class V3D;
  }
}

namespace MantidQt
{
  namespace SliceViewer
  {
    class FirstExperimentInfoQuery;
    class DLLExport PeakOverlayFactoryBase : public PeakOverlayViewFactory
    {
    public:
      PeakOverlayFactoryBase(const FirstExperimentInfoQuery& query);
      ~PeakOverlayFactoryBase();
      virtual boost::shared_ptr<PeakOverlayView> createView(const Mantid::API::IPeak&) const;
    protected:
      virtual boost::shared_ptr<PeakOverlayView> createViewAtPoint(const Mantid::Kernel::V3D& position, const double& radius, const bool hasIntensity) const = 0;
    };
  }
}

#endif /* MANTID_SLICEVIEWER_PEAKOVERLAY_FACTORY_BASE_H_ */