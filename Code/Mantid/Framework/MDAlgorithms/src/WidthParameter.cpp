#include "boost/algorithm/string.hpp"
#include "boost/format.hpp"
#include "MantidMDAlgorithms/WidthParameter.h"

namespace Mantid
{
  namespace MDAlgorithms
  {

    WidthParameter::WidthParameter(double width) : m_width(width)
    {
      if(width >= 0)
      {
        m_isValid = true;
      }
      else
      {
        m_isValid = false;
      }
    }

    WidthParameter::WidthParameter()
    { 
      m_isValid = false;
    }

    WidthParameter::WidthParameter(const WidthParameter& other)
    {
      this->m_isValid = other.m_isValid;
      this->m_width = other.m_width;
    }

    WidthParameter& WidthParameter::operator=(const WidthParameter& other)
    {
      if (&other != this)
      {
        this->m_isValid = other.m_isValid;
        this->m_width = other.m_width;
      }
      return *this;
    }

    std::string WidthParameter::getName() const
    {
      return parameterName();
    }

    bool WidthParameter::isValid() const
    {
      return this->m_isValid;
    }

    WidthParameter* WidthParameter::clone() const
    {
      return new WidthParameter(m_width);
    }

    WidthParameter::~WidthParameter()
    {
    }

    double WidthParameter::getValue() const
    {
      return m_width;
    }


    bool WidthParameter::operator==(const WidthParameter &other) const
    {
      return this->m_width == other.m_width;
    }

    bool WidthParameter::operator!=(const WidthParameter &other) const
    {
      return !(*this == other);
    }

    std::string WidthParameter::toXMLString() const
    {
      std::string valueXMLtext = boost::str(boost::format("%.4f") % m_width);

      return this->parameterXMLTemplate(valueXMLtext);
    }

  }

}
