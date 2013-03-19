#include "MantidQtSliceViewer/PeakOverlaySphereFactory.h"
#include "MantidQtSliceViewer/PeakOverlaySphere.h"
#include <boost/make_shared.hpp>

using namespace Mantid::API;

namespace MantidQt
{
  namespace SliceViewer
  {

    PeakOverlaySphereFactory::PeakOverlaySphereFactory(IPeaksWorkspace_sptr peaksWS, QwtPlot * plot, QWidget * parent, const size_t colourNumber) :
        PeakOverlayViewFactoryBase(plot, parent, colourNumber),
        m_peakRadius(0),
        m_backgroundInnerRadius(0),
        m_backgroundOuterRadius(0),
        m_peaksWS(peaksWS),
        m_FOM(0)
    {
      if (m_peaksWS->hasIntegratedPeaks())
      {
        m_peakRadius = boost::lexical_cast<double>(
            m_peaksWS->run().getProperty("PeakRadius")->value());
        m_backgroundInnerRadius = boost::lexical_cast<double>(
            m_peaksWS->run().getProperty("BackgroundInnerRadius")->value());
        m_backgroundOuterRadius = boost::lexical_cast<double>(
            m_peaksWS->run().getProperty("BackgroundOuterRadius")->value());
        m_FOM = 2; // Possible to display workspaces with this factory.
      }
    }

    boost::shared_ptr<PeakOverlayView> PeakOverlaySphereFactory::createView(const int peakIndex, PeakTransform_const_sptr transform) const
    {
      const IPeak& peak = m_peaksWS->getPeak(peakIndex);
      auto position = transform->transformPeak(peak);
      return boost::make_shared<PeakOverlaySphere>(m_plot, m_parent, position, this->m_peakRadius, this->m_backgroundInnerRadius, this->m_backgroundOuterRadius, this->m_peakColour, this->m_backColour);
    }

    PeakOverlaySphereFactory::~PeakOverlaySphereFactory()
    {
    }

    int PeakOverlaySphereFactory::FOM() const
    {
      return m_FOM;
    }

  }
}
