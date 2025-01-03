// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/Matrix.h"

#include <vector>

namespace Mantid {
namespace Kernel {
namespace Math {
/**
Minimize an objective function using the SLSQP optimization subroutine
originally implemented by Dieter Kraft
and ported to Python by scipy. This implementation is a port of the scipy
variant using f2c to translate the Fortran
code.

If the objective function is written as
\f[
Cx = d
\f]
where \f$x\f$ are the parameters, then the routine attempts to minimize
\f[
\frac{1}{2}||Cx - d||^2
\f]
where \f$ ||f|| \f$ denotes the 2nd-norm of \f$f\f$. It possible to specify an
optional set of constraints such that the function
is minimized subject to
\f[
Ax \geq 0
\f]
and
\f[
A_{eq} x = 0
\f]
 */
class MANTID_KERNEL_DLL SLSQPMinimizer {
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
  template <typename T>
  SLSQPMinimizer(const size_t nparams, const T &objfunc)
      : m_nparams(nparams), m_neq(0), m_nineq(0), m_objfunc(objfunc), m_constraintNorms() {}

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
  template <typename T>
  SLSQPMinimizer(const size_t nparams, const T &objfunc, const DblMatrix &equality, const DblMatrix &inequality)
      : m_nparams(nparams), m_neq(equality.numRows()), m_nineq(inequality.numRows()), m_objfunc(objfunc),
        m_constraintNorms() {
    initializeConstraints(equality, inequality);
  }

  /// Disable default constructor
  SLSQPMinimizer() = delete;

  /// Disable copy operator
  SLSQPMinimizer(const SLSQPMinimizer &) = delete;

  /// Disable assignment operator
  SLSQPMinimizer &operator=(const SLSQPMinimizer &) = delete;

  /// @returns The number of parameters under minimization
  inline size_t numParameters() const { return m_nparams; }
  /// @returns The number of equality constraints
  inline size_t numEqualityConstraints() const { return m_neq; }
  /// @returns The number of inequality constraints
  inline size_t numInequalityConstraints() const { return m_nineq; }

  /// Perform the minimization
  std::vector<double> minimize(const std::vector<double> &x0) const;

private:
  /**
   * Compute the value of the objective function
   * @param x The current parameter pt
   * @returns The value at the given pt
   */
  inline double fvalue(const std::vector<double> &x) const { return m_objfunc.eval(x); }
  /// Compute derivative numerically
  void fprime(std::vector<double> &grad, const std::vector<double> &x) const;
  /// Compute values of constraints
  void evaluateConstraints(std::vector<double> &constrValues, const std::vector<double> &x) const;

  /// Create constraint array
  void initializeConstraints(const DblMatrix &equality, const DblMatrix &inequality);

  /// Non-templated wrapper for objective function object to allow it to be
  /// stored
  /// without templating the class
  class FunctionWrapper {
  private:
    class BaseHolder {
    public:
      virtual ~BaseHolder() = default;
      virtual double eval(const std::vector<double> &x) const = 0;
    };
    template <typename T> class TypeHolder : public BaseHolder {
    public:
      TypeHolder(const T &func) : func(func) {}
      double eval(const std::vector<double> &x) const override { return func.eval(x); }
      /// The actual function supplied by the user
      T func;
    };

  public:
    /// Construct
    template <typename T> FunctionWrapper(const T &func) : m_funcHolder(std::make_unique<TypeHolder<T>>(func)) {}
    /**
     * Calls user supplied function
     * @param x - The current pt
     * @returns The value of the function
     */
    double eval(const std::vector<double> &x) const { return m_funcHolder->eval(x); }
    /// Templated holder
    std::unique_ptr<BaseHolder> m_funcHolder;
  };

  /// Number of parameters under minimization
  const size_t m_nparams;
  /// Number of equality constraints
  const size_t m_neq;
  /// Number of inequality constraints
  const size_t m_nineq;

  /// User-defined function
  FunctionWrapper m_objfunc;
  /// Holder for constraint normals
  mutable std::vector<double> m_constraintNorms;
};

} // namespace Math
} // namespace Kernel
} // namespace Mantid
