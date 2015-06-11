#include "MantidQtSliceViewer/PeakOverlayMultiSphereFactory.h"
#include "MantidQtSliceViewer/PeakOverlayMultiSphere.h"
#include "MantidQtSliceViewer/PeaksPresenter.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidDataObjects/PeakShapeSpherical.h"
#include <boost/make_shared.hpp>

using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace MantidQt
{
  namespace SliceViewer
  {

    PeakOverlayMultiSphereFactory::PeakOverlayMultiSphereFactory(IPeaksWorkspace_sptr peaksWS, QwtPlot * plot, QWidget * parent, const int plotXIndex, const int plotYIndex, const size_t colourNumber) :
        PeakOverlayViewFactoryBase(plot, parent, plotXIndex, plotYIndex, colourNumber),
        m_peakRadius(0),
        m_backgroundInnerRadius(0),
        m_backgroundOuterRadius(0),
        m_peaksWS(peaksWS),
        m_FOM(0)
    {
      if (m_peaksWS->hasIntegratedPeaks()) // TODO depends on the shape.
      {
    	  try
    	  {
				m_peakRadius =
					m_peaksWS->run().getPropertyValueAsType<std::vector<double> >("PeakRadius");
				m_backgroundInnerRadius =
					m_peaksWS->run().getPropertyValueAsType<std::vector<double> >("BackgroundInnerRadius");
				m_backgroundOuterRadius =
					m_peaksWS->run().getPropertyValueAsType<std::vector<double> >("BackgroundOuterRadius");
    	  }
    	  catch (...)
    	  {
				double m_peakRadius0 = m_peaksWS->run().getPropertyValueAsType<double>("PeakRadius");
				double m_backgroundInnerRadius0 =
					m_peaksWS->run().getPropertyValueAsType<double>("BackgroundInnerRadius");
				double m_backgroundOuterRadius0 =
					m_peaksWS->run().getPropertyValueAsType<double>("BackgroundOuterRadius");
				for(size_t i = 0; i < m_peaksWS->rowCount(); ++i)
				{
					m_peakRadius.push_back(m_peakRadius0);
					m_backgroundInnerRadius.push_back(m_backgroundInnerRadius0);
					m_backgroundOuterRadius.push_back(m_backgroundOuterRadius0);
				}
    	  }
        m_FOM = 2; // Possible to display workspaces with this factory.
      }
    }

    GCC_DIAG_OFF(strict-aliasing)
    boost::shared_ptr<PeakOverlayView> PeakOverlayMultiSphereFactory::createView(PeaksPresenter* const presenter, Mantid::Geometry::PeakTransform_const_sptr transform) const
    {
      // Construct all physical peaks
      VecPhysicalSphericalPeak physicalPeaks(m_peaksWS->rowCount());
      for(size_t i = 0; i < physicalPeaks.size(); ++i)
      {
        const Mantid::Geometry::IPeak& peak = m_peaksWS->getPeak(static_cast<int>(i));
        const Mantid::Geometry::PeakShape& peakShape = peak.getPeakShape();
        auto position = transform->transformPeak(peak);
        if(const PeakShapeSpherical* sphericalShape = dynamic_cast<const PeakShapeSpherical*>(&peakShape)){
            auto radius = sphericalShape->radius();
            auto optOuterRadius = sphericalShape->backgroundOuterRadius();
            auto optInnerRadius = sphericalShape->backgroundInnerRadius();


            auto outerRadius = optOuterRadius.is_initialized() ? optOuterRadius.get() : radius;
            auto innerRadius = optInnerRadius.is_initialized() ? optInnerRadius.get() : radius;


            physicalPeaks[i] = boost::make_shared<PhysicalSphericalPeak>(position, radius, innerRadius, outerRadius);

        } else {
            // This method of doing things is effectivlely deprecated now since we have the PeakShape. I will eventually strip this out.
            physicalPeaks[i] = boost::make_shared<PhysicalSphericalPeak>(position, m_peakRadius[i], m_backgroundInnerRadius[i], m_backgroundOuterRadius[i]);

        }

       }

      // Make the overlay widget.
      return boost::make_shared<PeakOverlayMultiSphere>(presenter, m_plot, m_parent, physicalPeaks, m_plotXIndex, m_plotYIndex, this->m_peakColour, this->m_backColour);
    }

    PeakOverlayMultiSphereFactory::~PeakOverlayMultiSphereFactory()
    {
    }

    int PeakOverlayMultiSphereFactory::FOM() const
    {
        return m_FOM;
    }

    void PeakOverlayMultiSphereFactory::swapPeaksWorkspace(boost::shared_ptr<IPeaksWorkspace> &peaksWS)
    {
        m_peaksWS = peaksWS;
    }

  }
}
