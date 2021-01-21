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
Provide Log Normal function: h*exp(-(log(x)-t)^2 / (2*b^2) )/x

@author Jose Borreguero, NScD
@date 11/14/2011
*/
class MANTID_CURVEFITTING_DLL LogNormal : public API::ParamFunction, public API::IFunction1D {
public:
  /// Constructor
  LogNormal();

  /// overwrite IFunction base class methods
  std::string name() const override { return "LogNormal"; }
  const std::string category() const override { return "Peak"; }

protected:
  void function1D(double *out, const double *xValues, const size_t nData) const override;
  void functionDeriv1D(API::Jacobian *out, const double *xValues, const size_t nData) override;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
