#ifndef MANTID_INDEXING_DETECTORIDTRANSLATOR_H_
#define MANTID_INDEXING_DETECTORIDTRANSLATOR_H_

#include "MantidIndexing/DllConfig.h"
#include "MantidIndexing/DetectorIndexSet.h"
#include "MantidIndexing/DetectorID.h"
#include "MantidIndexing/Partitioning.h"
#include "MantidIndexing/SpectrumNumber.h"

#include <unordered_map>

namespace Mantid {
namespace Indexing {

/** DetectorIDTranslator : TODO: DESCRIPTION

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
class MANTID_INDEXING_DLL DetectorIDTranslator {
public:
  DetectorIDTranslator(
      const std::vector<std::pair<SpectrumNumber, std::vector<DetectorID>>> &
          spectrumDefinitions,
      const Partitioning &partitioning, const PartitionIndex &partition)
      : m_partition(partition) {
    partitioning.checkValid(m_partition);

    size_t currentIndex = 0;
    for (const auto spectrumDefinition : spectrumDefinitions) {
      auto number = spectrumDefinition.first;
      auto partition = partitioning.indexOf(number);
      bool inThisPartition = (partition == m_partition);
      for (const auto detectorID : spectrumDefinition.second) {
        m_inThisPartition.emplace(detectorID, inThisPartition);
        if (inThisPartition)
          m_indices.emplace(detectorID, currentIndex++);
      }
    }
  }

  // Full set
  DetectorIndexSet makeIndexSet() { return DetectorIndexSet(m_indices.size()); }

  // This one is more difficult with MPI, need to deal with partial overlaps,
  // etc.
  // DetectorIndexSet makeIndexSet(DetectorID min, DetectorID max);

  DetectorIndexSet makeIndexSet(const std::vector<DetectorID> &detectorIDs) {
    std::vector<size_t> indices;
    for (const auto &detectorID : detectorIDs) {
      const auto rank_iterator = m_inThisPartition.find(detectorID);
      if (rank_iterator == m_inThisPartition.end())
        throw std::out_of_range("Invalid detector ID.");
      if (rank_iterator->second)
        indices.push_back(m_indices.at(detectorID));
    }
    return DetectorIndexSet(indices, m_indices.size());
  }

private:
  struct DetectorIDHash {
    std::size_t operator()(const DetectorID &detectorID) const {
      return std::hash<std::int32_t>()(static_cast<const int32_t>(detectorID));
    }
  };

  const PartitionIndex m_partition;
  std::unordered_map<DetectorID, bool, DetectorIDHash> m_inThisPartition;
  std::unordered_map<DetectorID, size_t, DetectorIDHash> m_indices;
};

} // namespace Indexing
} // namespace Mantid

#endif /* MANTID_INDEXING_DETECTORIDTRANSLATOR_H_ */
