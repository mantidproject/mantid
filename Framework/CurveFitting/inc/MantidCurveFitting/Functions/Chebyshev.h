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
#include "MantidCurveFitting/Functions/BackgroundFunction.h"
#include <valarray>

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
Implements Chebyshev polynomial expansion.

Attributes: int n - the highest polynomial order.
Parameters: n+1 expansion coefficients \f$a_i\f$ as in expression:
\f$\sum_{i=0}^{n} a_i T_i(x)\f$

Uses the Clenshaw algorithm to evaluate the expansion.

@author Roman Tolchenov, Tessella inc
@date 14/05/2010
*/
class MANTID_CURVEFITTING_DLL Chebyshev : public BackgroundFunction {
public:
  /// Constructor
  Chebyshev();

  /// overwrite IFunction base class methods
  std::string name() const override { return "Chebyshev"; }
  const std::string category() const { return "Background"; }
  void function1D(double *out, const double *xValues, const size_t nData) const override;
  void functionDeriv1D(API::Jacobian *out, const double *xValues, const size_t nData) override;

  /// Set a value to attribute attName
  void setAttribute(const std::string &attName, const Attribute &) override;

private:
  /// Polynomial order
  int m_n;
  /// Lower x boundary. The default is -1
  double m_StartX;
  /// Upper x boundary. The default is  1
  double m_EndX;
  /// Keep intermediate calculatons
  mutable std::valarray<double> m_b;
};

using Chebyshev_sptr = std::shared_ptr<Chebyshev>;

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
