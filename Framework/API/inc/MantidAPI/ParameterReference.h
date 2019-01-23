// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_PARAMETERREFERENCE_H_
#define MANTID_API_PARAMETERREFERENCE_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/DllConfig.h"
#include <string>

namespace Mantid {
namespace API {
class IFunction;
/**
    A reference to a parameter in a function. To uniquely identify a parameter
    in a composite function

    @author Roman Tolchenov, Tessella Support Services plc
    @date 26/02/2010
  */
class MANTID_API_DLL ParameterReference {
public:
  ParameterReference();
  ParameterReference(IFunction *fun, std::size_t index, bool isDefault = false);
  void setParameter(const double &value, bool isExplicitlySet = true);
  double getParameter() const;
  bool isDefault() const;
  bool isParameterOf(const IFunction *fun) const;
  virtual ~ParameterReference() = default;
  IFunction *getLocalFunction() const;
  std::size_t getLocalIndex() const;
  std::size_t parameterIndex() const;
  std::string parameterName() const;
  IFunction *ownerFunction() const;

protected:
  void reset(IFunction *fun, std::size_t index, bool isDefault = false);

private:
  /// Function-owner of this reference. parameterName() and parameterIndex()
  /// return values relative to this function.
  IFunction *m_owner;
  /// Function that together with m_index uniquely identify the parameter.
  IFunction *m_function;
  /// Index of the parameter in m_function. It is assumed that this index
  /// uniquely identifies the parameter withing m_function
  std::size_t m_index;
  /// Flag to mark as default the value of an object associated with this
  /// reference: a tie or a constraint.
  bool m_isDefault;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_PARAMETERREFERENCE_H_*/
