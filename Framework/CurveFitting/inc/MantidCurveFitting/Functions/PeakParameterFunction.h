// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FunctionParameterDecorator.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidCurveFitting/DllConfig.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/** PeakParameterFunction :

  This function implements API::FunctionParameterDecorator to wrap an
  IPeakFunction. The function expects a FunctionDomain1D with size exactly 4,
  corresponding to the 4 special parameters centre, height, fwhm and intensity.

  They are stored in the output values in that order. Calculating the derivative
  of the function yields the partial derivatives of these 4 parameters with
  respect to the function's native parameters defined through declareParameter.

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 24/02/2015
*/
class MANTID_CURVEFITTING_DLL PeakParameterFunction : virtual public API::IFunction1D,
                                                      virtual public API::FunctionParameterDecorator {
public:
  PeakParameterFunction() : FunctionParameterDecorator() {}

  std::string name() const override { return "PeakParameterFunction"; }

  void function1D(double *out, const double *xValues, const size_t nData) const override;

  void functionDeriv(const API::FunctionDomain &domain, API::Jacobian &jacobian) override;

protected:
  void beforeDecoratedFunctionSet(const API::IFunction_sptr &fn) override;

  API::IPeakFunction_sptr m_peakFunction;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
