#include "MantidQtSliceViewer/PeakOverlayFactory.h"
#include "MantidQtSliceViewer/PeakOverlay.h"
#include "MantidAPI/IPeak.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/MultipleExperimentInfos.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include <boost/make_shared.hpp>

using namespace Mantid::API;

namespace MantidQt
{
  namespace SliceViewer
  {
    template<typename WS_SPTR_TYPE>
    PeakDimensions establishPeakDimensionality(WS_SPTR_TYPE ws)
    {
      MultipleExperimentInfos_const_sptr expInfos = boost::dynamic_pointer_cast<const MultipleExperimentInfos>(ws);  
      PeakDimensions peakDims = PeakDimensions::LabView;
      if( expInfos != NULL && expInfos->getNumExperimentInfo() > 0)
      {
        Mantid::API::ExperimentInfo_const_sptr expInfo = expInfos->getExperimentInfo(0);
        if(expInfo->run().getGoniometerMatrix().isRotation())
        {
          peakDims = PeakDimensions::SampleView;
          if(expInfo->sample().hasOrientedLattice())
          {
            peakDims = PeakDimensions::HKLView;
          }
        }
      }
      return peakDims;
    }

    PeakOverlayFactory::PeakOverlayFactory(QwtPlot * plot, QWidget * parent, IMDWorkspace_const_sptr ws) : m_plot(plot), m_parent(parent)
    {
      if(!plot)
        throw std::invalid_argument("PeakOverlayFactory plot is null");
      if(!parent)
        throw std::invalid_argument("PeakOverlayFactory parent widget is null");

      IMDEventWorkspace_const_sptr eventWS = boost::dynamic_pointer_cast<const IMDEventWorkspace>(ws);
      if(eventWS != NULL)
      {
        m_peakDims = establishPeakDimensionality(eventWS);
      }
      else
      {
        IMDHistoWorkspace_const_sptr histoWS = boost::dynamic_pointer_cast<const IMDHistoWorkspace>(ws);
        m_peakDims = establishPeakDimensionality(histoWS);
      }
    }

    PeakDimensions PeakOverlayFactory::getPeakDimensions() const
    {
      return this->m_peakDims;
    }

    boost::shared_ptr<PeakOverlayView> PeakOverlayFactory::createView(const Mantid::API::IPeak& peak) const
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

      /* 1) Find out which dimensions are being plotted on x and y 
      2) Find out what h, k, l each of these dimensions correspond to.
      3) Create the origin x, y based on these hkl values.
      */

      return boost::make_shared<PeakOverlay>(m_plot, m_parent, position, intensity);
    }

    PeakOverlayFactory::~PeakOverlayFactory()
    {
    }
  }
}