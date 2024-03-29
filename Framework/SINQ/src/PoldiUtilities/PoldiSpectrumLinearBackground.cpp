// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidSINQ/PoldiUtilities/PoldiSpectrumLinearBackground.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid::Poldi {

using namespace API;

DECLARE_FUNCTION(PoldiSpectrumLinearBackground)

/// Default constructor
PoldiSpectrumLinearBackground::PoldiSpectrumLinearBackground()
    : ParamFunction(), IFunction1DSpectrum(), IPoldiFunction1D(), m_timeBinCount(0) {}

void PoldiSpectrumLinearBackground::setWorkspace(std::shared_ptr<const Workspace> ws) {
  MatrixWorkspace_const_sptr matrixWs = std::dynamic_pointer_cast<const MatrixWorkspace>(ws);

  if (matrixWs && matrixWs->getNumberHistograms() > 0) {
    m_timeBinCount = matrixWs->x(0).size();
  }
}

size_t PoldiSpectrumLinearBackground::getTimeBinCount() const { return m_timeBinCount; }

/// Calculates the function values as f(x) = A1 * wi
void PoldiSpectrumLinearBackground::function1DSpectrum(const FunctionDomain1DSpectrum &domain,
                                                       FunctionValues &values) const {
  auto wsIndexDouble = static_cast<double>(domain.getWorkspaceIndex());

  values.setCalculated(wsIndexDouble * getParameter(0));
}

/// Sets the Jacobian, which is wi at any point.
void PoldiSpectrumLinearBackground::functionDeriv1DSpectrum(const FunctionDomain1DSpectrum &domain,
                                                            Jacobian &jacobian) {
  auto wsIndexDouble = static_cast<double>(domain.getWorkspaceIndex());

  for (size_t i = 0; i < domain.size(); ++i) {
    jacobian.set(i, 0, wsIndexDouble);
  }
}

void PoldiSpectrumLinearBackground::poldiFunction1D(const std::vector<int> &indices, const FunctionDomain1D &domain,
                                                    FunctionValues &values) const {
  double backgroundDetector = getParameter(0);
  auto wireCount = static_cast<double>(indices.size());
  double distributionFactor =
      wireCount * wireCount * static_cast<double>(m_timeBinCount) / (2.0 * static_cast<double>(domain.size()));
  double backgroundD = backgroundDetector * distributionFactor;

  for (size_t i = 0; i < values.size(); ++i) {
    values.addToCalculated(i, backgroundD);
  }
}

/// Declares the function's single parameter A1.
void PoldiSpectrumLinearBackground::init() { declareParameter("A1"); }

} // namespace Mantid::Poldi
