#ifndef MANTID_CURVEFITTING_POLYNOMIAL_H_
#define MANTID_CURVEFITTING_POLYNOMIAL_H_

#include "MantidCurveFitting/Functions/BackgroundFunction.h"
#include "MantidKernel/System.h"
#include <cmath>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/** Polynomial : N-th polynomial background function.

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
class DLLExport Polynomial : public BackgroundFunction {
public:
  Polynomial();

  /// Overwrite IFunction base class
  std::string name() const override { return "Polynomial"; }

  const std::string category() const override { return "Background"; }

  void function1D(double *out, const double *xValues,
                  const size_t nData) const override;

  void functionDeriv1D(API::Jacobian *out, const double *xValues,
                       const size_t nData) override;

  /// Returns the number of attributes associated with the function (polynomial
  /// order n)
  size_t nAttributes() const override { return 1; }

  /// Returns a list of attribute names
  std::vector<std::string> getAttributeNames() const override;

  /// Return a value of attribute attName
  Attribute getAttribute(const std::string &attName) const override;

  /// Set a value to attribute attName
  void setAttribute(const std::string &attName, const Attribute &) override;

  /// Check if attribute attName exists
  bool hasAttribute(const std::string &attName) const override;

private:
  /// Polynomial order
  int m_n;
};

using Polynomial_sptr = boost::shared_ptr<Polynomial>;

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_POLYNOMIAL_H_ */
