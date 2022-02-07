// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFunction1DSpectrum.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidSINQ/DllConfig.h"
#include "MantidSINQ/PoldiUtilities/IPoldiFunction1D.h"

namespace Mantid {
namespace Poldi {

/** PoldiSpectrumLinearBackground :

    A function that is defined like this:

        f(x) = A1 * wi

    where wi is the workspace index and A1 is the only parameter
    of the function. Since it's derived from IFunction1DSpectrum,
    it works only on the proper domain.

        @author Michael Wedel, Paul Scherrer Institut - SINQ
        @date 06/08/2014
  */
class MANTID_SINQ_DLL PoldiSpectrumLinearBackground : public API::ParamFunction,
                                                      public API::IFunction1DSpectrum,
                                                      public IPoldiFunction1D {
public:
  PoldiSpectrumLinearBackground();

  std::string name() const override { return "PoldiSpectrumLinearBackground"; }

  void setWorkspace(std::shared_ptr<const API::Workspace> ws) override;
  size_t getTimeBinCount() const;

  void function1DSpectrum(const API::FunctionDomain1DSpectrum &domain, API::FunctionValues &values) const override;
  void functionDeriv1DSpectrum(const API::FunctionDomain1DSpectrum &domain, API::Jacobian &jacobian) override;

  void poldiFunction1D(const std::vector<int> &indices, const API::FunctionDomain1D &domain,
                       API::FunctionValues &values) const override;

protected:
  void init() override;

  size_t m_timeBinCount;
};

} // namespace Poldi
} // namespace Mantid
