#include "MantidSINQ/PoldiUtilities/PoldiSpectrumPawleyFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionValues.h"

namespace Mantid {
namespace Poldi {

using namespace API;

/// Default constructor
PoldiSpectrumPawleyFunction::PoldiSpectrumPawleyFunction()
    : PoldiSpectrumDomainFunction(), m_pawleyFunction() {}

/// This function does nothing to prevent setting the workspace on the wrapped
/// function (unit conversion will not work and is not needed).
void PoldiSpectrumPawleyFunction::setMatrixWorkspace(
    boost::shared_ptr<const MatrixWorkspace> workspace, size_t wi,
    double startX, double endX) {
  UNUSED_ARG(workspace);
  UNUSED_ARG(wi);
  UNUSED_ARG(startX);
  UNUSED_ARG(endX);
}

void PoldiSpectrumPawleyFunction::function1DSpectrum(
    const FunctionDomain1DSpectrum &domain, FunctionValues &values) const {
  values.zeroCalculated();

  size_t domainSize = domain.size();
  size_t index = domain.getWorkspaceIndex();
  Poldi2DHelper_sptr helper = m_2dHelpers[index];

  if (helper) {
    FunctionValues localValues(*helper->domain);

    for (size_t i = 0; i < helper->dOffsets.size(); ++i) {
      double newDOffset =
          helper->dOffsets[i] * helper->deltaD + helper->dFractionalOffsets[i];
      m_pawleyFunction->setParameter("f0.ZeroShift", newDOffset);

      size_t baseOffset = helper->minTOFN;

      m_pawleyFunction->function(*(helper->domain), localValues);

      for (size_t j = 0; j < localValues.size(); ++j) {
        values.addToCalculated((j + baseOffset) % domainSize,
                               localValues[j] * helper->factors[j]);
      }
    }

    m_pawleyFunction->setParameter("f0.ZeroShift", 0.0);
  }
}

void PoldiSpectrumPawleyFunction::functionDeriv1DSpectrum(
    const FunctionDomain1DSpectrum &domain, Jacobian &jacobian) {
  size_t index = domain.getWorkspaceIndex();
  Poldi2DHelper_sptr helper = m_2dHelpers[index];

  if (helper) {
    WrapAroundJacobian localJacobian(jacobian, helper->minTOFN, helper->factors,
                                     0, domain.size());
    for (size_t i = 0; i < helper->dOffsets.size(); ++i) {
      double newDOffset =
          helper->dOffsets[i] * helper->deltaD + helper->dFractionalOffsets[i];
      m_pawleyFunction->setParameter("f0.ZeroShift", newDOffset);

      m_pawleyFunction->functionDeriv(*(helper->domain), localJacobian);
    }

    m_pawleyFunction->setParameter("f0.ZeroShift", 0.0);
  }
}

void
PoldiSpectrumPawleyFunction::poldiFunction1D(const std::vector<int> &indices,
                                             const FunctionDomain1D &domain,
                                             FunctionValues &values) const {}

IPawleyFunction_sptr PoldiSpectrumPawleyFunction::getPawleyFunction() const {
  return m_pawleyFunction;
}

void PoldiSpectrumPawleyFunction::beforeDecoratedFunctionSet(
    const IFunction_sptr &fn) {
  IPawleyFunction_sptr pawleyFunction =
      boost::dynamic_pointer_cast<IPawleyFunction>(fn);

  if (!pawleyFunction) {
    throw std::invalid_argument("Function is not a pawley function.");
  }

  m_pawleyFunction = pawleyFunction;
  m_pawleyFunction->fix(m_pawleyFunction->parameterIndex("f0.ZeroShift"));
}

DECLARE_FUNCTION(PoldiSpectrumPawleyFunction)

} // namespace Poldi
} // namespace Mantid
