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

namespace Mantid
{
namespace CurveFitting
{
/** Cost function for least squares

    @author Anders Markvardsen, ISIS, RAL
    @date 11/05/2010

    Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport CostFuncFitting : public API::ICostFunction 
{
public:
  CostFuncFitting();
  /// Get i-th parameter
  /// @param i :: Index of a parameter
  /// @return :: Value of the parameter
  virtual double getParameter(size_t i)const;
  /// Set i-th parameter
  /// @param i :: Index of a parameter
  /// @param value :: New value of the parameter
  virtual void setParameter(size_t i, const double& value);
  /// Number of parameters
  virtual size_t nParams()const;

  /// Set fitting function.
  virtual void setFittingFunction(API::IFunction_sptr function, 
    API::FunctionDomain_sptr domain, API::FunctionValues_sptr values);

  /// Get fitting function.
  virtual API::IFunction_sptr getFittingFunction(){return m_function;}

  /// Calculates covariance matrix
  /// @param covar :: Returned covariance matrix, here as 
  /// @param epsrel :: Is used to remove linear-dependent columns
  ///
  virtual void calCovarianceMatrix(GSLMatrix& covar, double epsrel = 1e-8);

  /// Calculate fitting errors
  virtual void calFittingErrors(const GSLMatrix& covar);
  API::FunctionDomain_sptr getDomain() const {return m_domain;}
  API::FunctionValues_sptr getValues() const {return m_values;}

protected:

  /**
   * Calculates covariance matrix for fitting function's active parameters. 
   */
  virtual void calActiveCovarianceMatrix(GSLMatrix& covar, double epsrel = 1e-8);

  bool isValid() const;
  void checkValidity() const;
  void calTransformationMatrixNumerically(GSLMatrix& tm);
  void setDirty();
  
  API::IFunction_sptr m_function;
  API::FunctionDomain_sptr m_domain;
  API::FunctionValues_sptr m_values;
  std::vector<size_t> m_indexMap;

  mutable bool m_dirtyVal;
  mutable bool m_dirtyDeriv;
  mutable bool m_dirtyHessian;
};

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_COSTFUNCFITTING_H_*/
