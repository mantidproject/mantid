#include "boost/algorithm/string.hpp"
#include "boost/format.hpp"
#include "MantidMDAlgorithms/DepthParameter.h"

namespace Mantid
{
  namespace MDAlgorithms
  {

    DepthParameter::DepthParameter(double depth) : m_depth(depth)
    {
      if(depth >= 0)
      {
        m_isValid = true;
      }
      else
      {
       m_isValid = false;
      }
    }

    DepthParameter::DepthParameter()
    { 
      m_isValid = false;
    }

    DepthParameter::DepthParameter(const DepthParameter& other)
    {
      this->m_isValid = other.m_isValid;
      this->m_depth = other.m_depth;
    }

    DepthParameter& DepthParameter::operator=(const DepthParameter& other)
    {
      if (&other != this)
      {
        this->m_isValid = other.m_isValid;
        this->m_depth = other.m_depth;
      }
      return *this;
    }

    std::string DepthParameter::getName() const
    {
      return parameterName();
    }

    bool DepthParameter::isValid() const
    {
      return this->m_isValid;
    }

    DepthParameter* DepthParameter::clone() const
    {
      return new DepthParameter(m_depth);
    }

    DepthParameter::~DepthParameter()
    {
    }

    double DepthParameter::getValue() const
    {
      return m_depth;
    }


    bool DepthParameter::operator==(const DepthParameter &other) const
    {
      return this->m_depth == other.m_depth;
    }

    bool DepthParameter::operator!=(const DepthParameter &other) const
    {
      return !(*this == other);
    }

    std::string DepthParameter::toXMLString() const
    {
      std::string valueXMLtext = boost::str(boost::format("%.4f") % m_depth);

      return this->parameterXMLTemplate(valueXMLtext);
    }

  }

}
