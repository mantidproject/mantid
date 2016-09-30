#ifndef MANTID_ELASTICISOROTDIFF_H_
#define MANTID_ELASTICISOROTDIFF_H_

// Mantid Coding standars <http://www.mantidproject.org/Coding_Standards>
// Mantid Headers from the same project
#include "MantidCurveFitting/Functions/DeltaFunction.h"
// Mantid headers from other projects (N/A)
// 3rd party library headers (N/A)
// standard library headers (N/A)

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
 * @brief Elastic part of the DiffSphere function
 */
class DLLExport ElasticIsoRotDiff : public DeltaFunction {
public:
  /// Constructor
  ElasticIsoRotDiff();

  /// overwrite IFunction base class methods
  std::string name() const override { return "ElasticIsoRotDiff"; }

  const std::string category() const override { return "QuasiElastic"; }

  /// A rescaling of the peak intensity
  double HeightPrefactor() const override;

  /// overwrite IFunction base class method, which declare function parameters
  void init() override;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif // MANTID_ELASTICISOROTDIFF_H_
