// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_LEVENBERGMARQUARDTMINIMIZER_H_
#define MANTID_CURVEFITTING_LEVENBERGMARQUARDTMINIMIZER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidAPI/IFunction.h"
#include "MantidCurveFitting/GSLFunctions.h"
#include <gsl/gsl_multifit_nlin.h>

namespace Mantid {
namespace CurveFitting {
namespace FuncMinimisers {
/** Implementing Levenberg-Marquardt by wrapping the IFuncMinimizer interface
    around the GSL implementation of this algorithm.

    @author Anders Markvardsen, ISIS, RAL
    @date 11/12/2009
*/
class DLLExport LevenbergMarquardtMinimizer : public API::IFuncMinimizer {
public:
  /// constructor and destructor
  ~LevenbergMarquardtMinimizer() override;
  LevenbergMarquardtMinimizer();

  /// Overloading base class methods
  /// Name of the minimizer.
  std::string name() const override { return "Levenberg-Marquardt"; }

  /// Initialize minimizer, i.e. pass a function to minimize.
  void initialize(API::ICostFunction_sptr costFunction,
                  size_t maxIterations = 0) override;
  /// Do one iteration.
  bool iterate(size_t) override;
  /// Return current value of the cost function
  double costFunctionVal() override;

private:
  void calCovarianceMatrix(double epsrel, gsl_matrix *covar);
  int hasConverged();

  /// GSL data container
  std::unique_ptr<GSL_FitData> m_data;

  /// GSL minimizer container
  gsl_multifit_function_fdf gslContainer;

  /// pointer to the GSL solver doing the work
  gsl_multifit_fdfsolver *m_gslSolver;

  /// Stored to access IFunction interface in iterate()
  API::IFunction_sptr m_function;

  /// Absolute error required for parameters
  double m_absError;

  /// Relative error required for parameters
  double m_relError;
};

} // namespace FuncMinimisers
} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_LEVENBERGMARQUARDTMINIMIZER_H_*/
