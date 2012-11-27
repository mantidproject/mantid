#include "MantidQtSliceViewer/PeakOverlayFactoryBase.h"
#include "MantidAPI/IPeak.h"
#include <boost/regex.hpp>

namespace MantidQt
{
  namespace SliceViewer
  {

    PeakOverlayFactoryBase::PeakOverlayFactoryBase() : m_peakRadius(1)
    {

    }

    PeakOverlayFactoryBase::~PeakOverlayFactoryBase()
    {
    }

    /*
    Setter for the actual peak radius. The radius used for drawing will depend on the plane instesection.
    @param peakRadius : Global value for the peak radius to apply to all peaks manufactured through this factory.
    */
    void PeakOverlayFactoryBase::setRadius(const double& peakRadius)
    {
      m_peakRadius = peakRadius;
    }

    boost::shared_ptr<PeakOverlayView> PeakOverlayFactoryBase::createView(const Mantid::Kernel::V3D& position) const
    {
      return this->createViewAtPoint(position, m_peakRadius);
    }

  }
}
