#include "MantidQtSliceViewer/PeakOverlayFactory.h"
#include "MantidQtSliceViewer/PeakOverlay.h"
#include "MantidAPI/IPeak.h"
#include <boost/make_shared.hpp>

using namespace Mantid::API;

namespace MantidQt
{
  namespace SliceViewer
  {
    
      PeakOverlayFactory::PeakOverlayFactory(QwtPlot * plot, QWidget * parent, const PeakDimensions peakDims) : m_plot(plot), m_parent(parent), m_peakDims(peakDims)
      {
        if(!plot)
          throw std::invalid_argument("PeakOverlayFactory plot is null");
        if(!parent)
          throw std::invalid_argument("PeakOverlayFactory parent widget is null");
      }
      
      boost::shared_ptr<PeakOverlayView> PeakOverlayFactory::createView(const Mantid::API::IPeak& peak) const
      {
        Mantid::Kernel::V3D position;
        switch(m_peakDims)
        {
        case LabView:
          position = peak.getQLabFrame();
          break;
        case SampleView:
          position = peak.getQSampleFrame();
          break;
        case HKLView:
          position = peak.getHKL();
          break;
        default:
          throw std::runtime_error("Unknown PeakDimension type");
        }

        double radius = peak.getIntensity(); //TODO: we should normalise this!
        radius = 1; //HACK
        //QwtText xDim = m_plot->axisTitle(QwtPlot::xBottom);
        //QwtText yDim = m_plot->axisTitle(QwtPlot::yLeft);

        /* 1) Find out which dimensions are being plotted on x and y 
           2) Find out what h, k, l each of these dimensions correspond to.
           3) Create the origin x, y based on these hkl values.
        */

        return boost::make_shared<PeakOverlay>(m_plot, m_parent, position, radius);
      }

      PeakOverlayFactory::~PeakOverlayFactory()
      {
      }
  }
}