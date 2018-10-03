// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
