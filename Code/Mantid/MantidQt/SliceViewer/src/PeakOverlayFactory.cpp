#include "MantidQtSliceViewer/PeakOverlayFactory.h"
#include "MantidQtSliceViewer/PeakOverlay.h"

namespace MantidQt
{
  namespace SliceViewer
  {
    
      PeakOverlayFactory::PeakOverlayFactory(QwtPlot * plot, QWidget * parent) : m_plot(plot), m_parent(parent)
      {
        if(!plot)
          throw std::invalid_argument("PeakOverlayFactory plot is null");
        if(!parent)
          throw std::invalid_argument("PeakOverlayFactory parent widget is null");
      }
      
      PeakOverlayView* PeakOverlayFactory::createView(const QPointF& origin, const QPointF& radius) const
      {
        return new PeakOverlay(m_plot, m_parent, origin, radius);
      }

      PeakOverlayFactory::~PeakOverlayFactory()
      {
      }
  }
}