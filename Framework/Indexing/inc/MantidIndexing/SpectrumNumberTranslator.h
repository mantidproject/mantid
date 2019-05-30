// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
*/
class MANTID_INDEXING_DLL SpectrumNumberTranslator {
public:
  SpectrumNumberTranslator(const std::vector<SpectrumNumber> &spectrumNumbers,
                           const Partitioner &partitioner,
                           const PartitionIndex &partition);
  SpectrumNumberTranslator(const std::vector<SpectrumNumber> &spectrumNumbers,
                           const SpectrumNumberTranslator &parent);
  SpectrumNumberTranslator(
      const std::vector<GlobalSpectrumIndex> &globalIndices,
      const SpectrumNumberTranslator &parent);

  size_t globalSize() const;
  size_t localSize() const;

  SpectrumNumber spectrumNumber(const size_t index) const;
  const std::vector<SpectrumNumber> &globalSpectrumNumbers() const;

  SpectrumIndexSet makeIndexSet() const;
  SpectrumIndexSet makeIndexSet(SpectrumNumber min, SpectrumNumber max) const;
  SpectrumIndexSet makeIndexSet(GlobalSpectrumIndex min,
                                GlobalSpectrumIndex max) const;
  SpectrumIndexSet
  makeIndexSet(const std::vector<SpectrumNumber> &spectrumNumbers) const;
  SpectrumIndexSet
  makeIndexSet(const std::vector<GlobalSpectrumIndex> &globalIndices) const;

  PartitionIndex partitionOf(const GlobalSpectrumIndex globalIndex) const;

private:
  bool isPartitioned() const;
  void checkUniqueSpectrumNumbers() const;
  // Not thread-safe! Use only in combination with std::call_once!
  void setupSpectrumNumberToIndexMap() const;
  std::vector<SpectrumNumber>
  spectrumNumbers(const std::vector<GlobalSpectrumIndex> &globalIndices) const;

  struct SpectrumNumberHash {
    // Pass-by-value as qualifiers ignored on cast result type and
    // SpectrumNumber is a trivial type (int32_t).
    std::size_t operator()(SpectrumNumber spectrumNumber) const {
      return std::hash<std::int32_t>()(static_cast<int32_t>(spectrumNumber));
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
