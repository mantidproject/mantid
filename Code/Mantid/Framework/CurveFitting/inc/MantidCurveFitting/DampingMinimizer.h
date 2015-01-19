#ifndef MANTID_CURVEFITTING_DAMPINGMINIMIZER_H_
#define MANTID_CURVEFITTING_DAMPINGMINIMIZER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidCurveFitting/GSLVector.h"
#include "MantidCurveFitting/GSLMatrix.h"

namespace Mantid {
namespace CurveFitting {

class CostFuncLeastSquares;

/**
    Implements a least squares minimization algorithm with damping.

    @author Roman Tolchenov, Tessella plc

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
class DLLExport DampingMinimizer : public API::IFuncMinimizer {
public:
  /// Constructor
  DampingMinimizer(double relTol = 0.0001);
  /// Name of the minimizer.
  std::string name() const { return "DampingMinimizer"; }

  /// Initialize minimizer, i.e. pass a function to minimize.
  virtual void initialize(API::ICostFunction_sptr function,
                          size_t maxIterations = 0);
  /// Do one iteration.
  virtual bool iterate(size_t);
  /// Return current value of the cost function
  virtual double costFunctionVal();

private:
  /// Pointer to the cost function. Must be the least squares.
  boost::shared_ptr<CostFuncLeastSquares> m_leastSquares;
  /// Relative tolerance.
  double m_relTol;
  /// The damping mu parameter in the Levenberg-Marquardt method.
  // double m_damping;
};

} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_DAMPINGMINIMIZER_H_*/
