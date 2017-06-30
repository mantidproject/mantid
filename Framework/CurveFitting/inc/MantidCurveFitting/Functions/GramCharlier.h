#ifndef MANTID_CURVEFITTING_GRAMCHARLIER_H_
#define MANTID_CURVEFITTING_GRAMCHARLIER_H_

#include "MantidCurveFitting/DllConfig.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/**
  Implements a Gram-Charlier A series expansion.

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_CURVEFITTING_DLL GramCharlier : public API::ParamFunction,
                                             public API::IFunction1D {
protected:
  /// overwrite IFunction base class method, which declare function parameters
  void init() override;

public:
  std::string name() const override { return "GramCharlier"; }
  void function1D(double *out, const double *x, const size_t n) const override;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_GRAMCHARLIER_H_ */
