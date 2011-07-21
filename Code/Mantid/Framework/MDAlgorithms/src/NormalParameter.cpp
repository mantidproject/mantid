#include "MantidMDAlgorithms/NormalParameter.h"
#include <cmath>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

namespace Mantid
{
  namespace MDAlgorithms
  {

    NormalParameter::NormalParameter(double n1, double n2, double n3) : SuperType(n1, n2, n3)
    {
    }

    NormalParameter::NormalParameter()
    {
    }

    NormalParameter* NormalParameter::clone() const
    {
      return new NormalParameter(m_vector[0], m_vector[1], m_vector[2]);
    }

    NormalParameter::~NormalParameter()
    {
    }

    NormalParameter NormalParameter::reflect() const
    {
      return NormalParameter(-m_vector[0], -m_vector[1], -m_vector[2]);
    }

    NormalParameter NormalParameter::asUnitVector() const
    {
      double mag = magnitude();
      return NormalParameter(m_vector[0]/mag, m_vector[1]/mag, m_vector[2]/mag);
    }

    bool NormalParameter::isUnitVector() const
    {
      return 1 == magnitude();
    }

    double NormalParameter::magnitude() const
    {
      return std::sqrt(m_vector[0]*m_vector[0] + m_vector[1]*m_vector[1] + m_vector[2]*m_vector[2]);
    }

    std::string NormalParameter::getName() const
    {
      return NormalParameter::parameterName();
    }

  }

}
