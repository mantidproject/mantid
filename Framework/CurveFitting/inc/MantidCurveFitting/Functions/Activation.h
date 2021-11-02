// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin
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
Provide Activation fit function interface to IFunction.
I.e. if the unit is K, the function: y = = height*exp(-lifetime/x)
or if the unit is meV, the function: y = height*exp(-e*lifetime/(1000 k_B x))
Activation attributes:
<UL>
<LI> Unit - either K or meV (default K)</LI>
</UL>

Activation parameters:
<UL>
<LI> Height - coefficient for height (default 1.0)</LI>
<LI> Lifetime - coefficient for lifetime (default 1.0)</LI>
</UL>
*/

class MANTID_CURVEFITTING_DLL Activation : public BackgroundFunction {
public:
  /// overwrite IFunction base class methods
  std::string name() const override { return "Activation"; }
  void function1D(double *out, const double *xValues, const size_t nData) const override;
  void functionDeriv1D(API::Jacobian *out, const double *xValues, const size_t nData) override;
  const std::string category() const override { return "Muon\\MuonModelling"; }
  void beforeFunctionSet() const;

protected:
  /// overwrite IFunction base class method, which declare function parameters
  void init() override;

private:
  double getMeVConv() const;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
