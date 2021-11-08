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
Provide Smooth Transition function interface to IFunction.
I.e. the function: y = A2 + (A1-A2)/(exp((x-MidPoint)/GrowthRate)+1)
Smooth Transition parameters:
<UL>
<LI> A1 - the limit of the function as x tends to zero (default 0.0)</LI>
<LI> A2 - the limit of the function as x tends to infinity (default 0.1)</LI>
<LI> Midpoint - sigmoid midpoint (default 100.0)</LI>
<LI> GrowthRate - growth rate (default 1.0)</LI>
</UL>
*/

class MANTID_CURVEFITTING_DLL SmoothTransition : public BackgroundFunction {
public:
  /// overwrite IFunction base class methods
  std::string name() const override { return "SmoothTransition"; }
  void function1D(double *out, const double *xValues, const size_t nData) const override;
  void functionDeriv1D(API::Jacobian *out, const double *xValues, const size_t nData) override;
  const std::string category() const override { return "Muon\\MuonModelling"; }

protected:
  /// overwrite IFunction base class method, which declare function parameters
  void init() override;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
