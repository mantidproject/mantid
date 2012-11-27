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

    PeakOverlayFactory::PeakOverlayFactory(QwtPlot * plot, QWidget * parent) : PeakOverlayFactoryBase(), m_plot(plot), m_parent(parent)
    {
      if(!plot)
        throw std::invalid_argument("PeakOverlayFactory plot is null");
      if(!parent)
        throw std::invalid_argument("PeakOverlayFactory parent widget is null");
    }

    boost::shared_ptr<PeakOverlayView> PeakOverlayFactory::createViewAtPoint(const Mantid::Kernel::V3D& position, const double& radius) const
    {
      return boost::make_shared<PeakOverlay>(m_plot, m_parent, position, radius);
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
  }
}