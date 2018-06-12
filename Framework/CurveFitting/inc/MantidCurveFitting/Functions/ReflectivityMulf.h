#ifndef MANTID_CURVEFITTING_REFLECTIVITYMULF_H_
#define MANTID_CURVEFITTING_REFLECTIVITYMULF_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Logger.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/IFunction1D.h"
#include <complex>
#include <cmath>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/** ReflectivityMulf : Calculate the ReflectivityMulf from a simple layer model.

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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
class DLLExport ReflectivityMulf : public API::IFunction1D,
                                   public API::ParamFunction {
public:
  ReflectivityMulf();

  void init() override;

  /// Overwrite IFunction base class
  std::string name() const override { return "ReflectivityMulf"; }

  const std::string category() const override { return "General"; }

  void function1D(double *out, const double *xValues,
                  const size_t nData) const override;

  /// Set a value to attribute attName
  void setAttribute(const std::string &attName, const Attribute &) override;

private:
  /// ReflectivityMulf layers
  int m_nlayer, m_nlayer_old;
};

using ReflectivityMulf_sptr = boost::shared_ptr<ReflectivityMulf>;

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_REFLECTIVITYMULF_H_ */
