// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/IFunction1DSpectrum.h"
#include "MantidSINQ/DllConfig.h"
#include "MantidSINQ/PoldiUtilities/IPoldiFunction1D.h"

namespace Mantid {
namespace Poldi {

/** Poldi2DFunction :

    Function for POLDI 2D spectrum. It implements CompositeFunction in order to
    combine functions for different peaks and IFunction1DSpectrum so that Fit
    is able to choose the correct domain creator for it.

      @author Michael Wedel, Paul Scherrer Institut - SINQ
      @date 13/06/2014
  */
class MANTID_SINQ_DLL Poldi2DFunction : public API::IFunction1DSpectrum,
                                        public API::CompositeFunction,
                                        public IPoldiFunction1D {
public:
  Poldi2DFunction();

  void function(const API::FunctionDomain &domain, API::FunctionValues &values) const override;
  void functionDeriv(const API::FunctionDomain &domain, API::Jacobian &jacobian) override;

  void function1DSpectrum(const API::FunctionDomain1DSpectrum &domain, API::FunctionValues &values) const override;

  void poldiFunction1D(const std::vector<int> &indices, const API::FunctionDomain1D &domain,
                       API::FunctionValues &values) const override;

  void iterationFinished() override;

private:
  size_t m_iteration;
};

using Poldi2DFunction_sptr = std::shared_ptr<Poldi2DFunction>;

} // namespace Poldi
} // namespace Mantid
