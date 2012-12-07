#include "MantidQtSliceViewer/PeakOverlayCrossFactory.h"
#include "MantidQtSliceViewer/PeakOverlayCross.h"
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

    PeakOverlayCrossFactory::PeakOverlayCrossFactory(QwtPlot * plot, QWidget * parent, const size_t colourNumber) : PeakOverlayViewFactoryBase(plot, parent, colourNumber)
    {
    }

    boost::shared_ptr<PeakOverlayView> PeakOverlayCrossFactory::createView(const Mantid::Kernel::V3D& position) const
    {
      return boost::make_shared<PeakOverlayCross>(m_plot, m_parent, position, m_zMax, m_zMin, this->m_peakColour);
    }

    void PeakOverlayCrossFactory::setZRange(const double& max, const double& min)
    {
      m_zMax = max;
      m_zMin = min;
    }

    PeakOverlayCrossFactory::~PeakOverlayCrossFactory()
    {
    }
  }
}