#ifndef MANTID_CURVEFITTING_COSTFUNCFITTING_H_
#define MANTID_CURVEFITTING_COSTFUNCFITTING_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/ICostFunction.h"
#include "MantidAPI/IFunction.h"
//#include "MantidKernel/Matrix.h"
#include "MantidCurveFitting/GSLVector.h"
#include "MantidCurveFitting/GSLMatrix.h"

namespace Mantid {
namespace CurveFitting {
/** A semi-abstract class for a cost function for fitting functions.
    Implement val(), deriv(), and valAndDeriv() methods in a concrete class.

    @author Roman Tolchenov, Tessella plc
    @date 10/04/2012

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

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport CostFuncFitting : public API::ICostFunction {
public:
  CostFuncFitting();
  /// Get i-th parameter
  /// @param i :: Index of a parameter
  /// @return :: Value of the parameter
  virtual double getParameter(size_t i) const;
  /// Set i-th parameter
  /// @param i :: Index of a parameter
  /// @param value :: New value of the parameter
  virtual void setParameter(size_t i, const double &value);
  /// Number of parameters
  virtual size_t nParams() const;

  /// Set fitting function.
  virtual void setFittingFunction(API::IFunction_sptr function,
                                  API::FunctionDomain_sptr domain,
                                  API::FunctionValues_sptr values);

  /// Get fitting function.
  virtual API::IFunction_sptr getFittingFunction() const { return m_function; }

  /// Calculates covariance matrix
  /// @param covar :: Returned covariance matrix, here as
  /// @param epsrel :: Is used to remove linear-dependent columns
  ///
  virtual void calCovarianceMatrix(GSLMatrix &covar, double epsrel = 1e-8);

  /// Calculate fitting errors
  virtual void calFittingErrors(const GSLMatrix &covar, double chi2);
  /// Get the domain the fitting function is applied to
  API::FunctionDomain_sptr getDomain() const { return m_domain; }
  /// Get FunctionValues where function values are stored.
  API::FunctionValues_sptr getValues() const { return m_values; }

protected:
  /**
   * Calculates covariance matrix for fitting function's active parameters.
   */
  virtual void calActiveCovarianceMatrix(GSLMatrix &covar,
                                         double epsrel = 1e-8);

  bool isValid() const;
  void checkValidity() const;
  void calTransformationMatrixNumerically(GSLMatrix &tm);
  void setDirty();

  /// Shared pointer to the fitting function
  API::IFunction_sptr m_function;
  /// Shared pointer to the function domain
  API::FunctionDomain_sptr m_domain;
  /// Shared poinetr to the function values
  API::FunctionValues_sptr m_values;
  /// maps the cost function's parameters to the ones of the fitting function.
  std::vector<size_t> m_indexMap;

  mutable bool m_dirtyVal;     /// dirty value flag
  mutable bool m_dirtyDeriv;   /// dirty derivatives flag
  mutable bool m_dirtyHessian; /// dirty hessian flag
};

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_COSTFUNCFITTING_H_*/
