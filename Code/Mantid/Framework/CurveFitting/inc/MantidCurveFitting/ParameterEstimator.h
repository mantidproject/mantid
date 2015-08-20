#ifndef MANTID_CURVEFITTING_PARAMETERESTIMATOR_H_
#define MANTID_CURVEFITTING_PARAMETERESTIMATOR_H_

#include "MantidKernel/System.h"

#include <vector>

namespace Mantid {

namespace API {
  class IFunction;
  class FunctionDomain1D;
  class FunctionValues;
}

namespace CurveFitting {

class SimpleChebfun;

/** ParameterEstimator estimates parameter values of some fitting functions
  from fitting data.

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
class DLLExport ParameterEstimator {
public:
  ParameterEstimator(API::IFunction& function,
                     const API::FunctionDomain1D& domain,
                     const API::FunctionValues& values);
  void estimate();

private:

  void extractValues(std::vector<double> &x, std::vector<double> &y) const;
  /// Set initial values to a function if it needs to.
  void setValues(API::IFunction &function, const SimpleChebfun &fun,
                 const SimpleChebfun &der1, const SimpleChebfun &der2) const;
  /// Test if initial values need to be set before fitting
  static bool needSettingInitialValues(const API::IFunction& function);

  /// Function for which to estimae parameters.
  API::IFunction& m_function;
  /// The domain and values objects that have the fitting data.
  const API::FunctionDomain1D& m_domain;
  const API::FunctionValues& m_values;
};

} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_PARAMETERESTIMATOR_H_ */
