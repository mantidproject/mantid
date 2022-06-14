// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IFunction1D.h"
#include <cmath>

namespace Mantid {
namespace API {

template <typename EvaluationMethod>
void IFunction1D::calcNumericalDerivative1D(Jacobian *jacobian, EvaluationMethod eval1D, const double *xValues,
                                            const size_t nData) {
  /*
   * Uses the knowledge that this is a simple 1D domain to calculate the
   * derivative. The domains are not constructed and instead function1D
   * is called directly.
   *
   * There is a similar more generic method for all functions in IFunction
   * but the method takes different parameters and uses slightly different
   * function calls in places making it difficult to share code. Please also
   * consider that method when updating this.
   */
  using std::fabs;

  applyTies(); // just in case
  std::vector<double> minusStep(nData), plusStep(nData);
  eval1D(minusStep.data(), xValues, nData);

  double step;
  const size_t nParam = nParams();
  for (size_t iP = 0; iP < nParam; iP++) {
    if (isActive(iP)) {
      const double val = activeParameter(iP);
      step = calculateStepSize(val);

      const double paramPstep = val + step;
      setActiveParameter(iP, paramPstep);
      applyTies();
      eval1D(plusStep.data(), xValues, nData);
      setActiveParameter(iP, val);
      applyTies();

      step = paramPstep - val;
      for (size_t i = 0; i < nData; i++) {
        jacobian->set(i, iP, (plusStep[i] - minusStep[i]) / step);
      }
    }
  }
}
} // namespace API
} // namespace Mantid
