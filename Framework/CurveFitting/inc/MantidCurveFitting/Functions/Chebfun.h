#ifndef MANTID_CURVEFITTING_CHEBFUN_H_
#define MANTID_CURVEFITTING_CHEBFUN_H_

#include "MantidCurveFitting/DllConfig.h"
#include "MantidCurveFitting/Functions/SimpleChebfun.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/** Chebfun : TODO: DESCRIPTION

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
class MANTID_CURVEFITTING_DLL Chebfun {
public:
  Chebfun(ChebfunFunctionType fun, double start, double end,
          double accuracy = 0.0, size_t badSize = 10);
  /// Number of smooth parts
  size_t numberOfParts() const;
  /// Start of the interval
  double startX() const { return m_startX; }
  /// End of the interval
  double endX() const { return m_endX; }
  /// Evaluate the function.
  double operator()(double x) const;
  /// Evaluate the function.
  std::vector<double> operator()(const std::vector<double> &x) const;
  /// Total size of the approximation
  size_t size() const;
  /// Get the worst accuracy
  double accuracy() const;
  /// Is approximation good?
  bool isGood() const;
  /// Get all break points
  std::vector<double> getBreakPoints() const;
private:
  void bestFitAnyAccuracy(ChebfunFunctionType fun, double start, double end,
                          double accuracy = 0.0, size_t badSize = 10);
  /// Parts of a piece-wise function
  std::vector<SimpleChebfun> m_parts;
  /// Start of the interval
  double m_startX;
  /// End of the interval
  double m_endX;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_CHEBFUN_H_ */
