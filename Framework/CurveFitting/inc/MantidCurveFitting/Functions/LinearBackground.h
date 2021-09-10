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

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
Provide linear function interface to IFunction.
I.e. the function: A0+A1*x.

LinearBackground parameters:
<UL>
<LI> A0 - coefficient for constant term (default 0.0)</LI>
<LI> A1 - coefficient for linear term (default 0.0)</LI>
</UL>

@author Anders Markvardsen, ISIS, RAL
@date 23/10/2009
*/
class MANTID_CURVEFITTING_DLL LinearBackground : public BackgroundFunction {
public:
  /// overwrite IFunction base class methods
  std::string name() const override { return "LinearBackground"; }
  void function1D(double *out, const double *xValues, const size_t nData) const override;
  void functionDeriv1D(API::Jacobian *out, const double *xValues, const size_t nData) override;

  void fit(const std::vector<double> &X, const std::vector<double> &Y) override;

protected:
  /// overwrite IFunction base class method, which declare function parameters
  void init() override;
  /// Calculate histogram data.
  void histogram1D(double *out, double left, const double *right, const size_t nBins) const override;
  /// Devivatives of the histogram.
  void histogramDerivative1D(API::Jacobian *jacobian, double left, const double *right,
                             const size_t nBins) const override;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
