#ifndef MANTID_ALGORITHMS_MAXENTSPACEREAL_H_
#define MANTID_ALGORITHMS_MAXENTSPACEREAL_H_

#include "MantidAlgorithms/MaxEnt/MaxentSpace.h"

namespace Mantid {
namespace Algorithms {

/** MaxentSpaceReal : Defines the space of real numbers.

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_ALGORITHMS_DLL MaxentSpaceReal : public MaxentSpace {
public:
  // Converts a real vector to a complex vector
  std::vector<double> toComplex(const std::vector<double> &values) override;
  // Converts a complex vector to a real vector
  std::vector<double> fromComplex(const std::vector<double> &values) override;
};

using MaxentSpaceReal_sptr =
boost::shared_ptr<Mantid::Algorithms::MaxentSpaceReal>;

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_MAXENTSPACEREAL_H_ */
