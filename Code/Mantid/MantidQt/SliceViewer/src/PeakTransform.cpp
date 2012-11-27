#include "MantidQtSliceViewer/PeakTransform.h"
#include <boost/regex.hpp>

namespace MantidQt
{
  namespace SliceViewer
  {

   PeakTransform::PeakTransform(const std::string& xPlotLabel, const std::string& yPlotLabel) : 
  m_xPlotLabel(xPlotLabel),
    m_yPlotLabel(yPlotLabel),
    m_indexOfPlotX(0),
    m_indexOfPlotY(1),
    m_indexOfPlotZ(2)
  {
    const std::string& xLabel = m_xPlotLabel;
    const std::string& yLabel = m_yPlotLabel;

    const boost::regex HRegex = boost::regex("^H.*$");
    const boost::regex KRegex = boost::regex("^K.*$");
    const boost::regex LRegex = boost::regex("^L.*$");

    const int H = 0;
    const int K = 1;
    const int L = 2;

    Mantid::Kernel::V3D positionInCoordinateSystem;
    if(boost::regex_match(xLabel, HRegex) && boost::regex_match(yLabel, KRegex)) //HKL
    {
      m_indexOfPlotX = H;
      m_indexOfPlotY = K; 
      m_indexOfPlotZ = L; 
    }
    else if(boost::regex_match(xLabel, HRegex) && boost::regex_match(yLabel, LRegex)) //HLK
    {
      m_indexOfPlotX = H;
      m_indexOfPlotY = L; 
      m_indexOfPlotZ = K; 
    }
    else if(boost::regex_match(xLabel, LRegex) && boost::regex_match(yLabel, HRegex)) //LHK
    {
      m_indexOfPlotX = L;
      m_indexOfPlotY = H; 
      m_indexOfPlotZ = K; 
    }
    else if(boost::regex_match(xLabel, LRegex) && boost::regex_match(yLabel, KRegex)) //LKH
    {
      m_indexOfPlotX = L;
      m_indexOfPlotY = K; 
      m_indexOfPlotZ = H; 
    }
    else if(boost::regex_match(xLabel, KRegex) && boost::regex_match(yLabel, LRegex)) //KLH
    {
      m_indexOfPlotX = K;
      m_indexOfPlotY = L; 
      m_indexOfPlotZ = H; 
    }
    else if(boost::regex_match(xLabel, KRegex) && boost::regex_match(yLabel, HRegex)) //KHL
    {
      m_indexOfPlotX = K;
      m_indexOfPlotY = H; 
      m_indexOfPlotZ = L; 
    }
    else
    {
      throw std::runtime_error("Could not process mapped dimensions.");
    }
  }

  PeakTransform::PeakTransform(const PeakTransform& other):
    m_indexOfPlotX(other.m_indexOfPlotX),
    m_indexOfPlotY(other.m_indexOfPlotY),
    m_indexOfPlotZ(other.m_indexOfPlotZ),
    m_xPlotLabel(other.m_xPlotLabel),
    m_yPlotLabel(other.m_yPlotLabel)
  {
  }

  PeakTransform& PeakTransform::operator=(const PeakTransform & other)
  {
    if(this != &other)
    {
      m_indexOfPlotX = other.m_indexOfPlotX;
      m_indexOfPlotY = other.m_indexOfPlotY;
      m_indexOfPlotZ = other.m_indexOfPlotZ;
      m_xPlotLabel = other.m_xPlotLabel;
      m_yPlotLabel = other.m_yPlotLabel;
    }
    return *this;
  }

  PeakTransform::~PeakTransform()
  {
  }

  Mantid::Kernel::V3D PeakTransform::transform(const Mantid::Kernel::V3D& original) const
  {
    // Will have the plots x, y, and z aligned to the correct h, k, l value.
    Mantid::Kernel::V3D transformedPeakPosition;
    transformedPeakPosition.setX(original[m_indexOfPlotX]);
    transformedPeakPosition.setY(original[m_indexOfPlotY]);
    transformedPeakPosition.setZ(original[m_indexOfPlotZ]);
    return transformedPeakPosition;
  }

}
}