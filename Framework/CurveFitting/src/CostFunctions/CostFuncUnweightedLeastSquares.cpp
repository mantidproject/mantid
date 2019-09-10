// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/CostFunctions/CostFuncUnweightedLeastSquares.h"

#include "MantidKernel/Logger.h"
#include <cmath>

namespace Mantid {
namespace CurveFitting {
namespace CostFunctions {

namespace {
/// static logger
Kernel::Logger g_log("CostFuncUnweightedLeastSquares");
} // namespace

DECLARE_COSTFUNCTION(CostFuncUnweightedLeastSquares, Unweighted least squares)

CostFuncUnweightedLeastSquares::CostFuncUnweightedLeastSquares()
    : CostFuncLeastSquares() {}

void CostFuncUnweightedLeastSquares::calActiveCovarianceMatrix(GSLMatrix &covar,
                                                               double epsrel) {
  CostFuncLeastSquares::calActiveCovarianceMatrix(covar, epsrel);

  double variance = getResidualVariance();
  covar *= variance;

  if (g_log.is(Kernel::Logger::Priority::PRIO_DEBUG)) {
    g_log.debug() << "== Final covariance matrix (H^-1) ==\n";

    std::ios::fmtflags prevState = g_log.debug().flags();
    g_log.debug() << std::left << std::fixed;

    for (size_t i = 0; i < covar.size1(); ++i) {
      for (size_t j = 0; j < covar.size2(); ++j) {
        g_log.debug() << std::setw(10);
        g_log.debug() << covar.get(i, j) << "  ";
      }
      g_log.debug() << '\n';
    }
    g_log.debug().flags(prevState);
  }
}

/// Return unit weights for all data points.
std::vector<double> CostFuncUnweightedLeastSquares::getFitWeights(
    API::FunctionValues_sptr values) const {
  std::vector<double> weights(values->size());
  for (size_t i = 0; i < weights.size(); ++i) {
    weights[i] = values->getFitWeight(i) != 0 ? 1 : 0;
  }

  return weights;
}

/// Calculates the residual variance from the internally stored FunctionValues.
double CostFuncUnweightedLeastSquares::getResidualVariance() const {
  if (!m_values || m_values->size() == 0) {
    return 0;
  }

  double sum = 0.0;
  for (size_t i = 0; i < m_values->size(); ++i) {
    double difference = m_values->getCalculated(i) - m_values->getFitData(i);
    sum += difference * difference;
  }

  auto degreesOfFreedom = static_cast<double>(m_values->size() - nParams());
  double residualVariance = sum / degreesOfFreedom;

  if (g_log.is(Kernel::Logger::Priority::PRIO_DEBUG)) {
    g_log.debug() << "== Statistics of residuals ==\n";
    std::ios::fmtflags prevState = g_log.debug().flags();
    g_log.debug() << std::left << std::fixed << std::setw(10);
    g_log.debug() << "Residual sum of squares: " << sum << '\n';
    g_log.debug() << "Residual variance: " << residualVariance << '\n';
    g_log.debug() << "Residual standard deviation: " << sqrt(residualVariance)
                  << '\n';
    g_log.debug() << "Degrees of freedom: "
                  << static_cast<size_t>(degreesOfFreedom) << '\n';
    g_log.debug() << "Number of observations: " << m_values->size() << '\n';
    g_log.debug().flags(prevState);
  }

  return residualVariance;
}

} // namespace CostFunctions
} // namespace CurveFitting
} // namespace Mantid
