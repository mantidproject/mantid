// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/ImplicitFunctionParameter.h"
#include "MantidKernel/System.h"
#ifndef Q_MOC_RUN
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#endif

namespace Mantid {
namespace API {

/**
SingleValueParameter is a templated base class implementing CRTP. Allows
strongly named concrete SingleValueParameters to be defined and used
in a very simple manner.

@author Owen Arnold, Tessella plc
@date 21/07/2011
*/
template <typename Derived, typename ValType> class DLLExport SingleValueParameter : public ImplicitFunctionParameter {
public:
  using ValueType = ValType;
  SingleValueParameter(ValType value);
  SingleValueParameter();
  SingleValueParameter(const SingleValueParameter<Derived, ValType> &other);
  SingleValueParameter<Derived, ValType> &operator=(const SingleValueParameter<Derived, ValType> &other);
  std::string toXMLString() const override;
  Derived &operator=(const Derived &other);
  bool operator==(const Derived &other) const;
  bool operator!=(const Derived &other) const;
  ValType getValue() const;
  bool isValid() const override;

protected:
  ValType m_value;
  bool m_isValid;
};

//----------------------------------------------------------------
/* Getter for the valid state.
@return raw underlying value.
*/
template <typename Derived, typename ValType> ValType SingleValueParameter<Derived, ValType>::getValue() const {
  return m_value;
}

//----------------------------------------------------------------
/* Getter for the valid state.
@return true if the object is in a valid state.
*/
template <typename Derived, typename ValType> bool SingleValueParameter<Derived, ValType>::isValid() const {
  return m_isValid;
}

//----------------------------------------------------------------
/* Overloaded assignement operator. Utilises CRTP to peform assignements in
terms of derived class.
@param other : object to use as origin for assignment.
@return Ref to assigned object.
*/
template <typename Derived, typename ValType>
Derived &SingleValueParameter<Derived, ValType>::operator=(const Derived &other) {
  if (&other != this) {
    this->m_isValid = other.m_isValid;
    this->m_value = other.m_value;
  }
  return *(dynamic_cast<Derived *>(this));
}

//----------------------------------------------------------------
/* Overloaded equals operator.
@param other : object to use as rhs of comparison.
@return true only if the two objects are considered equal.
*/
template <typename Derived, typename ValType>
bool SingleValueParameter<Derived, ValType>::operator==(const Derived &other) const {
  return other.m_value == this->m_value;
}

//----------------------------------------------------------------
/* Overloaded not equals operator.
@param other : object to use as rhs of comparison.
@return true only if the two objects are not considered equal.
*/
template <typename Derived, typename ValType>
bool SingleValueParameter<Derived, ValType>::operator!=(const Derived &other) const {
  return !(*this == other);
}

//----------------------------------------------------------------
/* Copy constructor
@param other : ref to object to use as origin for the new instance.
*/
template <typename Derived, typename ValType>
SingleValueParameter<Derived, ValType>::SingleValueParameter(const SingleValueParameter<Derived, ValType> &other) {
  *this = other;
}

template <typename Derived, typename ValType>
SingleValueParameter<Derived, ValType> &
SingleValueParameter<Derived, ValType>::operator=(const SingleValueParameter<Derived, ValType> &other) {
  m_value = other.m_value;
  m_isValid = other.m_isValid;
  return *this;
}

/// Default constructor. Object is created in invalid state.
template <typename Derived, typename ValType>
SingleValueParameter<Derived, ValType>::SingleValueParameter() : m_isValid(false) {}

//----------------------------------------------------------------
/* Constructor. Leads to a valid instance/state.
@param value : Internal value for the object to wrap.
*/
template <typename Derived, typename ValType>
SingleValueParameter<Derived, ValType>::SingleValueParameter(ValType value) : m_value(value), m_isValid(true) {}

//----------------------------------------------------------------
/* Serializes the object to an xml string.
@return xml string containing the underlying value.
*/
template <typename Derived, typename ValType> std::string SingleValueParameter<Derived, ValType>::toXMLString() const {
  std::string valueXMLtext = ElementTraits<ValType>::format(m_value);
  return this->parameterXMLTemplate(valueXMLtext);
}

//-----------------------------------------------------------------------------------------------------------------//
// Macro for generating concrete types of SingleValueParameters.
//
// Use of macro enables parameter names to be assigned to each type.
// Most of the work is done in the VectorParamter base class, which utilises
// CRTP.
//
//-----------------------------------------------------------------------------------------------------------------//
#define DECLARE_SINGLE_VALUE_PARAMETER(classname, type_)                                                               \
  class classname : public Mantid::API::SingleValueParameter<classname, type_> {                                       \
  public:                                                                                                              \
    using SuperType = Mantid::API::SingleValueParameter<classname, type_>;                                             \
    static std::string parameterName() { return #classname; }                                                          \
    classname(type_ value) : SuperType(value) {}                                                                       \
    classname() : SuperType() {}                                                                                       \
    std::string getName() const override { return #classname; }                                                        \
    classname *clone() const override { return new classname(m_value); }                                               \
  };
} // namespace API
} // namespace Mantid
