#ifndef MANTID_ALGORITHMS_DETECTORGRIDDEFINITION_H_
#define MANTID_ALGORITHMS_DETECTORGRIDDEFINITION_H_

#include "MantidAlgorithms/DllConfig.h"

#include <array>

namespace Mantid {
namespace Algorithms {

/** DetectorGridDefinition is a helper class for building the sparse
  instrument in MonteCarloAbsorption.

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_ALGORITHMS_DLL DetectorGridDefinition {
public:
  DetectorGridDefinition(const double minLatitude, const double maxLatitude,
                         const size_t latitudePoints, const double minLongitude,
                         const double maxLongitude, const size_t longitudeStep);

  double latitudeAt(const size_t row) const;
  double longitudeAt(const size_t column) const;
  std::array<size_t, 4> nearestNeighbourIndices(const double latitude,
                                                const double longitude) const;
  size_t numberColumns() const;
  size_t numberRows() const;

private:
  double m_minLatitude;
  double m_maxLatitude;
  size_t m_latitudePoints;
  double m_latitudeStep;
  double m_minLongitude;
  double m_maxLongitude;
  size_t m_longitudePoints;
  double m_longitudeStep;
};

} // namespace Algorithms
} // namespace Mantid

#endif /* MANTID_ALGORITHMS_DETECTORGRIDDEFINITION_H_ */
