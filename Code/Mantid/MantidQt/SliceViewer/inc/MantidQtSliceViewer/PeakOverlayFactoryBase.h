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
    class DLLExport PeakOverlayFactoryBase : public PeakOverlayViewFactory
    {
    public:
      PeakOverlayFactoryBase();
      ~PeakOverlayFactoryBase();
      void setRadius(const double& radius);
      virtual boost::shared_ptr<PeakOverlayView> createView(const Mantid::Kernel::V3D&) const;
    protected:
      virtual boost::shared_ptr<PeakOverlayView> createViewAtPoint(const Mantid::Kernel::V3D& position, const double& radius) const = 0;
      

    private:
      /// The actual peak radius to use for all peaks created via the factory.
      double m_peakRadius;
    };
  }
}

#endif /* MANTID_SLICEVIEWER_PEAKOVERLAY_FACTORY_BASE_H_ */