#ifndef MANTID_KERNEL_SLSQPMINIMIZER_H_
#define MANTID_KERNEL_SLSQPMINIMIZER_H_

#include "MantidKernel/ClassMacros.h"
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

Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport SLSQPMinimizer {
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
      : m_nparams(nparams), m_neq(0), m_nineq(0),
        m_objfunc(FunctionWrapper(objfunc)), m_constraintNorms() {}

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
  SLSQPMinimizer(const size_t nparams, const T &objfunc,
                 const DblMatrix &equality, const DblMatrix &inequality)
      : m_nparams(nparams), m_neq(equality.numRows()),
        m_nineq(inequality.numRows()), m_objfunc(FunctionWrapper(objfunc)),
        m_constraintNorms() {
    initializeConstraints(equality, inequality);
  }

  /// @returns The number of parameters under minimization
  inline size_t numParameters() const { return m_nparams; }
  /// @returns The number of equality constraints
  inline size_t numEqualityConstraints() const { return m_neq; }
  /// @returns The number of inequality constraints
  inline size_t numInequalityConstraints() const { return m_nineq; }

  /// Perform the minimization
  std::vector<double> minimize(const std::vector<double> &x0) const;

private:
  DISABLE_DEFAULT_CONSTRUCT(SLSQPMinimizer)
  DISABLE_COPY_AND_ASSIGN(SLSQPMinimizer)

  /**
   * Compute the value of the objective function
   * @param x The current parameter pt
   * @returns The value at the given pt
   */
  inline double fvalue(const std::vector<double> &x) const {
    return m_objfunc.eval(x);
  }
  /// Compute derivative numerically
  void fprime(std::vector<double> &grad, const std::vector<double> &x) const;
  /// Compute values of constraints
  void evaluateConstraints(std::vector<double> &constrValues,
                           const std::vector<double> &x) const;

  /// Create constraint array
  void initializeConstraints(const DblMatrix &equality,
                             const DblMatrix &inequality);

  /// Non-templated wrapper for objective function object to allow it to be
  /// stored
  /// without templating the class
  class FunctionWrapper {
  private:
    class BaseHolder {
    public:
      virtual ~BaseHolder(){};
      virtual double eval(const std::vector<double> &x) const = 0;
    };
    template <typename T> class TypeHolder : public BaseHolder {
    public:
      TypeHolder(const T &func) : func(func) {}
      double eval(const std::vector<double> &x) const { return func.eval(x); }
      /// The actual function supplied by the user
      T func;
    };

  public:
    /// Construct
    template <typename T>
    FunctionWrapper(const T &func)
        : m_funcHolder(new TypeHolder<T>(func)) {}
    ~FunctionWrapper() { delete m_funcHolder; }
    /**
     * Calls user supplied function
     * @param x - The current pt
     * @returns The value of the function
     */
    double eval(const std::vector<double> &x) const {
      return m_funcHolder->eval(x);
    }
    /// Templated holder
    BaseHolder *m_funcHolder;
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
}
} // namespace Mantid::Kernel

#endif /* MANTID_KERNEL_SLSQPMINIMIZER_H_ */
