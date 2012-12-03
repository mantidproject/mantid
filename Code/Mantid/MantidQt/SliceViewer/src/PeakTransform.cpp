#include "MantidQtSliceViewer/PeakTransform.h"

namespace MantidQt
{
  namespace SliceViewer
  {

   PeakTransform::PeakTransform(const std::string& xPlotLabel, const std::string& yPlotLabel) : 
  m_xPlotLabel(xPlotLabel),
    m_yPlotLabel(yPlotLabel),
    m_indexOfPlotX(0),
    m_indexOfPlotY(1),
    m_indexOfPlotZ(2),
    m_HRegex("^H.*$"),
    m_KRegex("^K.*$"),
    m_LRegex("^L.*$")
  {
    const std::string& xLabel = m_xPlotLabel;
    const std::string& yLabel = m_yPlotLabel;

    const int H = 0;
    const int K = 1;
    const int L = 2;

    Mantid::Kernel::V3D positionInCoordinateSystem;
    if(boost::regex_match(xLabel, m_HRegex) && boost::regex_match(yLabel, m_KRegex)) //HKL
    {
      m_indexOfPlotX = H;
      m_indexOfPlotY = K; 
      m_indexOfPlotZ = L; 
    }
    else if(boost::regex_match(xLabel, m_HRegex) && boost::regex_match(yLabel, m_LRegex)) //HLK
    {
      m_indexOfPlotX = H;
      m_indexOfPlotY = L; 
      m_indexOfPlotZ = K; 
    }
    else if(boost::regex_match(xLabel, m_LRegex) && boost::regex_match(yLabel, m_HRegex)) //LHK
    {
      m_indexOfPlotX = L;
      m_indexOfPlotY = H; 
      m_indexOfPlotZ = K; 
    }
    else if(boost::regex_match(xLabel, m_LRegex) && boost::regex_match(yLabel, m_KRegex)) //LKH
    {
      m_indexOfPlotX = L;
      m_indexOfPlotY = K; 
      m_indexOfPlotZ = H; 
    }
    else if(boost::regex_match(xLabel, m_KRegex) && boost::regex_match(yLabel, m_LRegex)) //KLH
    {
      m_indexOfPlotX = K;
      m_indexOfPlotY = L; 
      m_indexOfPlotZ = H; 
    }
    else if(boost::regex_match(xLabel, m_KRegex) && boost::regex_match(yLabel, m_HRegex)) //KHL
    {
      m_indexOfPlotX = K;
      m_indexOfPlotY = H; 
      m_indexOfPlotZ = L; 
    }
    else
    {
      throw PeakTransformException();
    }
  }

  PeakTransform::PeakTransform(const PeakTransform& other):
    m_xPlotLabel(other.m_xPlotLabel),
    m_yPlotLabel(other.m_yPlotLabel),
    m_indexOfPlotX(other.m_indexOfPlotX),
    m_indexOfPlotY(other.m_indexOfPlotY),
    m_indexOfPlotZ(other.m_indexOfPlotZ),
    m_HRegex(other.m_HRegex),
    m_KRegex(other.m_KRegex),
    m_LRegex(other.m_LRegex)
  {
  }

  PeakTransform& PeakTransform::operator=(const PeakTransform & other)
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

  PeakTransform::~PeakTransform()
  {
  }

  boost::regex PeakTransform::getFreePeakAxisRegex() const
  {
    switch(m_indexOfPlotZ)
    {
    case 0:
      return m_HRegex;
    case 1:
      return m_KRegex;
    default:
      return m_LRegex;
    }
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