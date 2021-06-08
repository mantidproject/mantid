// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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
*/
class SpectrumDefinition {
public:
  SpectrumDefinition() = default;
  explicit SpectrumDefinition(const size_t detectorIndex, const size_t timeIndex = 0)
      : m_data{{detectorIndex, timeIndex}} {}

  /// Returns the size of the SpectrumDefinition, i.e., the number of detectors
  /// (or rather detector positions) that the spectrum comprises.
  size_t size() const { return m_data.size(); }

  /// Returns a const reference to the pair of detector index and time index at
  /// the given `index` in the spectrum definition.
  const std::pair<size_t, size_t> &operator[](const size_t index) const { return m_data[index]; }

  /// Adds a pair of detector index and time index to the spectrum definition.
  /// The time index defaults to zero when not specified.
  void add(const size_t detectorIndex, const size_t timeIndex = 0) {
    auto index = std::make_pair(detectorIndex, timeIndex);
    auto it = std::lower_bound(m_data.begin(), m_data.end(), index);
    if ((it == m_data.end()) || (*it != index))
      m_data.emplace(it, index);
  }

  bool operator==(const SpectrumDefinition &other) const { return m_data == other.m_data; }

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
