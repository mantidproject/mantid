#ifndef MANTID_CURVEFITTING_SIMPLEXMINIMIZER_H_
#define MANTID_CURVEFITTING_SIMPLEXMINIMIZER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/IFuncMinimizer.h"
#include <gsl/gsl_multimin.h>

namespace Mantid
{
namespace CurveFitting
{
/** Implementing Simplex by wrapping the IFuncMinimizer interface
    around the GSL implementation of this algorithm.

    @author Anders Markvardsen, ISIS, RAL
    @date 8/1/2010

    Copyright &copy; 2009 STFC Rutherford Appleton Laboratory

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
class DLLExport SimplexMinimizer : public IFuncMinimizer
{
public:
  /// constructor and destructor
  ~SimplexMinimizer();
  SimplexMinimizer(gsl_multimin_function& gslContainer, gsl_vector* startGuess, const double& size);

  /// Overloading base class methods
  std::string name()const;
  int iterate();
  int hasConverged();
  double costFunctionVal();
  void calCovarianceMatrix(double epsrel, gsl_matrix * covar);

private:
  /// name of this minimizer
  const std::string m_name;

  /// pointer to the GSL solver doing the work
  gsl_multimin_fminimizer *m_gslSolver;

  /// used by GSL
  gsl_vector *m_simplexStepSize;
};


} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_SIMPLEXMINIMIZER_H_*/
