#ifndef MANTID_CURVEFITTING_DERIVMINIMIZERMINIMIZER_H_
#define MANTID_CURVEFITTING_DERIVMINIMIZERMINIMIZER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/DllConfig.h"
#include "MantidAPI/IFuncMinimizer.h"

#include <gsl/gsl_multimin.h>
#include <gsl/gsl_multifit_nlin.h>

namespace Mantid
{
namespace CurveFitting
{
/** A wrapper around the GSL functions implementing a minimizer using derivatives.
    Concrete classes must implement the getGSLMinimizerType() method.

    @author Roman Tolchenov, Tessella plc

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport DerivMinimizer : public API::IFuncMinimizer
{
public:
  /// Constructor
  DerivMinimizer();
  DerivMinimizer(const double stepSize, const double tolerance);
  /// Destructor
  ~DerivMinimizer();

  /// Do one iteration.
  bool iterate(size_t);
  /// Return current value of the cost function
  double costFunctionVal();
  /// Calculate the covariance matrix.
  void calCovarianceMatrix(gsl_matrix * covar, double epsrel = 0.0001);
  /// Initialize minimizer, i.e. pass a function to minimize.
  virtual void initialize(API::ICostFunction_sptr function,size_t maxIterations = 0);
  /// Set maximum value of the gradient at which iterations can stop
  void setStopGradient(const double value);

protected:

  /// Return a concrete type to initialize m_gslSolver with
  virtual const gsl_multimin_fdfminimizer_type* getGSLMinimizerType() = 0;

  /// Function to minimize.
  API::ICostFunction_sptr m_costFunction;

  /// pointer to the GSL solver doing the work
  gsl_multimin_fdfminimizer *m_gslSolver;

  /// GSL container
  gsl_multimin_function_fdf m_gslMultiminContainer;

  /// GSL vector with function parameters
  gsl_vector *m_x;

  /// the norm of the gradient at which iterations stop
  double m_stopGradient;
  /// First trial step size
  double m_stepSize;
  /// Tolerance
  double m_tolerance;
  /// Used by the GSL
  static double fun(const gsl_vector * x, void * params);
  /// Used by the GSL
  static void dfun(const gsl_vector * x, void * params, gsl_vector * g);
  /// Used by the GSL
  static void fundfun (const gsl_vector * x, void * params, double * f, gsl_vector * g);
};


} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_BFGS_MINIMIZERMINIMIZER_H_*/
