#ifndef MANTID_TEIXEIRAWATERSQE_H_
#define MANTID_TEIXEIRAWATERSQE_H_

// Mantid Coding standars <http://www.mantidproject.org/Coding_Standards>
// Mantid Headers from the same project
// Mantid headers from other projects
#include "MantidCurveFitting/Functions/FunctionQDepends.h"
// 3rd party library headers (N/A)
// standard library headers (N/A)

namespace API {
//----------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------
class Jacobian;
} // namespace API

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
@author Jose Borreguero, NScD
@date 25/09/2016

Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

/**
 * @brief Teixeira's model to describe the translational diffusion of water
 */
class DLLExport TeixeiraWaterSQE : public FunctionQDepends {

public:
  std::string name() const override { return "TeixeiraWaterSQE"; }
  const std::string category() const override { return "QuasiElastic"; }
  void function1D(double *out, const double *xValues,
                  const size_t nData) const override;
  void functionDeriv1D(Mantid::API::Jacobian *jacobian, const double *xValues,
                       const size_t nData) override;

protected:
  void declareParameters() override;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif // MANTID_TEIXEIRAWATERSQE_H_
