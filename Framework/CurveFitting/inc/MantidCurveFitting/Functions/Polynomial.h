// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidCurveFitting/DllConfig.h"
#include "MantidCurveFitting/Functions/BackgroundFunction.h"
#include <cmath>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/** Polynomial : N-th polynomial background function.
 */
class MANTID_CURVEFITTING_DLL Polynomial : public BackgroundFunction {
public:
  Polynomial();

  /// Overwrite IFunction base class
  std::string name() const override { return "Polynomial"; }

  const std::string category() const override { return "Background; Muon\\MuonModelling"; }

  void function1D(double *out, const double *xValues, const size_t nData) const override;

  void functionDeriv1D(API::Jacobian *out, const double *xValues, const size_t nData) override;

  /// Returns the number of attributes associated with the function (polynomial
  /// order n)
  size_t nAttributes() const override { return 1; }

  /// Returns a list of attribute names
  std::vector<std::string> getAttributeNames() const override;

  /// Return a value of attribute attName
  Attribute getAttribute(const std::string &attName) const override;

  /// Set a value to attribute attName
  void setAttribute(const std::string &attName, const Attribute &) override;

  /// Check if attribute attName exists
  bool hasAttribute(const std::string &attName) const override;

private:
  /// Polynomial order
  int m_n;
};

using Polynomial_sptr = std::shared_ptr<Polynomial>;

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
