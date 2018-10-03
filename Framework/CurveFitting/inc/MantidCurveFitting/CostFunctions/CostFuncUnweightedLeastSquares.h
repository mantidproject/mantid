// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_COSTFUNCUNWEIGHTEDLEASTSQUARES_H_
#define MANTID_CURVEFITTING_COSTFUNCUNWEIGHTEDLEASTSQUARES_H_

#include "MantidCurveFitting/CostFunctions/CostFuncLeastSquares.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace CurveFitting {
namespace CostFunctions {

/** @class CostFuncUnweightedLeastSquares
 *
  In contrast to CostFuncLeastSquares, this variant of the cost function
  assumes that there are no weights attached to the values, so all observations
  will have unit weights.

  The covariance matrix is multiplied with the variance of the residuals.

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 03/03/2015
*/
class DLLExport CostFuncUnweightedLeastSquares : public CostFuncLeastSquares {
public:
  CostFuncUnweightedLeastSquares();

  std::string name() const override { return "Unweighted least squares"; }
  std::string shortName() const override { return "Chi-sq-unw."; }

protected:
  void calActiveCovarianceMatrix(GSLMatrix &covar, double epsrel) override;
  std::vector<double>
  getFitWeights(API::FunctionValues_sptr values) const override;

  double getResidualVariance() const;
};

} // namespace CostFunctions
} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_COSTFUNCUNWEIGHTEDLEASTSQUARES_H_ */
