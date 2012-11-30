#include "MantidQtSliceViewer/PeakTransformHKL.h"
#include <boost/make_shared.hpp>

using boost::regex;

namespace MantidQt
{
  namespace SliceViewer
  {

  PeakTransformHKL::PeakTransformHKL(const std::string& xPlotLabel, const std::string& yPlotLabel) : PeakTransform(xPlotLabel, yPlotLabel, regex("^H.*$"), regex("^K.*$"), regex("^L.*$") )
  {
  }

  PeakTransformHKL::PeakTransformHKL(const PeakTransformHKL& other): PeakTransform(other)
  {
  }

  PeakTransformHKL& PeakTransformHKL::operator=(const PeakTransformHKL & other)
  {
    if(this != &other)
    {
      m_xPlotLabel = other.m_xPlotLabel;
      m_yPlotLabel = other.m_yPlotLabel;
      m_indexOfPlotX = other.m_indexOfPlotX;
      m_indexOfPlotY = other.m_indexOfPlotY;
      m_indexOfPlotZ = other.m_indexOfPlotZ;
      m_HRegex = other.m_HRegex;
      m_KRegex = other.m_KRegex;
      m_LRegex = other.m_LRegex;
    }
    return *this;
  }

  PeakTransformHKL::~PeakTransformHKL()
  {
  }

  /**
  Clone the PeakTransformHKL.
  */
  PeakTransform_sptr PeakTransformHKL::clone() const
  {
    return boost::make_shared<PeakTransformHKL>(*this);
  }

}
}