#include "MantidQtSliceViewer/PeakOverlayFactoryBase.h"
#include "MantidQtSliceViewer/FirstExperimentInfoQuery.h"
#include "MantidAPI/IPeak.h"

namespace MantidQt
{
  namespace SliceViewer
  {

    PeakOverlayFactoryBase::PeakOverlayFactoryBase(const FirstExperimentInfoQuery& query)
    {
      m_peakDims = LabView;
      if(query.hasRotatedGoniometer())
      {
        m_peakDims = SampleView;
      }
      if(query.hasOrientedLattice())
      {
        m_peakDims = HKLView;
      }
    }

    PeakOverlayFactoryBase::~PeakOverlayFactoryBase()
    {
    }

    PeakDimensions PeakOverlayFactoryBase::getPeakDimensionality() const 
    {
      return m_peakDims;
    }

    boost::shared_ptr<PeakOverlayView> PeakOverlayFactoryBase::createView(const Mantid::API::IPeak& peak) const
    {
      Mantid::Kernel::V3D position;
      switch(m_peakDims)
      {
      case PeakDimensions::LabView:
        position = peak.getQLabFrame();
        break;
      case PeakDimensions::SampleView:
        position = peak.getQSampleFrame();
        break;
      case PeakDimensions::HKLView:
        position = peak.getHKL();
        break;
      default:
        throw std::runtime_error("Unknown PeakDimension type");
      }

      double intensity = peak.getIntensity(); 

      //QwtText xDim = m_plot->axisTitle(QwtPlot::xBottom);
      //QwtText yDim = m_plot->axisTitle(QwtPlot::yLeft);

      // Does the peak match the regex


      /* 1) Find out which dimensions are being plotted on x and y 
      2) Find out what h, k, l each of these dimensions correspond to.
      3) Create the origin x, y based on these hkl values.
      */

      return this->createViewAtPoint(position, intensity, intensity != 0);
    }

  }
}