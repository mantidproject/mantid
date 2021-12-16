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
Provide Decoupling of asymmetry in the ordered state of a powdered magnet for fitting function
interface to IFunction.
I.e. the function: y = Asym*(1-A_z(x/CharField)).
Where b = x/CharField and
A_z(b) = (3/4) - (1/4*b^2)+((b^2-1)^2)/(8*b^3)*log(|(b+1)/(b-1)|)
DecouplingAsymPowderMag parameters:
<UL>
<LI> Asymm - a scaling parameter for the overall asymmetry (default 1.0)</LI>
<LI> CharField - the characteristic field (default 1.0)</LI>
</UL>
*/

class MANTID_CURVEFITTING_DLL DecoupAsymPowderMagRot : public BackgroundFunction {
public:
  /// overwrite IFunction base class methods
  std::string name() const override { return "DecoupAsymPowderMagRot"; }
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
