#ifndef MANTID_MDALGORITHMS_Vector3DParameter_H_
#define MANTID_MDALGORITHMS_Vector3DParameter_H_

#include "MantidKernel/System.h"
#include "MantidAPI/ImplicitFunctionParameter.h"
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

namespace Mantid
{
namespace MDAlgorithms
{

/**
Vector3DParameter is abstract type implementing curiously recurring template pattern to implement common code associated with vector storage.

@author Owen Arnold, Tessella plc
@date 01/02/2011

Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
template<typename Derived, typename ElemType>
class DLLExport Vector3DParameter : public  Mantid::API::ImplicitFunctionParameter
{
public:
  Vector3DParameter(ElemType a, ElemType b, ElemType c);
  Vector3DParameter();
  Vector3DParameter(const Vector3DParameter<Derived, ElemType>& other);
  std::string toXMLString() const;
  Derived& operator=(const Derived& other);
  bool operator==(const Derived &other) const;
  bool operator!=(const Derived &other) const;
  ElemType getX() const;
  ElemType getY() const;
  ElemType getZ() const;
  virtual bool isValid() const;
  ElemType& operator[] (int index);
protected:
  std::vector<ElemType> m_vector;
  bool m_isValid;
};

template<typename Derived, typename ElemType>
bool Vector3DParameter<Derived,ElemType>::isValid() const
{
  return m_isValid;
}

template<typename Derived, typename ElemType>
Derived& Vector3DParameter<Derived,ElemType>::operator=(const Derived& other)
{
  if (&other != this)
  {
    this->m_isValid = other.m_isValid;
    this->m_vector.swap(std::vector<ElemType>(3));
    std::copy(other.m_vector.begin(), other.m_vector.end(), this->m_vector.begin());
  }
  return *(dynamic_cast<Derived*>(this));
}

template<typename Derived, typename ElemType>
bool Vector3DParameter<Derived,ElemType>::operator==(const Derived &other) const
{
  return std::equal(m_vector.begin(), m_vector.end(), other.m_vector.begin());
}

template<typename Derived, typename ElemType>
bool Vector3DParameter<Derived,ElemType>::operator!=(const Derived &other) const
{
  return !(*this == other);
}

template<typename Derived, typename ElemType>
Vector3DParameter<Derived,ElemType>::Vector3DParameter(const Vector3DParameter<Derived, ElemType> & other): m_vector(3)
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
Vector3DParameter<Derived,ElemType>::Vector3DParameter() : m_vector(3, 0), m_isValid(false)
{

}

template<typename Derived, typename ElemType>
Vector3DParameter<Derived,ElemType>::Vector3DParameter(ElemType a, ElemType b, ElemType c) : m_vector(3)
{
  m_vector[0] = a;
  m_vector[1] = b;
  m_vector[2] = c;
  m_isValid = true;
}

template<typename Derived, typename ElemType>
ElemType Vector3DParameter<Derived,ElemType>::getX() const
{
    return m_vector[0];
}

template<typename Derived, typename ElemType>
ElemType Vector3DParameter<Derived,ElemType>::getY() const
{
    return m_vector[1];
}

template<typename Derived, typename ElemType>
ElemType Vector3DParameter<Derived,ElemType>::getZ() const
{
    return m_vector[2];
}

template<typename Derived, typename ElemType>
std::string Vector3DParameter<Derived,ElemType>::toXMLString() const
{
  std::string valueXMLtext = boost::str(boost::format("%.4f, %.4f, %.4f") % m_vector[0] %  m_vector[1] % m_vector[2]);
  return this->parameterXMLTemplate(valueXMLtext);
}

template<typename Derived, typename ElemType>
ElemType& Vector3DParameter<Derived,ElemType>::operator[] (int index)
{
  return m_vector[index];
}

//-----------------------------------------------------------------------------------------------------------------//
// Macro for generating concrete types of Vector3DParameters. 
//
// Use of macro enables parameter names to be assigned to each type.
// Most of the work is done in the VectorParamter base class, which utilises CRTP.
//-----------------------------------------------------------------------------------------------------------------//
#define DECLARE_3D_VECTOR_PARAMETER(classname, type_) \
    class classname : public Mantid::MDAlgorithms::Vector3DParameter<classname, double> \
    {  \
      public: \
      typedef Vector3DParameter<classname, type_> SuperType;  \
      static std::string parameterName(){ return #classname;} \
      classname(type_ a, type_ b, type_ c) : SuperType(a, b, c) {} \
      classname() : SuperType() {} \
      std::string getName() const { return #classname;} \
      classname* clone() const {return new classname(m_vector[0], m_vector[1], m_vector[2]); } \
    }; 

}
}

#endif
