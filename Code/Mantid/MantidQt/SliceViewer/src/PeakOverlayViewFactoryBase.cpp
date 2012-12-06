#include "MantidQtSliceViewer/PeakOverlayViewFactoryBase.h"
#include <boost/make_shared.hpp>

using namespace Mantid::API;

namespace MantidQt
{
  namespace SliceViewer
  {

    PeakOverlayViewFactoryBase::PeakOverlayViewFactoryBase(QwtPlot * plot, QWidget * parent, const size_t colourNumber) : PeakOverlayViewFactory(), m_plot(plot), m_parent(parent)
    {
      if(!plot)
        throw std::invalid_argument("PeakOverlayViewFactoryBase plot is null");
      if(!parent)
        throw std::invalid_argument("PeakOverlayViewFactoryBase parent widget is null");

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

    std::string PeakOverlayViewFactoryBase::getPlotXLabel() const
    {
      QwtText xDim = m_plot->axisTitle(QwtPlot::xBottom);
      return xDim.text().toStdString();
    }

    std::string PeakOverlayViewFactoryBase::getPlotYLabel() const
    {
      QwtText yDim = m_plot->axisTitle(QwtPlot::yLeft);
      return yDim.text().toStdString();
    }

    PeakOverlayViewFactoryBase::~PeakOverlayViewFactoryBase()
    {
    }
  }
}