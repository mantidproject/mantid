// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/DllConfig.h"

#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/IFunction.h"
#include "MantidKernel/Logger.h"

namespace Mantid {
namespace API {

/** IFunction1DSpectrum :

    IFunction1DSpectrum is very similar to IFunction1D, it just builds on
    FunctionDomain1DSpectrum, which is more specialized than FunctionDomain1D.

      @author Michael Wedel, Paul Scherrer Institut - SINQ
      @date 03/06/2014
  */
class MANTID_API_DLL IFunction1DSpectrum : public virtual IFunction {
public:
  void function(const FunctionDomain &domain, FunctionValues &values) const override;
  void functionDeriv(const FunctionDomain &domain, Jacobian &jacobian) override;

  /// Provide a concrete function in an implementation that operates on a
  /// FunctionDomain1DSpectrum.
  virtual void function1DSpectrum(const FunctionDomain1DSpectrum &domain, FunctionValues &values) const = 0;

  /// Derivatives of the function. The base implementation calculates numerical
  /// derivatives.
  virtual void functionDeriv1DSpectrum(const FunctionDomain1DSpectrum &domain, Jacobian &jacobian);

protected:
  static Kernel::Logger g_log;
};

} // namespace API
} // namespace Mantid
