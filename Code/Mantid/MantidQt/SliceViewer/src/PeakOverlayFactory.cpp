#include "MantidQtSliceViewer/PeakOverlayFactory.h"
#include "MantidQtSliceViewer/PeakOverlay.h"
#include "MantidKernel/V3D.h"
#include "MantidAPI/IPeak.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include <boost/make_shared.hpp>

using namespace Mantid::API;

namespace MantidQt
{
  namespace SliceViewer
  {

    PeakOverlayFactory::PeakOverlayFactory(QwtPlot * plot, QWidget * parent) : PeakOverlayViewFactory(), m_plot(plot), m_parent(parent)
    {
      if(!plot)
        throw std::invalid_argument("PeakOverlayFactory plot is null");
      if(!parent)
        throw std::invalid_argument("PeakOverlayFactory parent widget is null");
    }

    boost::shared_ptr<PeakOverlayView> PeakOverlayFactory::createView(const Mantid::Kernel::V3D& position) const
    {
      return boost::make_shared<PeakOverlay>(m_plot, m_parent, position, this->m_peakRadius);
    }

    std::string PeakOverlayFactory::getPlotXLabel() const
    {
      QwtText xDim = m_plot->axisTitle(QwtPlot::xBottom);
      return xDim.text().toStdString();
    }

    std::string PeakOverlayFactory::getPlotYLabel() const
    {
      QwtText yDim = m_plot->axisTitle(QwtPlot::yLeft);
      return yDim.text().toStdString();
    }

    PeakOverlayFactory::~PeakOverlayFactory()
    {
    }

   /*
    Setter for the actual peak radius. The radius used for drawing will depend on the plane instesection.
    @param peakRadius : Global value for the peak radius to apply to all peaks manufactured through this factory.
    */
    void PeakOverlayFactory::setRadius(const double& peakRadius)
    {
      m_peakRadius = peakRadius;
    }
  }
}