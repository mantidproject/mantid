#ifndef MANTID_CURVEFITTING_SIMPLEXMINIMIZER_H_
#define MANTID_CURVEFITTING_SIMPLEXMINIMIZER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFuncMinimizer.h"

#include <gsl/gsl_multimin.h>

namespace Mantid {
namespace CurveFitting {
/** Implementing Simplex by wrapping the IFuncMinimizer interface
    around the GSL implementation of this algorithm.

    @author Anders Markvardsen, ISIS, RAL
    @date 8/1/2010

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport SimplexMinimizer : public API::IFuncMinimizer {
public:
  /// Constructor setting a value for the relative error acceptance
  /// (default=0.01)
  SimplexMinimizer(const double epsabs = 1e-2);
  /// Destructor
  ~SimplexMinimizer();

  /// Overloading base class methods
  std::string name() const { return "Simplex"; }
  /// Do one iteration
  bool iterate(size_t);
  /// Return current value of the cost function
  double costFunctionVal();
  /// Initialize minimizer, i.e. pass a function to minimize.
  virtual void initialize(API::ICostFunction_sptr function,
                          size_t maxIterations = 0);

protected:
  void resetSize(const double &size);

private:
  /// clear memory
  void clearMemory();

  /// Used by the GSL to evaluate the function
  static double fun(const gsl_vector *x, void *params);

  /// Absolute value of the error that is considered a fit
  double m_epsabs;

  /// Function to minimize.
  API::ICostFunction_sptr m_costFunction;

  /// size of simplex
  double m_size;

  /// used by GSL
  gsl_vector *m_simplexStepSize;

  /// Starting parameter values
  gsl_vector *m_startGuess;

  /// pointer to the GSL solver doing the work
  gsl_multimin_fminimizer *m_gslSolver;

  /// GSL simplex minimizer container
  gsl_multimin_function gslContainer;
};

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_SIMPLEXMINIMIZER_H_*/
