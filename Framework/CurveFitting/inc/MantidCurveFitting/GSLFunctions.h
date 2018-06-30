#ifndef MANTID_CURVEFITTING_GSLFUNCTIONS_H_
#define MANTID_CURVEFITTING_GSLFUNCTIONS_H_

#include "MantidAPI/ICostFunction.h"
#include "MantidAPI/IFunction.h"
#include "MantidCurveFitting/CostFunctions/CostFuncLeastSquares.h"
#include "MantidCurveFitting/GSLJacobian.h"
#include <gsl/gsl_blas.h>
#include <gsl/gsl_multifit_nlin.h>
#include <gsl/gsl_multimin.h>
#include <gsl/gsl_statistics.h>

namespace Mantid {
namespace CurveFitting {
/**
Various GSL specific functions used GSL specific minimizers

@author Anders Markvardsen, ISIS, RAL
@date 14/05/2010

Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

/// Structure to contain least squares data and used by GSL
struct GSL_FitData {
  /// Constructor
  GSL_FitData(boost::shared_ptr<CostFunctions::CostFuncLeastSquares> cf);
  /// Destructor
  ~GSL_FitData();
  /// number of points to be fitted (size of X, Y and sqrtWeightData arrays)
  size_t n;
  /// number of (active) fit parameters
  size_t p;
  /// Pointer to the function
  API::IFunction_sptr function;
  boost::shared_ptr<CostFunctions::CostFuncLeastSquares> costFunction;
  /// Initial function parameters
  gsl_vector *initFuncParams;
  /// Jacobi matrix interface
  JacobianImpl1 J;

  // this is presently commented out in the implementation
  // gsl_matrix *holdCalculatedJacobian; ///< cache of the calculated jacobian
};

int gsl_f(const gsl_vector *x, void *params, gsl_vector *f);
int gsl_df(const gsl_vector *x, void *params, gsl_matrix *J);
int gsl_fdf(const gsl_vector *x, void *params, gsl_vector *f, gsl_matrix *J);

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_GSLFUNCTIONS_H_*/
