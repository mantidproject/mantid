// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidCurveFitting/DllConfig.h"
#include "MantidKernel/Matrix.h"

#include <boost/function.hpp>

namespace Mantid {
namespace CurveFitting {
/**
 * The results of the optimization
 */
enum OptimizerResult { Failure = -1, Success = 1, FTolReached = 2, XTolReached = 3 };

//---------------------------------------------------------------------------------------------
// Forward declaration
//---------------------------------------------------------------------------------------------
class UnconstrainedCostFunction;

/**
Implements the Augmented Lagrangian optimization method of Birgin & Martinez.
See

E. G. Birgin and J. M. Martiinez, "Improving ultimate convergence of an
augmented Lagrangian method,",
  Optimization Methods and Software vol. 23, no. 2, p. 177-195 (2008).

If the objective function is written as
\f[
Cx = d
\f]
where \f$x\f$ are the parameters, then the routine attempts to minimize
\f[
\frac{1}{2}||Cx - d||^2
\f]
where \f$ ||f|| \f$ denotes the 2nd-norm of \f$f\f$. It possible to specify an
optional set of constraints
such that the function is minimized subject to
\f[
Ax \geq 0
\f]
and
\f[
A_{eq} x = 0
\f]
 */
class MANTID_CURVEFITTING_DLL AugmentedLagrangianOptimizer {

public:
  /// Function type
  using ObjFunction = boost::function<double(const size_t, const double *)>;

public:
  /**
   * Constructor
   * @param nparams The number of parameters in the problem
   * @param objfunc An object whose type has a member named eval with the
   * following signature
   * "double eval(const std::vector<double> &) const" to return the value of the
   * objective function
   * at a given pt
   */
  AugmentedLagrangianOptimizer(const size_t nparams, const ObjFunction &objfunc)
      : m_userfunc(objfunc), m_nparams(nparams), m_neq(0), m_eq(), m_nineq(0), m_ineq(), m_maxIter(500) {}

  /**
   * Constructor with constraints
   * @param nparams The number of parameters in the problem
   * @param objfunc An object whose type has a member named eval with the
   * following signature
   * "double eval(const std::vector<double> &)" to return the value of the
   * objective function
   * at a given pt
   * @param equality A matrix of coefficients, \f$A_{eq}\f$ such that in the
   * final solution \f$A_{eq} x = 0\f$
   * @param inequality A matrix of coefficients, \f$A\f$ such that in the final
   * solution \f$A_{eq} x\geq 0\f$
   */
  AugmentedLagrangianOptimizer(const size_t nparams, const ObjFunction &objfunc, const Kernel::DblMatrix &equality,
                               const Kernel::DblMatrix &inequality)
      : m_userfunc(objfunc), m_nparams(nparams), m_neq(equality.numRows()), m_eq(equality),
        m_nineq(inequality.numRows()), m_ineq(inequality), m_maxIter(500) {
    checkConstraints(equality, inequality);
  }

  /// Disable default constructor
  AugmentedLagrangianOptimizer() = delete;

  /// Disable copy operator
  AugmentedLagrangianOptimizer(const AugmentedLagrangianOptimizer &) = delete;

  /// Disable assignment operator
  AugmentedLagrangianOptimizer &operator=(const AugmentedLagrangianOptimizer &) = delete;

  /// @returns The number of parameters under minimization
  inline size_t numParameters() const { return m_nparams; }
  /// @returns The number of equality constraints
  inline size_t numEqualityConstraints() const { return m_neq; }
  /// @returns The number of inequality constraints
  inline size_t numInequalityConstraints() const { return m_nineq; }

  /**
   * Override the maximum number of iterations (Default = 500)
   * @param maxIter Maximum value for the main minimizer loop
   */
  inline void setMaxIterations(const int maxIter) { m_maxIter = maxIter; }

  /// Perform the minimization
  void minimize(std::vector<double> &xv) const;

private:
  friend class UnconstrainedCostFunction;
  /// Using gradient optimizer to perform limited optimization of current set
  void unconstrainedOptimization(const std::vector<double> &lambda, const std::vector<double> &mu, const double rho,
                                 std::vector<double> &xcur) const;

  /// Sanity check for constraint inputs
  void checkConstraints(const Kernel::DblMatrix &equality, const Kernel::DblMatrix &inequality);

  /// User-defined function
  // FunctionWrapper m_userfunc;

  ObjFunction m_userfunc;
  /// Number of parameters under minimization
  const size_t m_nparams;
  /// Number of equality constraints
  const size_t m_neq;
  /// Defines the equality constraints
  Kernel::DblMatrix m_eq;
  /// Number of inequality constraints
  const size_t m_nineq;
  /// Defines the inequality constraints
  Kernel::DblMatrix m_ineq;
  /// Maximum number of iterations
  int m_maxIter;
};

} // namespace CurveFitting
} // namespace Mantid
