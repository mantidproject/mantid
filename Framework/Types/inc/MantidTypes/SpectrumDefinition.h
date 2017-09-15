#ifndef MANTID_TYPES_SPECTRUMDEFINITION_H_
#define MANTID_TYPES_SPECTRUMDEFINITION_H_

#include <algorithm>
#include <utility>
#include <vector>

namespace Mantid {

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
class SpectrumDefinition {
public:
  /// Returns the size of the SpectrumDefinition, i.e., the number of detectors
  /// (or rather detector positions) that the spectrum comprises.
  size_t size() const { return m_data.size(); }

  /// Returns a const reference to the pair of detector index and time index at
  /// the given `index` in the spectrum definition.
  const std::pair<size_t, size_t> &operator[](const size_t index) const {
    return m_data[index];
  }

  /// Adds a pair of detector index and time index to the spectrum definition.
  /// The time index defaults to zero when not specified.
  void add(const size_t detectorIndex, const size_t timeIndex = 0) {
    auto index = std::make_pair(detectorIndex, timeIndex);
    auto it = std::lower_bound(m_data.begin(), m_data.end(), index);
    if ((it == m_data.end()) || (*it != index))
      m_data.emplace(it, index);
  }

  bool operator==(const SpectrumDefinition &other) const {
    return m_data == other.m_data;
  }

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

} // namespace Mantid

#endif /* MANTID_TYPES_SPECTRUMDEFINITION_H_ */
