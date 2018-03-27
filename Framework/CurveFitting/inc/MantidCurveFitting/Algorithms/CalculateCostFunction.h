#ifndef MANTID_CURVEFITTING_CALCULATECOSTFUNCTION_H_
#define MANTID_CURVEFITTING_CALCULATECOSTFUNCTION_H_

#include "MantidKernel/System.h"
#include "MantidCurveFitting/IFittingAlgorithm.h"

namespace Mantid {
namespace CurveFitting {

namespace CostFunctions {
class CostFuncFitting;
}

namespace Algorithms {

/**

  Calculate cost function for a function and a data set in a workspace.

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
class DLLExport CalculateCostFunction : public IFittingAlgorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override {
    return {"CalculateChiSquared", "Fit"};
  }
  const std::string summary() const override;

private:
  void initConcrete() override;
  void execConcrete() override;

  /// Cache for the cost function
  boost::shared_ptr<CostFunctions::CostFuncFitting> m_costFunction;
};

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_CALCULATECOSTFUNCTION_H_ */
