#ifndef MANTID_CURVEFITTING_DAMPEDGAUSSNEWTONMINIMIZER_H_
#define MANTID_CURVEFITTING_DAMPEDGAUSSNEWTONMINIMIZER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFuncMinimizer.h"
#include "MantidCurveFitting/GSLVector.h"
#include "MantidCurveFitting/GSLMatrix.h"

namespace Mantid {
namespace CurveFitting {
namespace CostFunctions {
class CostFuncLeastSquares;
} // namespace CostFunctions
namespace FuncMinimisers {

/**
    Implements a Gauss-Newton minimization algorithm with damping
    for use with least squares cost function.

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
class DLLExport DampedGaussNewtonMinimizer : public API::IFuncMinimizer {
public:
  /// Constructor
  DampedGaussNewtonMinimizer(double relTol = 0.0001);
  /// Name of the minimizer.
  std::string name() const override { return "DampedGaussNewtonMinimizer"; }

  /// Initialize minimizer, i.e. pass a function to minimize.
  void initialize(API::ICostFunction_sptr function,
                  size_t maxIterations = 0) override;
  /// Do one iteration.
  bool iterate(size_t) override;
  /// Return current value of the cost function
  double costFunctionVal() override;

private:
  /// Pointer to the cost function. Must be the least squares.
  boost::shared_ptr<CostFunctions::CostFuncLeastSquares> m_leastSquares;
  /// Relative tolerance.
  double m_relTol;
  /// The damping mu parameter in the Levenberg-Marquardt method.
  // double m_damping;
};
} // namespace FuncMinimisers
} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_DAMPEDGAUSSNEWTONMINIMIZER_H_*/
