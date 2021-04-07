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
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidCurveFitting/DllConfig.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
Provide oscillating exponential decay function:
h*exp(-lambda.x)*(cos(2pi*f*x+phi))

 @author Karl Palmen, ISIS, RAL
 @date 06/01/2012
 */

class MANTID_CURVEFITTING_DLL ExpDecayOsc : public API::ParamFunction, public API::IFunction1D {
public:
  /// overwrite IFunction base class methods
  std::string name() const override { return "ExpDecayOsc"; }

  /// overwrite IFunction base class methods
  const std::string category() const override { return "Muon\\MuonGeneric"; }

protected:
  void setActiveParameter(size_t i, double value) override;
  void function1D(double *out, const double *xValues, const size_t nData) const override;
  void functionDeriv1D(API::Jacobian *out, const double *xValues, const size_t nData) override;

  /// overwrite IFunction base class method that declares function parameters
  void init() override;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
