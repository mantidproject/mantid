#ifndef MANTID_CURVEFITTING_LEVENBERGMARQUARDTMINIMIZER_H_
#define MANTID_CURVEFITTING_LEVENBERGMARQUARDTMINIMIZER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFuncMinimizer.h"
#include <gsl/gsl_multifit_nlin.h>
#include "MantidAPI/IFunction.h"
#include "MantidCurveFitting/GSLFunctions.h"

namespace Mantid
{
namespace CurveFitting
{
/** Implementing Levenberg-Marquardt by wrapping the IFuncMinimizer interface
    around the GSL implementation of this algorithm.

    @author Anders Markvardsen, ISIS, RAL
    @date 11/12/2009

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
class DLLExport LevenbergMarquardtMinimizer : public API::IFuncMinimizer
{
public:
  /// constructor and destructor
  ~LevenbergMarquardtMinimizer();
  LevenbergMarquardtMinimizer();

  /// Overloading base class methods
  /// Name of the minimizer.
  std::string name() const {return "Levenberg-Marquardt";}

  /// Initialize minimizer, i.e. pass a function to minimize.
  virtual void initialize(API::ICostFunction_sptr function,size_t maxIterations = 0);
  /// Do one iteration.
  virtual bool iterate(size_t);
  /// Return current value of the cost function
  virtual double costFunctionVal();


private:
  void calCovarianceMatrix(double epsrel, gsl_matrix * covar);
  int hasConverged();

  /// GSL data container
  GSL_FitData *m_data;

  /// GSL minimizer container
  gsl_multifit_function_fdf gslContainer;

  /// pointer to the GSL solver doing the work
  gsl_multifit_fdfsolver *m_gslSolver;

  /// Stored to access IFunction interface in iterate()
  API::IFunction_sptr m_function;

  /// Absolute error required for parameters
  double m_absError;

  /// Relative error required for parameters
  double m_relError;
};


} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_LEVENBERGMARQUARDTMINIMIZER_H_*/
