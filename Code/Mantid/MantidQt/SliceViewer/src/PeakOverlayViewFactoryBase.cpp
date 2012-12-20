#include "MantidQtSliceViewer/PeakOverlayViewFactoryBase.h"
#include "MantidQtSliceViewer/PeakPalette.h"
#include <boost/make_shared.hpp>

using namespace Mantid::API;

namespace MantidQt
{
  namespace SliceViewer
  {

    PeakOverlayViewFactoryBase::PeakOverlayViewFactoryBase(QwtPlot * plot, QWidget * parent, const size_t workspaceNumber) : PeakOverlayViewFactory(), m_plot(plot), m_parent(parent)
    {
      if(!plot)
        throw std::invalid_argument("PeakOverlayViewFactoryBase plot is null");
      if(!parent)
        throw std::invalid_argument("PeakOverlayViewFactoryBase parent widget is null");

      PeakPalette defaultPalette;
      auto colourEnum = defaultPalette.foregroundIndexToColour(static_cast<int>(workspaceNumber));
      //Qt::GlobalColor qtColourEnum = colourEnum;
      m_peakColour = QColor(colourEnum);
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