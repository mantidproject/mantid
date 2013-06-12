#include "MantidQtSliceViewer/PeakOverlayCrossFactory.h"
#include "MantidQtSliceViewer/PeakOverlayCross.h"
#include "MantidKernel/V3D.h"
#include "MantidAPI/IPeak.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include <boost/make_shared.hpp>
#include <boost/regex.hpp>

using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace MantidQt
{
  namespace SliceViewer
  {

    PeakOverlayCrossFactory::PeakOverlayCrossFactory(boost::shared_ptr<Mantid::API::MDGeometry> mdWS, PeakTransform_const_sptr transform, IPeaksWorkspace_sptr peaksWS, QwtPlot * plot, QWidget * parent, const size_t colourNumber)
    : PeakOverlayViewFactoryBase(plot, parent, colourNumber),
      m_peaksWS(peaksWS),
      m_zMax(0),
      m_zMin(0)
    {
      for (size_t dimIndex = 0; dimIndex < mdWS->getNumDims(); ++dimIndex)
      {
        IMDDimension_const_sptr dimensionMappedToZ = mdWS->getDimension(dimIndex);
        if (boost::regex_match(dimensionMappedToZ->getName(), transform->getFreePeakAxisRegex()))
        {
          m_zMax = dimensionMappedToZ->getMaximum();
          m_zMin = dimensionMappedToZ->getMinimum();
          break;
        }
      }
    }

    PeakOverlayCrossFactory::~PeakOverlayCrossFactory()
    {
    }

    boost::shared_ptr<PeakOverlayView> PeakOverlayCrossFactory::createView(const int peakIndex, PeakTransform_const_sptr transform) const
    {
      const IPeak& peak = m_peaksWS->getPeak(peakIndex);
      auto position = transform->transformPeak(peak);
      return boost::make_shared<PeakOverlayCross>(m_plot, m_parent, position, m_zMax, m_zMin, this->m_peakColour);
    }

    int PeakOverlayCrossFactory::FOM() const
    {
      return 1; 
    }
  }
}
