#include "boost/algorithm/string.hpp"
#include "boost/format.hpp"
#include "MantidMDAlgorithms/UpParameter.h"
#include <cmath>

namespace Mantid
{
namespace MDAlgorithms
{

UpParameter::UpParameter(double n1, double n2, double n3)
{
    m_up.push_back(n1);
    m_up.push_back(n2);
    m_up.push_back(n3);
    m_isValid = true;
}

UpParameter::UpParameter()
{
    this->m_isValid = false;
}

UpParameter::UpParameter(const UpParameter& other) :
  m_up(3)
{
  this->m_isValid = other.m_isValid;
  if (m_isValid)
  {
    m_up[0] = other.m_up[0];
    m_up[1] = other.m_up[1];
    m_up[2] = other.m_up[2];
  }
}

UpParameter& UpParameter::operator=(const UpParameter& other)
{
    if(&other != this)
    {
        this->m_isValid = other.m_isValid;
        if(m_isValid)
        {
          std::vector<double> clear(3);
          m_up.swap(clear);
          m_up[0] = other.m_up[0];
          m_up[1] = other.m_up[1];
          m_up[2] = other.m_up[2];
        }
    }
    return *this;
}

std::string UpParameter::getName() const
{
    return parameterName();
}

bool UpParameter::isValid() const
{
    return m_isValid;
}

UpParameter* UpParameter::clone() const
{
    return new UpParameter(m_up.at(0), m_up.at(1), m_up.at(2));
}

UpParameter::~UpParameter()
{
}

double UpParameter::getX() const
{
    return m_up.at(0);
}

double UpParameter::getY() const
{
    return m_up.at(1);
}

double UpParameter::getZ() const
{
    return m_up.at(2);
}

bool UpParameter::operator==(const UpParameter &other) const
{
    return this->m_up.at(0) == other.m_up.at(0) && this->m_up.at(1) == other.m_up.at(1) && this->m_up.at(2) == other.m_up.at(2);
}

bool UpParameter::operator!=(const UpParameter &other) const
{
    return !(*this == other);
}

std::string UpParameter::toXMLString() const
{
    std::string valueXMLtext = boost::str(boost::format("%.4f, %.4f, %.4f") % m_up.at(0) %  m_up.at(1) % m_up.at(2));

    return this->parameterXMLTemplate(valueXMLtext);
}

UpParameter UpParameter::asUnitVector() const
{
  double mag = magnitude();
  return UpParameter(m_up[0]/mag, m_up[1]/mag, m_up[2]/mag);
}

bool UpParameter::isUnitVector() const
{
  return 1 == magnitude();
}

double UpParameter::magnitude() const
{
  return std::sqrt(m_up[0]*m_up[0] + m_up[1]*m_up[1] + m_up[2]*m_up[2]);
}

}
}

