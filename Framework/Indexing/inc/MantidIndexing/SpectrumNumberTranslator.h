#ifndef MANTID_INDEXING_SPECTRUMNUMBERTRANSLATOR_H_
#define MANTID_INDEXING_SPECTRUMNUMBERTRANSLATOR_H_

#include "MantidIndexing/DllConfig.h"
#include "MantidIndexing/GlobalSpectrumIndex.h"
#include "MantidIndexing/Partitioner.h"
#include "MantidIndexing/SpectrumIndexSet.h"
#include "MantidIndexing/SpectrumNumber.h"

#include <algorithm>
#include <map>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace Mantid {
namespace Indexing {

/** Translates spectrum numbers or global spectrum indices into (local) indices.
  Based on a partitioner, the spectrum numbers or global spectrum indices
  provided as input are filtered such that the returned qunatities (such as
  SpectrumIndexSet) contain only the local subset of the input indices.

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
                           std::unique_ptr<Partitioner> partitioner,
                           const PartitionIndex &partition);

  size_t globalSize() const;
  size_t localSize() const;

  SpectrumNumber spectrumNumber(const size_t index) const;

  SpectrumIndexSet makeIndexSet() const;
  SpectrumIndexSet makeIndexSet(SpectrumNumber min, SpectrumNumber max) const;
  SpectrumIndexSet makeIndexSet(GlobalSpectrumIndex min,
                                GlobalSpectrumIndex max) const;
  SpectrumIndexSet
  makeIndexSet(const std::vector<SpectrumNumber> &spectrumNumbers) const;
  SpectrumIndexSet
  makeIndexSet(const std::vector<GlobalSpectrumIndex> &globalIndices) const;

private:
  bool isPartitioned() const;
  void checkUniqueSpectrumNumbers() const;
  // Not thread-safe! Use only in combination with std::call_once!
  void setupSpectrumNumberToIndexMap() const;

  struct SpectrumNumberHash {
    std::size_t operator()(const SpectrumNumber &spectrumNumber) const {
      return std::hash<std::int32_t>()(
          static_cast<const int32_t>(spectrumNumber));
    }
  };

  bool m_isPartitioned;
  const PartitionIndex m_partition;
  std::unordered_map<SpectrumNumber, PartitionIndex, SpectrumNumberHash>
      m_spectrumNumberToPartition;
  mutable std::vector<std::pair<SpectrumNumber, size_t>>
      m_spectrumNumberToIndex;
  std::vector<std::pair<GlobalSpectrumIndex, size_t>> m_globalToLocal;
  std::vector<SpectrumNumber> m_spectrumNumbers;
  std::vector<SpectrumNumber> m_globalSpectrumNumbers;

  mutable std::once_flag m_mapSetup;
};

/// Returns the global number of spectra.
inline size_t SpectrumNumberTranslator::globalSize() const {
  return m_globalSpectrumNumbers.size();
}

/// Returns the local number of spectra.
inline size_t SpectrumNumberTranslator::localSize() const {
  if (isPartitioned())
    return m_spectrumNumbers.size();
  return globalSize();
}

/// Returns the spectrum number for given index.
inline SpectrumNumber
SpectrumNumberTranslator::spectrumNumber(const size_t index) const {
  if (!isPartitioned())
    return m_globalSpectrumNumbers[index];
  return m_spectrumNumbers[index];
}

// Full set
inline SpectrumIndexSet SpectrumNumberTranslator::makeIndexSet() const {
  return SpectrumIndexSet(localSize());
}

inline bool SpectrumNumberTranslator::isPartitioned() const {
  return m_isPartitioned;
}

} // namespace Indexing
} // namespace Mantid

#endif /* MANTID_INDEXING_SPECTRUMNUMBERTRANSLATOR_H_ */
