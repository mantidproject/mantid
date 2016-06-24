#ifndef MANTID_ELASTICDIFFROTDISCRETECIRCLE_H_
#define MANTID_ELASTICDIFFROTDISCRETECIRCLE_H_

// Mantid Coding standards <http://www.mantidproject.org/Coding_Standards>
// Mantid Headers from the same project
#include "DeltaFunction.h"
// Mantid headers from other projects (N/A)
// 3rd party library headers (N/A)
// standard library (N/A)

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
@author Jose Borreguero, NScD
@date July 24 2016

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

/* Class representing the elastic portion of DiffRotDiscreteCircle
 * Contains a Delta Dirac.
 */
class DLLExport ElasticDiffRotDiscreteCircle : public DeltaFunction {
public:
  /// Constructor
  ElasticDiffRotDiscreteCircle();

  /// overwrite IFunction base class methods
  std::string name() const override { return "ElasticDiffRotDiscreteCircle"; }

  const std::string category() const override { return "QuasiElastic"; }

  /// overwrite IFunction base class method, which declare function parameters
  void init() override;

  /// A rescaling of the peak intensity
  double HeightPrefactor() const override;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif // MANTID_ELASTICDIFFROTDISCRETECIRCLE_H_
