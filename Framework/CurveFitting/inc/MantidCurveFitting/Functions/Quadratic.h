// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_QUADRATIC_H_
#define MANTID_CURVEFITTING_QUADRATIC_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/BackgroundFunction.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
Provide quadratic function interface to IFunction.
I.e. the function: A0+A1*x+A2*x^2

Quadratic parameters:
<UL>
<LI> A0 - coefficient for constant term (default 0.0)</LI>
<LI> A1 - coefficient for linear term (default 0.0)</LI>
<LI> A2 - coefficient for quadratic term (default 0.0)</LI>
</UL>

@author Anders Markvardsen, ISIS, RAL
@date 20/10/2009
*/
class DLLExport Quadratic : public BackgroundFunction {
public:
  /// overwrite IFunction base class methods
  std::string name() const override { return "Quadratic"; }
  void function1D(double *out, const double *xValues,
                  const size_t nData) const override;
  void functionDeriv1D(API::Jacobian *out, const double *xValues,
                       const size_t nData) override;

protected:
  /// overwrite IFunction base class method, which declare function parameters
  void init() override;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_QUADRATIC_H_*/
