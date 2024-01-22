// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidSINQ/PoldiUtilities/PoldiSpectrumPawleyFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionValues.h"

namespace Mantid::Poldi {

using namespace API;

/// Default constructor
PoldiSpectrumPawleyFunction::PoldiSpectrumPawleyFunction() : PoldiSpectrumDomainFunction(), m_pawleyFunction() {}

/// This function does nothing to prevent setting the workspace on the wrapped
/// function (unit conversion will not work and is not needed).
void PoldiSpectrumPawleyFunction::setMatrixWorkspace(std::shared_ptr<const MatrixWorkspace> workspace, size_t wi,
                                                     double startX, double endX) {
  UNUSED_ARG(workspace);
  UNUSED_ARG(wi);
  UNUSED_ARG(startX);
  UNUSED_ARG(endX);
}

/// Implementation of function1DSpectrum that transforms PawleyFunction output.
void PoldiSpectrumPawleyFunction::function1DSpectrum(const FunctionDomain1DSpectrum &domain,
                                                     FunctionValues &values) const {
  values.zeroCalculated();

  size_t index = domain.getWorkspaceIndex();
  Poldi2DHelper_sptr helper = m_2dHelpers[index];

  if (helper) {
    for (size_t i = 0; i < helper->dOffsets.size(); ++i) {
      double newDOffset = helper->dOffsets[i] * helper->deltaD + helper->dFractionalOffsets[i];
      m_pawleyFunction->setParameter("f0.ZeroShift", newDOffset);

      size_t baseOffset = helper->minTOFN;

      m_pawleyFunction->function(*(helper->domain), helper->values);

      for (size_t j = 0; j < helper->values.size(); ++j) {
        values.addToCalculated((j + baseOffset) % domain.size(), helper->values[j] * helper->factors[j]);
      }
    }

    m_pawleyFunction->setParameter("f0.ZeroShift", 0.0);
  }
}

/// Using numerical derivatives turned out to be faster for this case.
void PoldiSpectrumPawleyFunction::functionDeriv1DSpectrum(const FunctionDomain1DSpectrum &domain, Jacobian &jacobian) {
  calNumericalDeriv(domain, jacobian);
}

/// Calculate 1D function by adding the functions calculated for each index.
void PoldiSpectrumPawleyFunction::poldiFunction1D(const std::vector<int> &indices, const FunctionDomain1D &domain,
                                                  FunctionValues &values) const {
  FunctionValues localValues(domain);

  m_pawleyFunction->function(domain, localValues);

  auto chopperSlitCount = static_cast<double>(m_chopperSlitOffsets.size());

  for (auto index : indices) {
    std::vector<double> factors(domain.size());

    for (size_t i = 0; i < factors.size(); ++i) {
      values.addToCalculated(i, chopperSlitCount * localValues[i] *
                                    m_timeTransformer->detectorElementIntensity(domain[i], static_cast<size_t>(index)));
    }
  }
}

/// Returns the internally stored Pawley function.
IPawleyFunction_sptr PoldiSpectrumPawleyFunction::getPawleyFunction() const { return m_pawleyFunction; }

/// Makes sure that the decorated function is of the right type.
void PoldiSpectrumPawleyFunction::beforeDecoratedFunctionSet(const IFunction_sptr &fn) {
  IPawleyFunction_sptr pawleyFunction = std::dynamic_pointer_cast<IPawleyFunction>(fn);

  if (!pawleyFunction) {
    throw std::invalid_argument("Function is not a pawley function.");
  }

  m_pawleyFunction = pawleyFunction;
}

DECLARE_FUNCTION(PoldiSpectrumPawleyFunction)

} // namespace Mantid::Poldi
