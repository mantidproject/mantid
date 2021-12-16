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
Provide Critical peak of relaxation rate for fitting interface to IFunction.
I.e. y = Sc/(|x-Tc|)^a + Bg
Activation attributes:
<UL>
<LI> Sc - Scaling</LI>
<LI> Tc - critical temperature</LI>
<LI> a - critical exponent </LI>
<LI> Bg - non-critical background</LI>
</UL>

CriticalPeakRelaxationRate parameters:
<UL>
<LI> Scaling - coefficient for scaling (default 1.0)</LI>
<LI> CriticalTemp - coefficient for barrier energy (default 0.01)</LI>
<LI> Exponent - coefficient for critical exponent (default 1.0)</LI>
<LI> Background1 - coefficient for non-critical background when x < Critical Temperature (default 0.0)</LI>
<LI> Background2 - coefficient for non-critical background when x > Critical Temperature (default 0.0)</LI>
<LI> Delta - coefficient to determine distance from the peak to start/end fitting </LI>
</UL>
*/

class MANTID_CURVEFITTING_DLL CriticalPeakRelaxationRate : public BackgroundFunction {
public:
  /// overwrite IFunction base class methods
  std::string name() const override { return "CriticalPeakRelaxationRate"; }
  void function1D(double *out, const double *xValues, const size_t nData) const override;
  const std::string category() const override { return "Muon\\MuonModelling\\Magnetism"; }

protected:
  /// overwrite IFunction base class method, which declare function parameters
  void init() override;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
