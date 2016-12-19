#ifndef MANTID_INDEXING_SPECTRUMNUMBERTRANSLATOR_H_
#define MANTID_INDEXING_SPECTRUMNUMBERTRANSLATOR_H_

#include "MantidIndexing/DllConfig.h"
#include "MantidIndexing/GlobalWorkspaceIndex.h"
#include "MantidIndexing/Partitioning.h"
#include "MantidIndexing/SpectrumIndexSet.h"
#include "MantidIndexing/SpectrumNumber.h"

#include <algorithm>
#include <unordered_map>

namespace Mantid {
namespace Indexing {

/** SpectrumNumberTranslator : TODO: DESCRIPTION

  @author Simon Heybrock
  @date 2016

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
class MANTID_INDEXING_DLL SpectrumNumberTranslator {
public:
  SpectrumNumberTranslator(std::vector<SpectrumNumber> spectrumNumbers,
                           const Partitioning &partitioning,
                           const PartitionIndex &partition)
      : m_partition(partition) {
    partitioning.checkValid(m_partition);

    // This sort is the reason for taking the spectrumNumbers argument by value.
    // We must sort such that the min/max variant of makeIndexSet() makes sense.
    std::sort(spectrumNumbers.begin(), spectrumNumbers.end());

    size_t currentIndex = 0;
    for (size_t i = 0; i < spectrumNumbers.size(); ++i) {
      auto number = spectrumNumbers[i];
      auto partition = partitioning.indexOf(number);
      m_partitions.emplace(number, partition);
      if (partition == m_partition) {
        m_indices.emplace(number, currentIndex);
        m_globalToLocal.emplace(GlobalWorkspaceIndex(i), currentIndex);
        ++currentIndex;
      }
    }
    if (spectrumNumbers.size() != m_partitions.size())
      throw std::logic_error("SpectrumNumberTranslator: The vector of spectrum "
                             "numbers contained duplicate entries.");
  }

  // Full set
  SpectrumIndexSet makeIndexSet() { return SpectrumIndexSet(m_indices.size()); }

  SpectrumIndexSet makeIndexSet(SpectrumNumber min, SpectrumNumber max) {
    // Range check
    static_cast<void>(m_partitions.at(min));
    static_cast<void>(m_partitions.at(max));

    const auto begin = m_indices.lower_bound(min);
    const auto end = m_indices.upper_bound(max);
    if (begin == m_indices.end() || end == m_indices.begin() || begin == end)
      return SpectrumIndexSet(0);
    return SpectrumIndexSet(begin->second, std::prev(end)->second,
                            m_indices.size());
  }

  SpectrumIndexSet makeIndexSet(GlobalWorkspaceIndex min,
                                GlobalWorkspaceIndex max) {
    if (min > max)
      throw std::logic_error(
          "SpectrumIndexTranslator: specified min is larger than max.");
    if (max >= m_partitions.size())
      throw std::out_of_range(
          "SpectrumIndexTranslator: specified max value is out of range.");

    const auto begin = m_globalToLocal.lower_bound(min);
    const auto end = m_globalToLocal.upper_bound(max);
    if (begin == m_globalToLocal.end() || end == m_globalToLocal.begin() ||
        begin == end)
      return SpectrumIndexSet(0);
    return SpectrumIndexSet(begin->second, std::prev(end)->second,
                            m_globalToLocal.size());
  }

  SpectrumIndexSet
  makeIndexSet(const std::vector<SpectrumNumber> &spectrumNumbers) {
    std::vector<size_t> indices;
    for (const auto &spectrumNumber : spectrumNumbers)
      if (m_partitions.at(spectrumNumber) == m_partition)
        indices.push_back(m_indices.at(spectrumNumber));
    return SpectrumIndexSet(indices, m_indices.size());
  }

  SpectrumIndexSet
  makeIndexSet(const std::vector<GlobalWorkspaceIndex> &globalIndices) {
    std::vector<size_t> indices;
    for (const auto &globalIndex : globalIndices) {
      if (globalIndex >= m_partitions.size())
        throw std::out_of_range(
            "SpectrumIndexTranslator: specified index is out of range.");
      const auto it = m_globalToLocal.find(globalIndex);
      if (it != m_globalToLocal.end())
        indices.push_back(std::distance(m_globalToLocal.begin(), it));
    }
    return SpectrumIndexSet(indices, m_globalToLocal.size());
  }

private:
  struct SpectrumNumberHash {
    std::size_t operator()(const SpectrumNumber &spectrumNumber) const {
      return std::hash<std::int32_t>()(
          static_cast<const int32_t>(spectrumNumber));
    }
  };

  const PartitionIndex m_partition;
  std::unordered_map<SpectrumNumber, PartitionIndex, SpectrumNumberHash>
      m_partitions;
  std::map<SpectrumNumber, size_t> m_indices;
  std::map<GlobalWorkspaceIndex, size_t> m_globalToLocal;
};

} // namespace Indexing
} // namespace Mantid

#endif /* MANTID_INDEXING_SPECTRUMNUMBERTRANSLATOR_H_ */
