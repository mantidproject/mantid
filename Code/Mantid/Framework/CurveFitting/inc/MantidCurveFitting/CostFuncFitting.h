#ifndef MANTID_CURVEFITTING_COSTFUNCLEASTSQUARES_H_
#define MANTID_CURVEFITTING_COSTFUNCLEASTSQUARES_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/ICostFunction.h"
#include "MantidAPI/IFunction.h"

#include <gsl/gsl_matrix.h>

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

  /// Calculates covariance matrix
  /// @param covar :: Returned covariance matrix, here as 
  /// @param epsrel :: Is used to remove linear-dependent columns
  ///
  virtual void calCovarianceMatrix(gsl_matrix * covar, double epsrel = 0.0001);

protected:

  bool isValid() const;
  
  API::IFunction_sptr m_function;
  API::FunctionDomain_sptr m_domain;
  API::FunctionValues_sptr m_values;
};

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_COSTFUNCLEASTSQUARES_H_*/
