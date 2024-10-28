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
Provide Muonium-style decoupling curve  function interface to IFunction.
I.e. the function:y = A_R(0.5+(x/B0)^2)/(1+(x/B0)^2)+A_BG.
Where A_R = repolarising asymmetry
B0 = decoupling field
A_BG = background asymmetry

MuoniumDecouplingCurve parameters:
<UL>
<LI> RepolarisingAsymmetry - the asymmetry of the muonium fraction (default 0.2)</LI>
<LI> DecouplingField - the decoupling field. This is related to the hyperfine constant A of the muonium state by A =
B0*(gamma_e+gamma_mu)/2pi (with x and B0 in Gauss, A = 2.81605 * B0 in MHz) (default 500.0)</LI>
<LI> BackgroundAsymmetry - the background asymmetry (default 0.0)</LI>
</UL>
*/

class MANTID_CURVEFITTING_DLL MuoniumDecouplingCurve : public BackgroundFunction {
public:
  /// overwrite IFunction base class methods
  std::string name() const override { return "MuoniumDecouplingCurve"; }
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
