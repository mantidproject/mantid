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
Provide Power Law function interface to IFunction.
I.e. the function: y = ax^b +c.
PowerLaw parameters:
<UL>
<LI> Magnitude - coefficient for linear term (default 1.0)</LI>
<LI> Exponent - exponent (default 1.0)</LI>
<LI> Constant - coefficient for constant term (default 0.0)</LI>
</UL>
*/

class MANTID_CURVEFITTING_DLL PowerLaw : public BackgroundFunction {
public:
  /// overwrite IFunction base class methods
  std::string name() const override { return "PowerLaw"; }
  void function1D(double *out, const double *xValues, const size_t nData) const override;
  void functionDeriv1D(API::Jacobian *out, const double *xValues, const size_t nData) override;
  const std::string category() const override { return "General; Muon\\MuonModelling"; }

protected:
  /// overwrite IFunction base class method, which declare function parameters
  void init() override;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
