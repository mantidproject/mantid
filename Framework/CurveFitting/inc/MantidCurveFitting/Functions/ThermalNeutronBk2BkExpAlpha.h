#ifndef MANTID_CURVEFITTING_THERMALNEUTRONBK2BKEXPALPHA_H_
#define MANTID_CURVEFITTING_THERMALNEUTRONBK2BKEXPALPHA_H_

#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidKernel/System.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/** ThermalNeutronBk2BkExpAlpha : Function to calculate Alpha of Bk2Bk
  Exponential function from
  Thermal Neutron Function's Alph0, Alph1, Alph0t, Alph1t, Dtt1, and etc.

  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

  File change history is stored at:
  <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport ThermalNeutronBk2BkExpAlpha : virtual public API::IFunction1D,
                                              public API::ParamFunction {
public:
  /// Override
  void function1D(double *out, const double *xValues,
                  const size_t nData) const override;

  /// overwrite IFunction base class methods
  std::string name() const override { return "ThermalNeutronBk2BkExpAlpha"; }

  /// Overwrite IFunction
  const std::string category() const override { return "General"; }

protected:
  /// overwrite IFunction base class method, which declare function parameters
  void init() override;

private:
  /// Derivative
  void functionDerivLocal(API::Jacobian *, const double *, const size_t);

  /// Derivative to overwrite
  void functionDeriv(const API::FunctionDomain &domain,
                     API::Jacobian &jacobian) override;

  /// Core function (inline) to calcualte TOF_h from d-spacing
  inline double corefunction(double dh, double width, double tcross,
                             double alph0, double alph1, double alph0t,
                             double alph1t) const;
};

using ThermalNeutronBk2BkExpAlpha_sptr =
    boost::shared_ptr<ThermalNeutronBk2BkExpAlpha>;

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_THERMALNEUTRONBK2BKEXPALPHA_H_ */
