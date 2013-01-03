#include "MantidQtSliceViewer/PeakOverlaySphereFactory.h"
#include "MantidQtSliceViewer/PeakOverlaySphere.h"
#include <boost/make_shared.hpp>

using namespace Mantid::API;

namespace MantidQt
{
  namespace SliceViewer
  {

    PeakOverlaySphereFactory::PeakOverlaySphereFactory(QwtPlot * plot, QWidget * parent, const size_t colourNumber) : PeakOverlayViewFactoryBase(plot, parent, colourNumber)
    {
    }

    boost::shared_ptr<PeakOverlayView> PeakOverlaySphereFactory::createView(const Mantid::Kernel::V3D& position) const
    {
      return boost::make_shared<PeakOverlaySphere>(m_plot, m_parent, position, this->m_peakRadius, this->m_backgroundInnerRadius, this->m_backgroundOuterRadius, this->m_peakColour, this->m_backColour);
    }

    PeakOverlaySphereFactory::~PeakOverlaySphereFactory()
    {
    }

   /*
    Setter for the actual peak radius. The radius used for drawing will depend on the plane instesection.
    @param peakRadius : Global value for the peak radius to apply to all peaks manufactured through this factory.
    */
    void PeakOverlaySphereFactory::setPeakRadius(const double& peakRadius, const double& backgroundInnerRadius, const double& backgroundOuterRadius)
    {
      m_peakRadius = peakRadius;
      m_backgroundInnerRadius = backgroundInnerRadius;
      m_backgroundOuterRadius  = backgroundOuterRadius;
    }
  }
}