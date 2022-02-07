// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidCurveFitting/DllConfig.h"

#include <gsl/gsl_multifit_nlin.h>
#include <gsl/gsl_multimin.h>

namespace Mantid {
namespace CurveFitting {
namespace FuncMinimisers {
/** A wrapper around the GSL functions implementing a minimizer using
   derivatives.
    Concrete classes must implement the getGSLMinimizerType() method.

    @author Roman Tolchenov, Tessella plc
*/
class MANTID_CURVEFITTING_DLL DerivMinimizer : public API::IFuncMinimizer {
public:
  /// Constructor
  DerivMinimizer();
  DerivMinimizer(const double stepSize, const double tolerance);
  /// Destructor
  ~DerivMinimizer() override;

  /// Do one iteration.
  bool iterate(size_t) override;
  /// Return current value of the cost function
  double costFunctionVal() override;
  /// Calculate the covariance matrix.
  void calCovarianceMatrix(gsl_matrix *covar, double epsrel = 0.0001);
  /// Initialize minimizer, i.e. pass a function to minimize.
  void initialize(API::ICostFunction_sptr function, size_t maxIterations = 0) override;
  /// Set maximum value of the gradient at which iterations can stop
  void setStopGradient(const double value);

protected:
  /// Return a concrete type to initialize m_gslSolver with
  virtual const gsl_multimin_fdfminimizer_type *getGSLMinimizerType() = 0;

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
  static double fun(const gsl_vector *x, void *params);
  /// Used by the GSL
  static void dfun(const gsl_vector *x, void *params, gsl_vector *g);
  /// Used by the GSL
  static void fundfun(const gsl_vector *x, void *params, double *f, gsl_vector *g);

private:
  /// simply init the values for the gsl minimizer
  void initGSLMMin();
};

} // namespace FuncMinimisers
} // namespace CurveFitting
} // namespace Mantid
