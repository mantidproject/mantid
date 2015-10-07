#ifndef MANTID_CURVEFITTING_COSTFUNCUNWEIGHTEDLEASTSQUARES_H_
#define MANTID_CURVEFITTING_COSTFUNCUNWEIGHTEDLEASTSQUARES_H_

#include "MantidKernel/System.h"
#include "MantidCurveFitting/CostFuncLeastSquares.h"

namespace Mantid {
namespace CurveFitting {

/** @class CostFuncUnweightedLeastSquares
 *
  In contrast to CostFuncLeastSquares, this variant of the cost function
  assumes that there are no weights attached to the values, so all observations
  will have unit weights.

  The covariance matrix is multiplied with the variance of the residuals.

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 03/03/2015

  Copyright © 2015 PSI-NXMM

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport CostFuncUnweightedLeastSquares : public CostFuncLeastSquares {
public:
  CostFuncUnweightedLeastSquares();
  virtual ~CostFuncUnweightedLeastSquares() {}

  std::string name() const { return "Unweighted least squares"; }
  std::string shortName() const { return "Chi-sq-unw."; }

protected:
  void calActiveCovarianceMatrix(GSLMatrix &covar, double epsrel);
  std::vector<double> getFitWeights(API::FunctionValues_sptr values) const;

  double getResidualVariance() const;
};

} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_COSTFUNCUNWEIGHTEDLEASTSQUARES_H_ */
