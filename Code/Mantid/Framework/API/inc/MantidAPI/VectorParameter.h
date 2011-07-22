#ifndef MANTID_MDALGORITHMS_VectorParameter_H_
#define MANTID_MDALGORITHMS_VectorParameter_H_

#include "MantidKernel/System.h"
#include "MantidAPI/ImplicitFunctionParameter.h"


namespace Mantid
{
namespace API
{

/**
VectorParameter is abstract type implementing curiously recurring template pattern to implement common code associated with vector storage.

@author Owen Arnold, Tessella plc
@date 21/07/2011

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
  typedef ElemType ValueType;
  VectorParameter();
  VectorParameter(const VectorParameter<Derived, ElemType>& other);
  void addValue(const ElemType& value);
  std::string toXMLString() const;
  Derived& operator=(const Derived& other);
  bool operator==(const Derived &other) const;
  bool operator!=(const Derived &other) const;
  virtual bool isValid() const;
  ElemType& operator[] (int index);
  typename std::vector<ElemType>::reference element(int index);
  size_t getSize() const;
protected:
  std::vector<ElemType> m_vector;
  bool m_isValid;
};

//----------------------------------------------------------------------
/* Getter for the valid state.
@return true if object is valid.
*/
template<typename Derived, typename ElemType>
bool VectorParameter<Derived,ElemType>::isValid() const
{
  return m_isValid;
}

//----------------------------------------------------------------------
/* Overriden assignement operator
@param other : object to assign from.
@return ref to assined object.
*/
template<typename Derived, typename ElemType>
Derived& VectorParameter<Derived,ElemType>::operator=(const Derived& other)
{
  if(other.m_vector.size() != this->m_vector.size())
  {
    throw std::runtime_error("Cannot assign between VectorParameters where the size of the vectors are different.");
  }
  if (&other != this)
  {
    this->m_isValid = other.m_isValid;
    std::copy(other.m_vector.begin(), other.m_vector.end(), this->m_vector.begin());
  }
  return *(dynamic_cast<Derived*>(this));
}

//----------------------------------------------------------------------
/* Overriden equality operator
@param other : object to compare with
@return true if to be considered equal
*/
template<typename Derived, typename ElemType>
bool VectorParameter<Derived,ElemType>::operator==(const Derived &other) const
{
  if(other.m_isValid != this->m_isValid)
  {
    return false; //Early termination
  }
  if(other.getSize() != this->getSize())
  {
    return false; //Early termination
  }
  return std::equal(m_vector.begin(), m_vector.end(), other.m_vector.begin());
}

//----------------------------------------------------------------------
/* Overriden not equals operator
@param other : object to compare with
@return true if to be considered not equal
*/
template<typename Derived, typename ElemType>
bool VectorParameter<Derived,ElemType>::operator!=(const Derived &other) const
{
  return !(*this == other);
}

//----------------------------------------------------------------------
/* Copy constructor
@param other : object to act as source of copy.
*/
template<typename Derived, typename ElemType>
VectorParameter<Derived,ElemType>::VectorParameter(const VectorParameter<Derived, ElemType> & other): m_vector(other.m_vector.size()) , m_isValid(other.m_isValid)
{
  std::copy(other.m_vector.begin(), other.m_vector.end(), this->m_vector.begin());
}

/// Default constructor
template<typename Derived, typename ElemType>
VectorParameter<Derived,ElemType>::VectorParameter() : m_vector(0) 
{
  m_isValid = false;
}

//----------------------------------------------------------------------
/* Setter for values on vector
@param value : Value to add.
*/
template<typename Derived, typename ElemType>
void VectorParameter<Derived,ElemType>::addValue(const ElemType& value)
{
  m_vector.push_back(value);
  m_isValid = true;
}

//----------------------------------------------------------------------
/* Serialize the object to an xml string.
@return string containing object in xml serialized form.
*/
template<typename Derived, typename ElemType>
std::string VectorParameter<Derived,ElemType>::toXMLString() const
{
  if(!m_isValid)
  {
    throw std::runtime_error("Cannot serialize VectorParameter if it is not valid!");
  }
  std::string valueXMLtext;
  size_t vecSize = m_vector.size();
  
  for(size_t i = 0; i < vecSize ; i++)
  {
    if(i < (vecSize -1))
    {
      valueXMLtext.append(ElementTraits<ElemType>::formatCS(m_vector[i])); //Comma-seperated
    }
    else
    {
      valueXMLtext.append(ElementTraits<ElemType>::format(m_vector[i])); //No comma sepearation for last value
    }
  }
  return this->parameterXMLTemplate(valueXMLtext);
}


//----------------------------------------------------------------------
/* Overrriden array operator
@param index : array index to access.
@return ref to the element at the specified index.
*/
template<typename Derived, typename ElemType>
ElemType& VectorParameter<Derived,ElemType>::operator[] (int index)
{
  return m_vector[index];
}

template<typename Derived, typename ElemType>
typename std::vector<ElemType>::reference VectorParameter<Derived,ElemType>::element(int index) 
{
   return m_vector[index];
}

//----------------------------------------------------------------------
/* Getter for the vector size.
@return the size of the vector
*/
template<typename Derived, typename ElemType>
size_t VectorParameter<Derived,ElemType>::getSize() const
{
  return m_vector.size();
}

//-----------------------------------------------------------------------------------------------------------------//
// Macro for generating concrete types of VectorParameters. 
//
// Use of macro enables parameter names to be assigned to each type.
// Most of the work is done in the VectorParamter base class, which utilises CRTP.
//-----------------------------------------------------------------------------------------------------------------//
#define DECLARE_VECTOR_PARAMETER(classname, type_) \
    class classname : public Mantid::API::VectorParameter<classname, type_> \
    {  \
      public: \
      typedef Mantid::API::VectorParameter<classname, type_> SuperType;  \
      static std::string parameterName(){ return #classname;} \
      classname() : SuperType() {} \
      std::string getName() const { return #classname;} \
      classname* clone() const {return new classname(*this); } \
    }; 

}
}

#endif
