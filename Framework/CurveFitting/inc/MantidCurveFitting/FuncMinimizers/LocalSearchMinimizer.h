#ifndef MANTID_CURVEFITTING_LOCALSEARCHMINIMIZER_H_
#define MANTID_CURVEFITTING_LOCALSEARCHMINIMIZER_H_

#include "MantidAPI/IFuncMinimizer.h"
#include "MantidCurveFitting/DllConfig.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {
  class Chebfun;
}
namespace FuncMinimisers {

/** LocalSearchMinimizer : TODO: DESCRIPTION

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class MANTID_CURVEFITTING_DLL LocalSearchMinimizer
    : public API::IFuncMinimizer {
public:
  LocalSearchMinimizer();
  /// Name of the minimizer.
  std::string name() const { return "LocalSearchMinimizer"; }
  /// Do one iteration.
  bool iterate(size_t);
  /// Return current value of the cost function
  double costFunctionVal();
  /// Initialize minimizer, i.e. pass a function to minimize.
  virtual void initialize(API::ICostFunction_sptr function,
                          size_t maxIterations = 0);
private:
  /// Function to minimize.
  API::ICostFunction_sptr m_costFunction;
};

} // namespace FuncMinimisers
} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_LOCALSEARCHMINIMIZER_H_ */
