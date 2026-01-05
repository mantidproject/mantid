// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/ICostFunction.h"
#include "MantidAPI/IFunction.h"
#include "MantidCurveFitting/CostFunctions/CostFuncLeastSquares.h"
#include "MantidCurveFitting/EigenJacobian.h"
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
  GSL_FitData(const std::shared_ptr<CostFunctions::CostFuncLeastSquares> &cf);
  /// Destructor
  ~GSL_FitData();
  /// number of points to be fitted (size of X, Y and sqrtWeightData arrays)
  size_t n;
  /// number of (active) fit parameters
  size_t p;
  /// Pointer to the function
  API::IFunction_sptr function;
  std::shared_ptr<CostFunctions::CostFuncLeastSquares> costFunction;
  /// Initial function parameters
  gsl_vector *initFuncParams;
  /// Jacobi matrix interface
  JacobianImpl1<EigenMatrix> J;

  // this is presently commented out in the implementation
  // gsl_matrix *holdCalculatedJacobian; ///< cache of the calculated jacobian
};

int gsl_f(const gsl_vector *x, void *params, gsl_vector *f);
int gsl_df(const gsl_vector *x, void *params, gsl_matrix *J);
int gsl_fdf(const gsl_vector *x, void *params, gsl_vector *f, gsl_matrix *J);

/// take const data from Eigen Vector and take a gsl view
inline gsl_vector_const_view const getGSLVectorView_const(const vec_map_type v) {
  return gsl_vector_const_view_array(v.data(), v.size());
}
/// take data from a constEigen Matrix and return a transposed gsl view.
inline gsl_matrix_const_view const getGSLMatrixView_const(const map_type m) {
  return gsl_matrix_const_view_array(m.data(), m.cols(), m.rows());
}
} // namespace CurveFitting
} // namespace Mantid
