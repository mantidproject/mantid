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
class DLLExport VectorParameter : public  ImplicitFunctionParameter
{
public:
  typedef ElemType ValueType;
  VectorParameter();
  VectorParameter(size_t size);
  VectorParameter(const VectorParameter<Derived, ElemType>& other);
  virtual ~VectorParameter();
  void addValue(const size_t index, const ElemType& value);
  std::string toXMLString() const;
  Derived& assignFrom(const Derived& other);
  bool operator==(const Derived &other) const;
  bool operator!=(const Derived &other) const;
  virtual bool isValid() const;
  ElemType& operator[] (int index);
  const ElemType* getPointerToStart();
  size_t getSize() const;
  ElemType& at(size_t index);
protected:
  ElemType* m_arry;
  size_t m_size;
  bool m_isValid;
 private:
Derived& operator=(const Derived& other);
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
/* Assignment
@param other : object to assign from.
@return ref to assined object.
*/
template<typename Derived, typename ElemType>
Derived& VectorParameter<Derived,ElemType>::assignFrom(const Derived& other)
{
  if(other.getSize() != this->getSize())
  {
    throw std::runtime_error("Cannot assign between VectorParameters where the size of the vectors are different.");
  }
  if (&other != this)
  {
    this->m_isValid = other.m_isValid;
    for(size_t i = 0; i < other.getSize(); i++)
    {
      this->m_arry[i] = other.m_arry[i];
    }
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
  for(size_t i = 0; i < other.getSize(); i++)
  {
     if(this->m_arry[i] != other.m_arry[i])
     {
       return false;
     }
  }
  return true;
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
VectorParameter<Derived,ElemType>::VectorParameter(const VectorParameter<Derived, ElemType> & other): m_size(other.m_size), m_isValid(other.m_isValid)
{
  m_arry = new ElemType[other.getSize()];
  for(size_t i = 0; i < other.getSize(); i++)
  {
    this->m_arry[i] = other.m_arry[i];
  }
}

/// Default constructor
template<typename Derived, typename ElemType>
VectorParameter<Derived,ElemType>::VectorParameter() : m_arry(NULL), m_size(0)
{
  m_isValid = false;
}

//----------------------------------------------------------------------
/* Constructor
@param size : size of array
*/
template<typename Derived, typename ElemType>
VectorParameter<Derived,ElemType>::VectorParameter(size_t size) : m_size(size)
{
  m_arry = new ElemType[size];
  m_isValid = true;
}

///Destructor
template<typename Derived, typename ElemType>
VectorParameter<Derived,ElemType>::~VectorParameter()
{
  delete[] m_arry;
}

//----------------------------------------------------------------------
/* Setter for values on vector
@param value : Value to add.
*/
template<typename Derived, typename ElemType>
void VectorParameter<Derived,ElemType>::addValue(const size_t index, const ElemType& value)
{
  m_arry[index] = value;
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
  for(size_t i = 0; i < m_size ; i++)
  {
    if(i < (m_size -1))
    {
      valueXMLtext.append(ElementTraits<ElemType>::formatCS(m_arry[i])); //Comma-seperated
    }
    else
    {
      valueXMLtext.append(ElementTraits<ElemType>::format(m_arry[i])); //No comma sepearation for last value
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
  return m_arry[index];
}

//----------------------------------------------------------------------
/* Getter for the vector size.
@return the size of the vector
*/
template<typename Derived, typename ElemType>
size_t VectorParameter<Derived,ElemType>::getSize() const
{
  return m_size;
}

//----------------------------------------------------------------------
/* Getter to pointer to the start of the underlying array
@return pointer to start.
*/
template<typename Derived, typename ElemType>
const ElemType* VectorParameter<Derived,ElemType>::getPointerToStart()
{
  return m_arry;
}

//----------------------------------------------------------------------
/* Convienince array access method
@param index : index to get element at
@return element at index
*/
template<typename Derived, typename ElemType>
ElemType& VectorParameter<Derived,ElemType>::at(size_t index)
{
  return m_arry[index];
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
      classname(size_t index) : SuperType(index) {} \
      std::string getName() const { return #classname;} \
      classname* clone() const {return new classname(*this); } \
    }; 

}
}

#endif
