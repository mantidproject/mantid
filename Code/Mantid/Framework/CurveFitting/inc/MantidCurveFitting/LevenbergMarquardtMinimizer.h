#ifndef MANTID_CURVEFITTING_LEVENBERGMARQUARDTMINIMIZER_H_
#define MANTID_CURVEFITTING_LEVENBERGMARQUARDTMINIMIZER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/IFuncMinimizer.h"
#include <gsl/gsl_multifit_nlin.h>
#include "MantidAPI/IFitFunction.h"
//#include "MantidCurveFitting/Fit.h"
#include "MantidCurveFitting/GSLFunctions.h"

namespace Mantid
{
namespace CurveFitting
{
/** Implementing Levenberg-Marquardt by wrapping the IFuncMinimizer interface
    around the GSL implementation of this algorithm.

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
class DLLExport LevenbergMarquardtMinimizer : public IFuncMinimizer
{
public:
  /// constructor and destructor
  ~LevenbergMarquardtMinimizer();
  LevenbergMarquardtMinimizer(): m_name("Levenberg-Marquardt") {}

  /// Overloading base class methods
  std::string name()const;
  int iterate();
  int hasConverged();
  double costFunctionVal();
  void calCovarianceMatrix(double epsrel, gsl_matrix * covar);
  void initialize(double* X, const double* Y, double *sqrtWeight, const int& nData, const int& nParam, 
    gsl_vector* startGuess, API::IFitFunction* fit, const std::string& costFunction);
  void initialize(API::IFitFunction* fit, const std::string& costFunction);

private:
  /// name of this minimizer
  const std::string m_name;

  /// GSL data container
  GSL_FitData *m_data;

  /// GSL minimizer container
  gsl_multifit_function_fdf gslContainer;

  /// pointer to the GSL solver doing the work
  gsl_multifit_fdfsolver *m_gslSolver;

  /// Stored to access IFunction interface in iterate()
  API::IFitFunction* m_function;

	/// Static reference to the logger class
	static Kernel::Logger& g_log;
};


} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_LEVENBERGMARQUARDTMINIMIZER_H_*/
