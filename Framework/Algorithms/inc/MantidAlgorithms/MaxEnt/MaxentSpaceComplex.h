#ifndef MANTID_ALGORITHMS_MAXENTSPACECOMPLEX_H_
#define MANTID_ALGORITHMS_MAXENTSPACECOMPLEX_H_

#include "MantidAlgorithms/MaxEnt/MaxentSpace.h"

namespace Mantid {
namespace Algorithms {

/** MaxentSpaceComplex : Defines a space of complex numbers.

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
class MANTID_ALGORITHMS_DLL MaxentSpaceComplex : public MaxentSpace {
public:
  // Converts a given vector to a complex vector
  std::vector<double> toComplex(const std::vector<double> &values) override;
  // Converts to a complex vector
  std::vector<double> fromComplex(const std::vector<double> &values) override;
};

using MaxentSpaceComplex_sptr =
    boost::shared_ptr<Mantid::Algorithms::MaxentSpaceComplex>;

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_MAXENTSPACECOMPLEX_H_ */
