#ifndef MANTID_CURVEFITTING_TRUSTREGIONMINIMIZER_H_
#define MANTID_CURVEFITTING_TRUSTREGIONMINIMIZER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidAPI/IFunction.h"
#include "MantidCurveFitting/CostFunctions/CostFuncLeastSquares.h"
#include "MantidCurveFitting/FortranDefs.h"
#include "MantidCurveFitting/GSLJacobian.h"
#include "MantidCurveFitting/RalNlls/Workspaces.h"

namespace Mantid {
namespace CurveFitting {
namespace FuncMinimisers {
/** A base class for least squares trust region minimizers.

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
    National Laboratory & European Spallation Source

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
class DLLExport TrustRegionMinimizer : public API::IFuncMinimizer {
public:
  /// constructor and destructor
  TrustRegionMinimizer();
  /// Initialize minimizer, i.e. pass a function to minimize.
  void initialize(API::ICostFunction_sptr costFunction,
                  size_t maxIterations = 0) override;
  /// Do one iteration.
  bool iterate(size_t) override;
  /// Return current value of the cost function
  double costFunctionVal() override;

private:
  /// Evaluate the fitting function and calculate the residuals.
  void evalF(const DoubleFortranVector &x, DoubleFortranVector &f) const;
  /// Evaluate the Jacobian
  void evalJ(const DoubleFortranVector &x, DoubleFortranMatrix &J) const;
  /// Evaluate the Hessian
  void evalHF(const DoubleFortranVector &x, const DoubleFortranVector &f,
              DoubleFortranMatrix &h) const;
  /// Find a correction vector to the parameters.
  virtual void
  calculateStep(const DoubleFortranMatrix &J, const DoubleFortranVector &f,
                const DoubleFortranMatrix &hf, const DoubleFortranVector &g,
                double Delta, DoubleFortranVector &d, double &normd,
                const NLLS::nlls_options &options, NLLS::nlls_inform &inform,
                NLLS::calculate_step_work &w) = 0;

  /// Stored cost function
  boost::shared_ptr<CostFunctions::CostFuncLeastSquares> m_leastSquares;
  /// Stored to access IFunction interface in iterate()
  API::IFunction_sptr m_function;
  /// Fitting data weights
  DoubleFortranVector m_weights;
  /// Fitting parameters
  DoubleFortranVector m_x;
  /// The Jacobian
  mutable JacobianImpl1 m_J;
  /// Options
  NLLS::nlls_options m_options;
  /// Information about the fitting
  NLLS::nlls_inform m_inform;
  /// Temporary and helper objects
  NLLS::NLLS_workspace m_workspace;
};

} // namespace FuncMinimisers
} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_TRUSTREGIONMINIMIZER_H_*/
