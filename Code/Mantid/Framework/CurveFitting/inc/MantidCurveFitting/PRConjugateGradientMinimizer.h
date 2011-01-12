#ifndef MANTID_CURVEFITTING_PRCONJUGATEGRADIENTMINIMIZER_H_
#define MANTID_CURVEFITTING_PRCONJUGATEGRADIENTMINIMIZER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/IFuncMinimizer.h"
#include "MantidCurveFitting/GSLFunctions.h"
#include <gsl/gsl_multimin.h>
#include <gsl/gsl_multifit_nlin.h>

namespace Mantid
{
namespace CurveFitting
{
/** Implementing Polak-Ribiere flavour of the conjugate gradient algorithm
    by wrapping the IFuncMinimizer interface around the GSL implementation of this algorithm.

    @author Anders Markvardsen, ISIS, RAL
    @date 13/1/2010

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
class DLLExport PRConjugateGradientMinimizer : public IFuncMinimizer
{
public:
  /// constructor and destructor
  ~PRConjugateGradientMinimizer();
  PRConjugateGradientMinimizer(): m_name("Conjugate gradient (Polak-Ribiere imp.)") {}

  /// Overloading base class methods
  std::string name()const;
  int iterate();
  int hasConverged();
  double costFunctionVal();
  void calCovarianceMatrix(double epsrel, gsl_matrix * covar);
  void initialize(double* X, const double* Y, double *sqrtWeight, const int& nData, const int& nParam, 
    gsl_vector* startGuess, API::IFitFunction* function, const std::string& costFunction);
  void initialize(API::IFitFunction* function, const std::string& costFunction);

private:
  /// name of this minimizer
  const std::string m_name;

  /// pointer to the GSL solver doing the work
  gsl_multimin_fdfminimizer *m_gslSolver;

  /// passed information about the derivative etc of fitting function
  /// rather than the derivative etc of cost function
  /// used for calculating covariance matrix
  gsl_multifit_function_fdf m_gslLeastSquaresContainer;

  /// GSL data container
  GSL_FitData *m_data;

  /// GSL container
  gsl_multimin_function_fdf m_gslMultiminContainer;

	/// Static reference to the logger class
	static Kernel::Logger& g_log;
};


} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_PRCONJUGATEGRADIENTMINIMIZER_H_*/
