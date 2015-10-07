//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/CostFuncRwp.h"
#include "MantidCurveFitting/Jacobian.h"
#include "MantidCurveFitting/SeqDomain.h"
#include "MantidAPI/IConstraint.h"
#include "MantidAPI/CompositeDomain.h"
#include "MantidAPI/FunctionValues.h"

#include <cmath>
#include <iomanip>

namespace Mantid {
namespace CurveFitting {

DECLARE_COSTFUNCTION(CostFuncRwp, Rwp)

//----------------------------------------------------------------------------------------------
/**
 * Constructor
 */
CostFuncRwp::CostFuncRwp() : CostFuncLeastSquares() {
  m_includePenalty = false;
  m_value = 0.;
  m_pushed = false;
  m_factor = 1.;
}

std::vector<double>
CostFuncRwp::getFitWeights(API::FunctionValues_sptr values) const {
  double sqrtW = calSqrtW(values);

  std::vector<double> weights(values->size());
  for (size_t i = 0; i < weights.size(); ++i) {
    weights[i] = getWeight(values, i, sqrtW);
  }

  return weights;
}

//----------------------------------------------------------------------------------------------
/** Get weight of data point i(1/sigma)
  */
double CostFuncRwp::getWeight(API::FunctionValues_sptr values, size_t i,
                              double sqrtW) const {
  return (values->getFitWeight(i) / sqrtW);
}

//----------------------------------------------------------------------------------------------
/** Get square root of normalization weight (W)
  */
double CostFuncRwp::calSqrtW(API::FunctionValues_sptr values) const {
  double weight = 0.0;

  // FIXME : This might give a wrong answer in case of multiple-domain
  size_t ny = values->size();
  for (size_t i = 0; i < ny; ++i) {
    double obsval = values->getFitData(i);
    double inv_sigma = values->getFitWeight(i);
    weight += obsval * obsval * inv_sigma * inv_sigma;
  }

  return sqrt(weight);
}

} // namespace CurveFitting
} // namespace Mantid
