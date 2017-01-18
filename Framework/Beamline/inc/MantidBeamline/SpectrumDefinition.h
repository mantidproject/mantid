#ifndef MANTID_BEAMLINE_SPECTRUMDEFINITION_H_
#define MANTID_BEAMLINE_SPECTRUMDEFINITION_H_

#include "MantidBeamline/DllConfig.h"

#include <utility>
#include <vector>

namespace Mantid {
namespace Beamline {

/** SpectrumDefinition is a class that provides a definition of what a spectrum
  comprises, i.e., indices of all detectors that contribute to the data stored
  in the spectrum. Indices have two components: The detector index, which refers
  to a specific detector in the beamline, and a time index, which refers to a
  specific time period in the position table of moving detectors. The latter
  index is always zero, except for beamlines with scanning detectors.


  @author Simon Heybrock
  @date 2017

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
class MANTID_BEAMLINE_DLL SpectrumDefinition {
public:
  size_t size() const;
  const std::pair<size_t, size_t> &operator[](const size_t index) const;
  void add(const size_t detectorIndex, const size_t timeIndex = 0);

private:
  std::vector<std::pair<size_t, size_t>> m_data;

public:
  /// Returns an iterator to the first element of the vector of index pairs.
  auto begin() const -> decltype(m_data.begin()) { return m_data.begin(); }
  /// Returns an iterator to the last element of the vector of index pairs.
  auto end() const -> decltype(m_data.end()) { return m_data.end(); }
  /// Returns an iterator to the first element of the vector of index pairs.
  auto cbegin() const -> decltype(m_data.cbegin()) { return m_data.cbegin(); }
  /// Returns an iterator to the last element of the vector of index pairs.
  auto cend() const -> decltype(m_data.cend()) { return m_data.cend(); }
};

} // namespace Beamline
} // namespace Mantid

#endif /* MANTID_BEAMLINE_SPECTRUMDEFINITION_H_ */
