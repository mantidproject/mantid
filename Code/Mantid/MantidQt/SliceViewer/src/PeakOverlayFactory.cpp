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

    PeakOverlayFactory::PeakOverlayFactory(QwtPlot * plot, QWidget * parent, const size_t colourNumber) : PeakOverlayViewFactory(), m_plot(plot), m_parent(parent)
    {
      if(!plot)
        throw std::invalid_argument("PeakOverlayFactory plot is null");
      if(!parent)
        throw std::invalid_argument("PeakOverlayFactory parent widget is null");

      // Create a colour from the input number. We may want to make this more flexible in the future.
      Qt::GlobalColor colour = Qt::green;
      switch(colourNumber)
      {
      case 0:
        colour = Qt::green;
        break;
      case 1:
        colour = Qt::darkMagenta;
        break;
      case 2:
        colour = Qt::cyan;
        break;
      case 3:
        colour = Qt::darkGreen;
        break;
      case 4:
        colour = Qt::darkCyan;
        break;
      case 5:
        colour = Qt::darkYellow;
        break;
      case 6:
        colour = Qt::darkRed;
        break;
      case 7:
        colour = Qt::black;
        break;
      case 8:
        colour = Qt::white;
        break;
      default:
        colour = Qt::darkGray;
        break;
      }
      m_peakColour = QColor(colour);
    }

    boost::shared_ptr<PeakOverlayView> PeakOverlayFactory::createView(const Mantid::Kernel::V3D& position) const
    {
      return boost::make_shared<PeakOverlay>(m_plot, m_parent, position, this->m_peakRadius, this->m_peakColour);
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