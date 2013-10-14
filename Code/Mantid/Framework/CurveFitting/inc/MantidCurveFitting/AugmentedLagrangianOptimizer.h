#ifndef MANTID_CURVEFITTING_AUGMENTEDLAGRANGIANOPTIMIZER_H_
#define MANTID_CURVEFITTING_AUGMENTEDLAGRANGIANOPTIMIZER_H_
/**
  Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
#include "MantidCurveFitting/DllConfig.h"

#include "MantidKernel/ClassMacros.h"
#include "MantidKernel/Matrix.h"

namespace Mantid
{
  namespace CurveFitting
  {
    /**
     * The results of the optimization
     */
    enum OptimizerResult
    {
       Failure = -1,
       Success = 1,
       FTolReached = 2,
       XTolReached = 3
    };

    /// Non-templated wrapper for objective function object to allow it to be stored
    /// without templating the class
    class FunctionWrapper
    {
    private:
      class BaseHolder
      {
      public:
        virtual ~BaseHolder() {};
        virtual double eval(const std::vector<double> & x) const = 0;
      };
      template<typename T>
      class TypeHolder : public BaseHolder
      {
      public:
        TypeHolder(const T & func) : func(func) {}
        double eval(const std::vector<double> & x) const { return func.eval(x); }
        /// The actual function supplied by the user
        T func;
      };

    public:
      /// Construct
      template<typename T>
      FunctionWrapper(const T & func) : m_funcHolder(new TypeHolder<T>(func)) {}
      ~FunctionWrapper()
      {
        delete m_funcHolder;
      }
      /**
       * Calls user supplied function
       * @param x - The current pt
       * @returns The value of the function
       */
      inline double eval(const std::vector<double> & x) const { return m_funcHolder->eval(x); }
      /// Templated holder
      BaseHolder *m_funcHolder;
    };

    //---------------------------------------------------------------------------------------------
    // Forward declaration
    //---------------------------------------------------------------------------------------------
    class UnconstrainedCostFunction;

    /**
    Implements the Augmented Lagrangian optimization method of Birgin & Martinez. See

    E. G. Birgin and J. M. Martiinez, "Improving ultimate convergence of an augmented Lagrangian method,",
      Optimization Methods and Software vol. 23, no. 2, p. 177-195 (2008).

    If the objective function is written as
    \f[
    Cx = d
    \f]
    where \f$x\f$ are the parameters, then the routine attempts to minimize
    \f[
    \frac{1}{2}||Cx - d||^2
    \f]
    where \f$ ||f|| \f$ denotes the 2nd-norm of \f$f\f$. It possible to specify an optional set of constraints
    such that the function is minimized subject to
    \f[
    Ax \geq 0
    \f]
    and
    \f[
    A_{eq} x = 0
    \f]


    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class MANTID_CURVEFITTING_DLL AugmentedLagrangianOptimizer
    {
    public:
      /**
       * Constructor
       * @param nparams The number of parameters in the problem
       * @param objfunc An object whose type has a member named eval with the following signature
       * "double eval(const std::vector<double> &) const" to return the value of the objective function
       * at a given pt
       */
      template <typename T>
      AugmentedLagrangianOptimizer(const size_t nparams, const T & objfunc)
        : m_userfunc(FunctionWrapper(objfunc)), m_nparams(nparams), m_neq(0),
          m_eq(), m_nineq(0), m_ineq(), m_maxIter(500)
      {
      }

      /**
       * Constructor with constraints
       * @param nparams The number of parameters in the problem
       * @param objfunc An object whose type has a member named eval with the following signature
       * "double eval(const std::vector<double> &)" to return the value of the objective function
       * at a given pt
       * @param equality A matrix of coefficients, \f$A_{eq}\f$ such that in the final solution \f$A_{eq} x = 0\f$
       * @param inequality A matrix of coefficients, \f$A\f$ such that in the final solution \f$A_{eq} x\geq 0\f$
       */
      template <typename T>
      AugmentedLagrangianOptimizer(const size_t nparams, const T & objfunc,
                                   const Kernel::DblMatrix & equality, const Kernel::DblMatrix & inequality)
        : m_userfunc(FunctionWrapper(objfunc)),
          m_nparams(nparams), m_neq(equality.numRows()), m_eq(equality), m_nineq(inequality.numRows()),
          m_ineq(inequality), m_maxIter(500)
      {
        checkConstraints(equality, inequality);
      }

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
      void minimize(std::vector<double> & xv) const;

    private:
      DISABLE_DEFAULT_CONSTRUCT(AugmentedLagrangianOptimizer);
      DISABLE_COPY_AND_ASSIGN(AugmentedLagrangianOptimizer);

      friend class UnconstrainedCostFunction;
      /// Using gradient optimizer to perform limited optimization of current set
      void unconstrainedOptimization(const std::vector<double> & lambda, const std::vector<double> & mu,
                                     const double rho, std::vector<double> & xcur) const;

      /// Sanity check for constraint inputs
      void checkConstraints(const Kernel::DblMatrix & equality, const Kernel::DblMatrix & inequality);

      /// User-defined function
      FunctionWrapper m_userfunc;
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

#endif  /* MANTID_CURVEFITTING_AUGMENTEDLAGRANGIANOPTIMIZER_H_ */
