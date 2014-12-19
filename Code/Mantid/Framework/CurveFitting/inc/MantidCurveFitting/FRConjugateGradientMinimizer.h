#ifndef MANTID_CURVEFITTING_FRCONJUGATEGRADIENTMINIMIZER_H_
#define MANTID_CURVEFITTING_FRCONJUGATEGRADIENTMINIMIZER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/DllConfig.h"
#include "MantidCurveFitting/DerivMinimizer.h"

namespace Mantid {
namespace CurveFitting {
/** Implementing Fletcher-Reeves flavour of the conjugate gradient algorithm
    by wrapping the IFuncMinimizer interface around the GSL implementation of
   this algorithm.

    @author Anders Markvardsen, ISIS, RAL
    @date 12/1/2010

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
class DLLExport FRConjugateGradientMinimizer : public DerivMinimizer {
public:
  /// Constructor.
  FRConjugateGradientMinimizer() : DerivMinimizer() {}
  /// Name of the minimizer.
  std::string name() const {
    return "Conjugate gradient (Fletcher-Reeves imp.)";
  }

protected:
  /// Return a concrete type to initialize m_gslSolver with
  virtual const gsl_multimin_fdfminimizer_type *getGSLMinimizerType();
};

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_FRCONJUGATEGRADIENTMINIMIZER_H_*/
