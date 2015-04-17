#include "MantidCurveFitting/CostFuncUnweightedLeastSquares.h"

#include "MantidKernel/Logger.h"
#include <iomanip>
#include <cmath>

namespace Mantid {
namespace CurveFitting {

namespace {
/// static logger
Kernel::Logger g_log("CostFuncUnweightedLeastSquares");
}

DECLARE_COSTFUNCTION(CostFuncUnweightedLeastSquares, Unweighted least squares)

CostFuncUnweightedLeastSquares::CostFuncUnweightedLeastSquares()
    : CostFuncLeastSquares() {}

void CostFuncUnweightedLeastSquares::calActiveCovarianceMatrix(GSLMatrix &covar,
                                                               double epsrel) {
  CostFuncLeastSquares::calActiveCovarianceMatrix(covar, epsrel);

  double variance = getResidualVariance();
  covar *= variance;

  if (g_log.is(Kernel::Logger::Priority::PRIO_INFORMATION)) {
    g_log.information() << "== Final covariance matrix (H^-1) ==" << std::endl;

    std::ios::fmtflags prevState = g_log.information().flags();
    g_log.information() << std::left << std::fixed;

    for (size_t i = 0; i < covar.size1(); ++i) {
      for (size_t j = 0; j < covar.size2(); ++j) {
        g_log.information() << std::setw(10);
        g_log.information() << covar.get(i, j) << "  ";
      }
      g_log.information() << std::endl;
    }
    g_log.information().flags(prevState);
  }
}

/// Return unit weights for all data points.
std::vector<double> CostFuncUnweightedLeastSquares::getFitWeights(
    API::FunctionValues_sptr values) const {
  return std::vector<double>(values->size(), 1.0);
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

  double degreesOfFreedom = static_cast<double>(m_values->size() - nParams());
  double residualVariance = sum / degreesOfFreedom;

  if (g_log.is(Kernel::Logger::Priority::PRIO_INFORMATION)) {
    g_log.information() << "== Statistics of residuals ==" << std::endl;
    std::ios::fmtflags prevState = g_log.information().flags();
    g_log.information() << std::left << std::fixed << std::setw(10);
    g_log.information() << "Residual sum of squares: " << sum << std::endl;
    g_log.information() << "Residual variance: " << residualVariance
                        << std::endl;
    g_log.information() << "Residual standard deviation: "
                        << sqrt(residualVariance) << std::endl;
    g_log.information() << "Degrees of freedom: "
                        << static_cast<size_t>(degreesOfFreedom) << std::endl;
    g_log.information() << "Number of observations: " << m_values->size()
                        << std::endl;
    g_log.information().flags(prevState);
  }

  return residualVariance;
}

} // namespace CurveFitting
} // namespace Mantid
