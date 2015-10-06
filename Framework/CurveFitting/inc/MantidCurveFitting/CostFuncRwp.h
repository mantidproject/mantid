#ifndef MANTID_CURVEFITTING_COSTFUNCRWP_H_
#define MANTID_CURVEFITTING_COSTFUNCRWP_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/CostFuncLeastSquares.h"
#include "MantidCurveFitting/GSLMatrix.h"
#include "MantidCurveFitting/GSLVector.h"

namespace Mantid {
namespace Kernel {
class Logger;
}
namespace CurveFitting {
class SeqDomain;
class ParDomain;

/** Cost function for Rwp = (sum_i (( obs_i - cal_i )/sigma_i)**2 ) / (sum_i
   (obs_i/sigma_i)**2)

    @author
    @date

    Copyright &copy;

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport CostFuncRwp : public CostFuncLeastSquares {
public:
  /// Constructor
  CostFuncRwp();
  /// Virtual destructor
  virtual ~CostFuncRwp() {}

  /// Get name of minimizer
  virtual std::string name() const { return "Rwp"; }

  /// Get short name of minimizer - useful for say labels in guis
  virtual std::string shortName() const { return "Rwp"; }

private:
  virtual std::vector<double>
  getFitWeights(API::FunctionValues_sptr values) const;

  /// Get weight (1/sigma)
  double getWeight(API::FunctionValues_sptr values, size_t i,
                   double sqrtW = 1.0) const;

  /// Calcualte sqrt(W). Final cost function = sum_i [ (obs_i - cal_i) / (sigma
  /// * sqrt(W))]**2
  double calSqrtW(API::FunctionValues_sptr values) const;
};

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_COSTFUNCRWP_H_*/
