// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/DllConfig.h"
#include "MantidCurveFitting/Functions/TabulatedFunction.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
Resolution function. It is implemented in terms of TabulatedFunction but doesn't
inherit form it.
It is done to make Resolution parameterless and at the same time use
TabulatedFunction's attributes.

@author Roman Tolchenov, Tessella plc
@date 12/02/2010
*/
class MANTID_CURVEFITTING_DLL Resolution : public API::ParamFunction, public API::IFunction1D {
public:
  /// Constructor
  Resolution();

  /// overwrite IFunction base class methods
  std::string name() const override { return "Resolution"; }
  /// Function values
  void function1D(double *out, const double *xValues, const size_t nData) const override;
  ///  function derivatives
  void functionDeriv1D(API::Jacobian *out, const double *xValues, const size_t nData) override;
  /// Returns the number of attributes associated with the function
  size_t nAttributes() const override;
  /// Returns a list of attribute names
  std::vector<std::string> getAttributeNames() const override;
  /// Return a value of attribute attName
  Attribute getAttribute(const std::string &attName) const override;
  /// Set a value to attribute attName
  void setAttribute(const std::string &attName, const Attribute &) override;
  /// Check if attribute attName exists
  bool hasAttribute(const std::string &attName) const override;
  // return attribute name from ith attribute
  std::string attributeName(size_t index) const override;

private:
  /// Function that does the actual job
  TabulatedFunction m_fun;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
