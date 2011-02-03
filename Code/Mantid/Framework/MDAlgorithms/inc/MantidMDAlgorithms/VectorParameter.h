#ifndef MANTID_MDALGORITHMS_VECTORPARAMETER_H_
#define MANTID_MDALGORITHMS_VECTORPARAMETER_H_

#include "MantidKernel/System.h"
#include "MantidAPI/ImplicitFunctionParameter.h"
#include "boost/algorithm/string.hpp"
#include "boost/format.hpp"

namespace Mantid
{
namespace MDAlgorithms
{

/**
VectorParameter is abstract type implementing curiously recurring template pattern to implement common code associated with vector storage.

@author Owen Arnold, Tessella plc
@date 01/02/2011

Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
using Mantid::API::ImplicitFunctionParameter;
template<typename Derived, typename ElemType>
class DLLExport VectorParameter : public  ImplicitFunctionParameter
{
public:

  VectorParameter(ElemType a, ElemType b, ElemType c);

  VectorParameter();

  VectorParameter(const VectorParameter<Derived, ElemType>& other);

  std::string toXMLString() const;

  Derived& operator=(const Derived& other);

  bool operator==(const Derived &other) const;

  bool operator!=(const Derived &other) const;

  ElemType getX() const;

  ElemType getY() const;

  ElemType getZ() const;

protected:

  std::vector<ElemType> m_vector;

  bool m_isValid;
};

template<typename Derived, typename ElemType>
Derived& VectorParameter<Derived,ElemType>::operator=(const Derived& other)
{
  if (&other != this)
  {
    this->m_isValid = other.m_isValid;
    this->m_vector.swap(std::vector<ElemType>(3));
    std::copy(other.m_vector.begin(), other.m_vector.end(), this->m_vector.begin());
  }
  return *this;
}

template<typename Derived, typename ElemType>
bool VectorParameter<Derived,ElemType>::operator==(const Derived &other) const
{
  return std::equal(m_vector.begin(), m_vector.end(), other.m_vector.begin());
}

template<typename Derived, typename ElemType>
bool VectorParameter<Derived,ElemType>::operator!=(const Derived &other) const
{
  return !(*this == other);
}

template<typename Derived, typename ElemType>
VectorParameter<Derived,ElemType>::VectorParameter(const VectorParameter<Derived, ElemType> & other): m_vector(3)
{
  m_isValid = other.isValid();
  if(true == other.m_isValid)
  {
    m_vector[0] = other.m_vector[0];
    m_vector[1] = other.m_vector[1];
    m_vector[2] = other.m_vector[2];
  }
}

template<typename Derived, typename ElemType>
VectorParameter<Derived,ElemType>::VectorParameter() : m_isValid(false)
{

}

template<typename Derived, typename ElemType>
VectorParameter<Derived,ElemType>::VectorParameter(ElemType a, ElemType b, ElemType c) : m_vector(3)
{
  m_vector[0] = a;
  m_vector[1] = b;
  m_vector[2] = c;
  m_isValid = true;
}

template<typename Derived, typename ElemType>
ElemType VectorParameter<Derived,ElemType>::getX() const
{
    return m_vector[0];
}

template<typename Derived, typename ElemType>
ElemType VectorParameter<Derived,ElemType>::getY() const
{
    return m_vector[1];
}

template<typename Derived, typename ElemType>
ElemType VectorParameter<Derived,ElemType>::getZ() const
{
    return m_vector[2];
}

template<typename Derived, typename ElemType>
std::string VectorParameter<Derived,ElemType>::toXMLString() const
{
  std::string valueXMLtext = boost::str(boost::format("%.4f, %.4f, %.4f") % m_vector[0] %  m_vector[1] % m_vector[2]);
  return this->parameterXMLTemplate(valueXMLtext);
}

}
}

#endif
