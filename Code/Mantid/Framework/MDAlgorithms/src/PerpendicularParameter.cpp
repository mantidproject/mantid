#include "boost/algorithm/string.hpp"
#include "boost/format.hpp"
#include "MantidMDAlgorithms/PerpendicularParameter.h"

namespace Mantid
{
namespace MDAlgorithms
{

PerpendicularParameter::PerpendicularParameter(double n1, double n2, double n3) : m_perpendicular(3)
{
    m_perpendicular[0] = n1;
    m_perpendicular[1] = n2;
    m_perpendicular[2] = n3;
    m_isValid = true;
}

PerpendicularParameter::PerpendicularParameter()
{
    this->m_isValid = false;
}

PerpendicularParameter::PerpendicularParameter(const PerpendicularParameter& other)
{
    this->m_isValid = other.m_isValid;
    this->m_perpendicular = std::vector<double>(3);
    std::copy(other.m_perpendicular.begin(), other.m_perpendicular.end(), this->m_perpendicular.begin());
}

PerpendicularParameter& PerpendicularParameter::operator=(const PerpendicularParameter& other)
{
    if(&other != this)
    {
        this->m_isValid = other.m_isValid;
        if(m_isValid)
        {
          std::vector<double> clear(3);
          m_perpendicular.swap(clear);
          m_perpendicular[0] = other.m_perpendicular[0];
          m_perpendicular[1] = other.m_perpendicular[1];
          m_perpendicular[2] = other.m_perpendicular[2];
        }
    }
    return *this;
}

std::string PerpendicularParameter::getName() const
{
    return parameterName();
}

bool PerpendicularParameter::isValid() const
{
    return m_isValid;
}

PerpendicularParameter* PerpendicularParameter::clone() const
{
    return new PerpendicularParameter(m_perpendicular.at(0), m_perpendicular.at(1), m_perpendicular.at(2));
}

PerpendicularParameter::~PerpendicularParameter()
{
}

double PerpendicularParameter::getX() const
{
    return m_perpendicular.at(0);
}

double PerpendicularParameter::getY() const
{
    return m_perpendicular.at(1);
}

double PerpendicularParameter::getZ() const
{
    return m_perpendicular.at(2);
}

bool PerpendicularParameter::operator==(const PerpendicularParameter &other) const
{
    return this->m_perpendicular.at(0) == other.m_perpendicular.at(0) && this->m_perpendicular.at(1) == other.m_perpendicular.at(1) && this->m_perpendicular.at(2) == other.m_perpendicular.at(2);
}

bool PerpendicularParameter::operator!=(const PerpendicularParameter &other) const
{
    return !(*this == other);
}

std::string PerpendicularParameter::toXMLString() const
{
    std::string valueXMLtext = boost::str(boost::format("%.4f, %.4f, %.4f") % m_perpendicular.at(0) %  m_perpendicular.at(1) % m_perpendicular.at(2));

    return this->parameterXMLTemplate(valueXMLtext);
}

}
}
