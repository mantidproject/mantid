// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/CompositeFunction.h"

#include <map>

namespace Mantid {
namespace API {
/**
    Immutable composite function is a composite function which members cannot be
   added or removed
    after creation. Only a derived class can add functions in its contructor (or
   methods).
    The function factory treat an ImmutableCompositeFunction as a simple
   function.
*/
class MANTID_API_DLL ImmutableCompositeFunction : public CompositeFunction {
public:
  /* Overriden methods */

  /// Returns the function's name
  std::string name() const override { return "ImmutableCompositeFunction"; }
  /// Set i-th parameter
  void setParameter(size_t i, const double &value, bool explicitlySet = true) override {
    CompositeFunction::setParameter(i, value, explicitlySet);
  }
  /// Set i-th parameter description
  void setParameterDescription(size_t i, const std::string &description) override {
    CompositeFunction::setParameterDescription(i, description);
  }
  /// Set parameter by name.
  void setParameter(const std::string &name, const double &value, bool explicitlySet = true) override;
  /// Set description of parameter by name.
  void setParameterDescription(const std::string &name, const std::string &description) override;
  /// Get i-th parameter
  double getParameter(size_t i) const override { return CompositeFunction::getParameter(i); }
  /// Get parameter by name.
  double getParameter(const std::string &name) const override;
  /// Returns the index of parameter name
  size_t parameterIndex(const std::string &name) const override;
  /// Returns the name of parameter i
  std::string parameterName(size_t i) const override;

protected:
  /// Make it protected
  using CompositeFunction::addFunction;
  /// Overload addFunction to take a bare pointer
  void addFunction(IFunction *fun);
  /// Define an alias for a parameter
  void setAlias(const std::string &parName, const std::string &alias);
  /// Add default ties
  void addDefaultTies(const std::string &ties);
  /// Add default constraints
  void addDefaultConstraints(const std::string &constraints);
  /// Writes itself into a string
  std::string writeToString(const std::string &parentLocalAttributesStr = "") const override;

private:
  /// Keep paramater aliases
  std::map<std::string, size_t> m_aliases;
};

} // namespace API
} // namespace Mantid
