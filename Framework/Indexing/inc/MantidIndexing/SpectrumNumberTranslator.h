#ifndef MANTID_INDEXING_SPECTRUMNUMBERTRANSLATOR_H_
#define MANTID_INDEXING_SPECTRUMNUMBERTRANSLATOR_H_

#include "MantidIndexing/DllConfig.h"
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
      if (partition == m_partition)
        m_indices.emplace(number, currentIndex++);
    }
    if (spectrumNumbers.size() != m_partitions.size())
      throw std::logic_error("SpectrumNumberTranslator: The vector of spectrum "
                             "numbers contained duplicate entries.");
  }

  // Full set
  SpectrumIndexSet makeIndexSet() { return SpectrumIndexSet(m_indices.size()); }

  SpectrumIndexSet makeIndexSet(SpectrumNumber min, SpectrumNumber max) {
    const auto min_iterator = m_partitions.find(min);
    const auto max_iterator = m_partitions.find(max);
    if (min_iterator == m_partitions.end() ||
        max_iterator == m_partitions.end())
      throw std::out_of_range("Invalid spectrum number.");

    auto begin = m_indices.lower_bound(min);
    auto end = m_indices.upper_bound(max);
    if (begin == m_indices.end() || end == m_indices.begin() || begin == end)
      return SpectrumIndexSet(0);
    return SpectrumIndexSet(begin->second, (--end)->second, m_indices.size());
  }

  SpectrumIndexSet
  makeIndexSet(const std::vector<SpectrumNumber> &spectrumNumbers) {
    std::vector<size_t> indices;
    for (const auto &spectrumNumber : spectrumNumbers) {
      const auto rank_iterator = m_partitions.find(spectrumNumber);
      if (rank_iterator == m_partitions.end())
        throw std::out_of_range("Invalid spectrum number.");
      if (rank_iterator->second == m_partition)
        indices.push_back(m_indices.at(spectrumNumber));
    }
    return SpectrumIndexSet(indices, m_indices.size());
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
};

} // namespace Indexing
} // namespace Mantid

#endif /* MANTID_INDEXING_SPECTRUMNUMBERTRANSLATOR_H_ */
