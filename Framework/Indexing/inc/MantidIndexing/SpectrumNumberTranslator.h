#ifndef MANTID_INDEXING_SPECTRUMNUMBERTRANSLATOR_H_
#define MANTID_INDEXING_SPECTRUMNUMBERTRANSLATOR_H_

#include "MantidIndexing/DllConfig.h"
#include "MantidIndexing/GlobalSpectrumIndex.h"
#include "MantidIndexing/Partitioner.h"
#include "MantidIndexing/SpectrumIndexSet.h"
#include "MantidIndexing/SpectrumNumber.h"

#include <algorithm>
#include <map>
#include <unordered_map>

namespace Mantid {
namespace Indexing {

/** SpectrumNumberTranslator : TODO: DESCRIPTION

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
class MANTID_INDEXING_DLL SpectrumNumberTranslator {
public:
  SpectrumNumberTranslator(const std::vector<SpectrumNumber> &spectrumNumbers,
                           const Partitioner &partitioner,
                           const PartitionIndex &partition);

  SpectrumIndexSet makeIndexSet();
  SpectrumIndexSet makeIndexSet(SpectrumNumber min, SpectrumNumber max);
  SpectrumIndexSet makeIndexSet(GlobalSpectrumIndex min,
                                GlobalSpectrumIndex max);
  SpectrumIndexSet
  makeIndexSet(const std::vector<SpectrumNumber> &spectrumNumbers);
  SpectrumIndexSet
  makeIndexSet(const std::vector<GlobalSpectrumIndex> &globalIndices);

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
  std::map<GlobalSpectrumIndex, size_t> m_globalToLocal;
};

} // namespace Indexing
} // namespace Mantid

#endif /* MANTID_INDEXING_SPECTRUMNUMBERTRANSLATOR_H_ */
