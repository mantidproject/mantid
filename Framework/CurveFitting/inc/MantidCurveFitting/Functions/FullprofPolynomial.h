// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidCurveFitting/DllConfig.h"
#include "MantidCurveFitting/Functions/BackgroundFunction.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/** FullprofPolynomial : Polynomial background defined in Fullprof
 */
class MANTID_CURVEFITTING_DLL FullprofPolynomial : public BackgroundFunction {
public:
  FullprofPolynomial();

  /// Overwrite IFunction base class
  std::string name() const override { return "FullprofPolynomial"; }

  void function1D(double *out, const double *xValues, const size_t nData) const override;

  void functionDeriv1D(API::Jacobian *out, const double *xValues, const size_t nData) override;

  // virtual void functionLocal(std::vector<double> &out, std::vector<double>
  // xValues) const;

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

  /// Background origin position
  double m_bkpos;
};

using FullprofPolynomial_sptr = std::shared_ptr<FullprofPolynomial>;

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
