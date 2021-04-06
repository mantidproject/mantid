// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IPawleyFunction.h"
#include "MantidSINQ/DllConfig.h"
#include "MantidSINQ/PoldiUtilities/PoldiSpectrumDomainFunction.h"

namespace Mantid {
namespace Poldi {

/** PoldiSpectrumPawleyFunction : TODO: DESCRIPTION
 */
class MANTID_SINQ_DLL PoldiSpectrumPawleyFunction : public PoldiSpectrumDomainFunction {
public:
  PoldiSpectrumPawleyFunction();

  std::string name() const override { return "PoldiSpectrumPawleyFunction"; }

  void setMatrixWorkspace(std::shared_ptr<const API::MatrixWorkspace> workspace, size_t wi, double startX,
                          double endX) override;

  void function1DSpectrum(const API::FunctionDomain1DSpectrum &domain, API::FunctionValues &values) const override;
  void functionDeriv1DSpectrum(const API::FunctionDomain1DSpectrum &domain, API::Jacobian &jacobian) override;
  void poldiFunction1D(const std::vector<int> &indices, const API::FunctionDomain1D &domain,
                       API::FunctionValues &values) const override;

  API::IPawleyFunction_sptr getPawleyFunction() const;

protected:
  void beforeDecoratedFunctionSet(const API::IFunction_sptr &fn) override;

  API::IPawleyFunction_sptr m_pawleyFunction;
};

} // namespace Poldi
} // namespace Mantid
