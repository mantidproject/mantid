#ifndef MANTID_CURVEFITTING_IFUNCMINIMIZER_H_
#define MANTID_CURVEFITTING_IFUNCMINIMIZER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/System.h"
#include <string>
#include <gsl/gsl_multifit_nlin.h>
#include "MantidCurveFitting/FuncMinimizerFactory.h"

namespace Mantid
{
namespace API
{
  class IFitFunction;
}
namespace CurveFitting
{
/** An interface for function minimizers.

    @author Anders Markvardsen, ISIS, RAL
    @date 11/12/2009

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
class DLLExport IFuncMinimizer 
{
public:
  /// Virtual destructor
  virtual ~IFuncMinimizer() {}

  /// Initialize minimizer, i.e. pass costFunction, data etc
  virtual void initialize(double* X, const double* Y, double *sqrtWeight, const int& nData, const int& nParam, 
    gsl_vector* startGuess, API::IFitFunction* function, const std::string& costFunction) = 0;

  /// Get name of minimizer
  virtual std::string name() const = 0;

  /// Perform iteration with minimizer and return info about how well this went
  /// using the GSL status integer system. See gsl_errno.h for details.
  virtual int iterate() = 0;

  /// Has fit converged. Status is returned using the GSL status flags listed 
  /// in the header file gsl_errno.h. E.g. GSL_SUCCESS  = 0, GSL_FAILURE  = -1,
  /// GSL_CONTINUE = -2 where the latter means the minimizer has not quite 
  /// converged yet and need another iteration
  virtual int hasConverged() = 0;

  /// Get value of cost function 
  virtual double costFunctionVal() = 0;

  /** Calculates covariance matrix
   *
   * @param epsrel Is used to remove linear-dependent columns
   * @param covar Returned covariance matrix, here as 
   */
  virtual void calCovarianceMatrix(double epsrel, gsl_matrix * covar) = 0;
};

} // namespace CurveFitting
} // namespace Mantid

/**
 * Macro for declaring a new type of minimizers to be used with the FuncMinimizerFactory
 */
#define DECLARE_FUNCMINIMIZER(classname,username) \
        namespace { \
	Mantid::Kernel::RegistrationHelper register_funcminimizer_##classname( \
  ((Mantid::CurveFitting::FuncMinimizerFactory::Instance().subscribe<classname>(#username)) \
	, 0)); \
	} 

#endif /*MANTID_CURVEFITTING_IFUNCMINIMIZER_H_*/
