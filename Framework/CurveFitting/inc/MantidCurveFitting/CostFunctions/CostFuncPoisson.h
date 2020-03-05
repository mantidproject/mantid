#ifndef MANTID_CURVEFITTING_COSTFUNCPOISSON_H_
#define MANTID_CURVEFITTING_COSTFUNCPOISSON_H_

#include "MantidAPI/FunctionDomain.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidCurveFitting/CostFunctions/CostFuncFitting.h"

namespace Mantid {
namespace CurveFitting {
namespace CostFunctions {

/** CostFuncPoisson : Implements a cost function for fitting applications using
  a Poisson measure

  Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_CURVEFITTING_DLL CostFuncPoisson : public CostFuncFitting {
public:
  CostFuncPoisson();
  /// Get name of minimizer
  virtual std::string name() const override { return "Poisson"; }
  /// Get short name of minimizer - useful for say labels in guis
  virtual std::string shortName() const override { return "Poisson"; };

  void addVal(API::FunctionDomain_sptr domain,
              API::FunctionValues_sptr values) const override;
  void addValDerivHessian(API::IFunction_sptr function,
                          API::FunctionDomain_sptr domain,
                          API::FunctionValues_sptr values,
                          bool evalDeriv = true,
                          bool evalHessian = true) const override;

private:
  /// Calculates the derivative for the addValDerivHessian method
  void calculateDerivative(API::IFunction &function,
                           API::FunctionDomain &domain,
                           API::FunctionValues &values) const;
  /// Calculates the Hessian matrix for the addValDerivHessian method
  void calculateHessian(API::IFunction &function, API::FunctionDomain &domain,
                        API::FunctionValues &values) const;
};

} // namespace CostFunctions
} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_COSTFUNCPOISSON_H_ */