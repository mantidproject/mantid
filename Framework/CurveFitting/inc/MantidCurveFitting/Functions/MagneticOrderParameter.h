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
Provide Magnetic Order Paramtere fit function interface to IFunction.
I.e. the function:y = A0(1-(x/Tc)^alpha)^beta.
Where A0 = amplitude
alpha = balance parameter
beta = critical exponent
Tc = critical temperature

MagneticOrderParameter parameters:
<UL>
<LI> A0 - amplitude (default 1.0)</LI>
<LI> Alpha - the balance parameter (default 2.0)</LI>
<LI> Beta - the critical exponent (default 0.5)</LI>
<LI> CriticalTemp - the critical temperature (default 1.0)</LI>
</UL>
*/

class MANTID_CURVEFITTING_DLL MagneticOrderParameter : public BackgroundFunction {
public:
  /// overwrite IFunction base class methods
  std::string name() const override { return "MagneticOrderParameter"; }
  void function1D(double *out, const double *xValues, const size_t nData) const override;
  void functionDeriv1D(API::Jacobian *out, const double *xValues, const size_t nData) override;
  const std::string category() const override { return "Muon\\MuonModelling\\Magnetism"; }

protected:
  /// overwrite IFunction base class method, which declare function parameters
  void init() override;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
