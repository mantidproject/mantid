#include "MantidQtSliceViewer/PeakOverlayFactoryBase.h"
#include "MantidQtSliceViewer/FirstExperimentInfoQuery.h"
#include "MantidAPI/IPeak.h"

namespace MantidQt
{
  namespace SliceViewer
  {

    PeakOverlayFactoryBase::PeakOverlayFactoryBase(const FirstExperimentInfoQuery& query) : m_peakRadius(1)
    {
      if(!query.hasOrientedLattice())
      {
        throw std::invalid_argument("Input MDWorkspace must be in QSpace HKL and must have an oriented lattice.");
      }
    }

    PeakOverlayFactoryBase::~PeakOverlayFactoryBase()
    {
    }

    /*
    Setter for the actual peak radius. The radius used for drawing will depend on the plane instesection.
    @param peakRadius : Global value for the peak radius to apply to all peaks manufactured through this factory.
    */
    void PeakOverlayFactoryBase::setRadius(const double& peakRadius)
    {
      m_peakRadius = peakRadius;
    }

    boost::shared_ptr<PeakOverlayView> PeakOverlayFactoryBase::createView(const Mantid::API::IPeak& peak) const
    {
      Mantid::Kernel::V3D position = peak.getHKL();

      double intensity = peak.getIntensity(); 

      //QwtText xDim = m_plot->axisTitle(QwtPlot::xBottom);
      //QwtText yDim = m_plot->axisTitle(QwtPlot::yLeft);

      // Does the peak match the regex


      /* 1) Find out which dimensions are being plotted on x and y 
      2) Find out what h, k, l each of these dimensions correspond to.
      3) Create the origin x, y based on these hkl values.
      */

      return this->createViewAtPoint(position, m_peakRadius);
    }

  }
}
