#include "MantidQtSliceViewer/PeakOverlayCrossFactory.h"
#include "MantidQtSliceViewer/PeakOverlaySphere.h"
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
      return boost::make_shared<PeakOverlaySphere>(m_plot, m_parent, position, 0, this->m_peakColour);
    }

    PeakOverlayCrossFactory::~PeakOverlayCrossFactory()
    {
    }
  }
}