// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidIndexing/IndexInfo.h"
#include "MantidIndexing/RoundRobinPartitioner.h"
#include "MantidIndexing/SpectrumNumberTranslator.h"
#include "MantidKernel/make_cow.h"

#include "MantidTypes/SpectrumDefinition.h"

#include <algorithm>
#include <functional>
#include <numeric>

namespace Mantid::Indexing {

/// Construct a default IndexInfo, with contiguous spectrum numbers starting at
/// 1 and no spectrum definitions.
IndexInfo::IndexInfo(const size_t globalSize) {
  // Default to spectrum numbers 1...globalSize
  std::vector<SpectrumNumber> specNums(globalSize);
  std::iota(specNums.begin(), specNums.end(), 1);
  makeSpectrumNumberTranslator(std::move(specNums));
}

/// Construct with given spectrum number for each index and no spectrum
/// definitions.
IndexInfo::IndexInfo(std::vector<SpectrumNumber> spectrumNumbers) {
  makeSpectrumNumberTranslator(std::move(spectrumNumbers));
}

/** Construct with given index subset of parent.
 *
 * The template argument IndexType can be SpectrumNumber or GlobalSpectrumIndex.
 * The parent defines the partitioning of the spectrum numbers, i.e., the
 * partition assigned to a given spectrum number in the constructed IndexInfo is
 * given by the partition that spectrum number has in parent. This is used to
 * extract spectrum numbers while maintaining the partitioning, avoiding the
 * need to redistribute data between partitions (MPI ranks). Throws if any of
 * the spectrum numbers is not present in parent. */
template <class IndexType> IndexInfo::IndexInfo(std::vector<IndexType> indices, const IndexInfo &parent) {
  if (const auto parentSpectrumDefinitions = parent.spectrumDefinitions()) {
    m_spectrumDefinitions = Kernel::make_cow<std::vector<SpectrumDefinition>>();
    const auto &indexSet = parent.makeIndexSet(indices);
    auto &specDefs = m_spectrumDefinitions.access();
    specDefs.reserve(specDefs.size() + indexSet.size());
    std::transform(indexSet.begin(), indexSet.end(), std::back_inserter(specDefs),
                   [&parentSpectrumDefinitions](const auto index) { return (*parentSpectrumDefinitions)[index]; });
  }
  m_spectrumNumberTranslator =
      Kernel::make_cow<SpectrumNumberTranslator>(std::move(indices), *parent.m_spectrumNumberTranslator);
}

IndexInfo::IndexInfo(const IndexInfo &other)
    : m_spectrumDefinitions(other.m_spectrumDefinitions), m_spectrumNumberTranslator(other.m_spectrumNumberTranslator) {
}

IndexInfo::IndexInfo(IndexInfo &&) noexcept = default;

// Defined as default in source for forward declaration with std::unique_ptr.
IndexInfo::~IndexInfo() = default;

IndexInfo &IndexInfo::operator=(const IndexInfo &other) {
  auto copy(other);
  *this = std::move(copy);
  return *this;
}

IndexInfo &IndexInfo::operator=(IndexInfo &&) noexcept = default;

/// The *local* size, i.e., the number of spectra in this partition.
size_t IndexInfo::size() const {
  if (!m_spectrumNumberTranslator)
    return 0;
  return m_spectrumNumberTranslator->localSize();
}

/// The *global* size, i.e., the total number of spectra across all partitions.
size_t IndexInfo::globalSize() const {
  if (!m_spectrumNumberTranslator)
    return 0;
  return m_spectrumNumberTranslator->globalSize();
}

/// Returns the spectrum number for given *local* index, i.e., spectrum numbers
/// for spectra in this partition.
SpectrumNumber IndexInfo::spectrumNumber(const size_t index) const {
  return m_spectrumNumberTranslator->spectrumNumber(index);
}

/// Returns a reference to the *global* vector of spectrum numbers, i.e., the
/// spectrum numbers of spectra across all partitions.
const std::vector<SpectrumNumber> &IndexInfo::spectrumNumbers() const {
  return m_spectrumNumberTranslator->globalSpectrumNumbers();
}

/// Set a spectrum number for each index.
void IndexInfo::setSpectrumNumbers(std::vector<SpectrumNumber> &&spectrumNumbers) {
  if (m_spectrumNumberTranslator->globalSize() != spectrumNumbers.size())
    throw std::runtime_error("IndexInfo::setSpectrumNumbers: Size mismatch. "
                             "The vector must contain a spectrum number for "
                             "each spectrum (not just for the local "
                             "partition).");
  makeSpectrumNumberTranslator(std::move(spectrumNumbers));
}

/// Set a contiguous range of spectrum numbers.
void IndexInfo::setSpectrumNumbers(const SpectrumNumber min, const SpectrumNumber max) {
  auto newSize = static_cast<int32_t>(max) - static_cast<int32_t>(min) + 1;
  if (static_cast<int64_t>(m_spectrumNumberTranslator->globalSize()) != newSize)
    throw std::runtime_error("IndexInfo::setSpectrumNumbers: Size mismatch. "
                             "The range of spectrum numbers must provide a "
                             "spectrum number for each spectrum (not just for "
                             "the local partition).");
  std::vector<SpectrumNumber> specNums(newSize);
  std::iota(specNums.begin(), specNums.end(), static_cast<int32_t>(min));
  makeSpectrumNumberTranslator(std::move(specNums));
}

/// Set the spectrum definitions.
void IndexInfo::setSpectrumDefinitions(std::vector<SpectrumDefinition> spectrumDefinitions) {
  if (size() != spectrumDefinitions.size())
    throw std::runtime_error("IndexInfo: Size mismatch when setting new spectrum definitions");
  m_spectrumDefinitions = Kernel::make_cow<std::vector<SpectrumDefinition>>(std::move(spectrumDefinitions));
}

/** Set the spectrum definitions.
 *
 * Note that in principle the spectrum definitions contain the same information
 * as the groups of detector IDs. However, Mantid currently supports invalid
 * detector IDs in groups, whereas spectrum definitions contain only valid
 * indices. Validation requires access to the instrument and thus cannot be done
 * internally in IndexInfo, i.e., spectrum definitions must be set by hand. */
void IndexInfo::setSpectrumDefinitions(const Kernel::cow_ptr<std::vector<SpectrumDefinition>> &spectrumDefinitions) {
  if (!spectrumDefinitions || (size() != spectrumDefinitions->size()))
    throw std::runtime_error("IndexInfo: Size mismatch when setting new spectrum definitions");
  m_spectrumDefinitions = spectrumDefinitions;
}

/// Returns the spectrum definitions.
const Kernel::cow_ptr<std::vector<SpectrumDefinition>> &IndexInfo::spectrumDefinitions() const {
  return m_spectrumDefinitions;
}

/** Creates an index set containing all indices.
 *
 * If there are multiple partitions (MPI ranks), the returned set contains the
 * subset of indices on this partition. */
SpectrumIndexSet IndexInfo::makeIndexSet() const { return m_spectrumNumberTranslator->makeIndexSet(); }

/** Creates an index set containing all indices with spectrum number between
 * `min` and `max`.
 *
 * If there are multiple partitions (MPI ranks), the returned set contains the
 * subset of indices on this partition. */
SpectrumIndexSet IndexInfo::makeIndexSet(SpectrumNumber min, SpectrumNumber max) const {
  return m_spectrumNumberTranslator->makeIndexSet(min, max);
}

/** Creates an index set containing all indices with global index between `min`
 * and `max`.
 *
 * If there are multiple partitions (MPI ranks), the returned set contains the
 * subset of indices on this partition. */
SpectrumIndexSet IndexInfo::makeIndexSet(GlobalSpectrumIndex min, GlobalSpectrumIndex max) const {
  return m_spectrumNumberTranslator->makeIndexSet(min, max);
}

/** Creates an index set containing all indices corresponding to the spectrum
 * numbers in the provided vector.
 *
 * If there are multiple partitions (MPI ranks), the returned set contains the
 * subset of indices on this partition. */
SpectrumIndexSet IndexInfo::makeIndexSet(const std::vector<SpectrumNumber> &spectrumNumbers) const {
  return m_spectrumNumberTranslator->makeIndexSet(spectrumNumbers);
}

/** Creates an index set containing all indices corresponding to the global
 * indices in the provided vector.
 *
 * If there are multiple partitions (MPI ranks), the returned set contains the
 * subset of indices on this partition. */
SpectrumIndexSet IndexInfo::makeIndexSet(const std::vector<GlobalSpectrumIndex> &globalIndices) const {
  return m_spectrumNumberTranslator->makeIndexSet(globalIndices);
}

/** Map a vector of detector indices to a vector of global spectrum indices. *
 * The mapping is based on the held spectrum definitions.
 * Throws if any spectrum maps to more than one detectors.
 * Throws if any of the detectors has no matching spectrum.
 * Throws if more than one spectrum maps to the same detector at the same time
 * index.
 */
std::vector<GlobalSpectrumIndex>
IndexInfo::globalSpectrumIndicesFromDetectorIndices(const std::vector<size_t> &detectorIndices) const {
  if (!m_spectrumDefinitions)
    throw std::runtime_error("IndexInfo::"
                             "globalSpectrumIndicesFromDetectorIndices -- no "
                             "spectrum definitions available");
  /*
   * We need some way of keeping track of which time indices of given detector
   * have a matching mapping. detectorMap holds pairs; first in the pair
   * indicates the detector ID, used to check if there is mapping with the
   * detector, regardless of time index. The second, which is a vector indexed
   * by time indices for the given detector, is only used to count and check
   * whether more than one spectra map to the same detector at the same time
   * index. The first item in the pair means as follows:
   * 0 : placeholder value, see the comment below
   * 1 : the detector is requested (i.e. is in the detectorIndices)
   * 2 : a matching spectrum has been found
   */
  std::vector<std::pair<char, std::vector<char>>> detectorMap;
  for (const auto &index : detectorIndices) {
    // IndexInfo has no knowledge of the maximum detector index so we workaround
    // this knowledge gap by assuming below that any index beyond the end of the
    // map is 0.
    if (index >= detectorMap.size())
      detectorMap.resize(index + 1, std::make_pair(0, std::vector<char>()));
    detectorMap[index].first = 1;
  }

  // Global vector of spectrum definitions. For this purpose we do not need
  // actual definitions either single detector or error flag.
  std::vector<std::pair<int64_t, size_t>> specDefinitions;
  specDefinitions.resize(size());
  for (size_t i = 0; i < size(); ++i) {
    const auto &spectrumDefinition = m_spectrumDefinitions->operator[](i);
    if (spectrumDefinition.size() == 1) {
      const auto detectorIndex = spectrumDefinition[0].first;
      const auto timeIndex = spectrumDefinition[0].second;
      specDefinitions[i] = std::make_pair(detectorIndex, timeIndex);
    }
    // detectorIndex is unsigned so we can use negative values as error flags.
    if (spectrumDefinition.size() == 0)
      specDefinitions[i] = {-1, 0};
    if (spectrumDefinition.size() > 1)
      specDefinitions[i] = {-2, 0};
  }

  std::vector<GlobalSpectrumIndex> spectrumIndices;
  for (size_t i = 0; i < globalSize(); ++i) {
    const auto spectrumDefinition = specDefinitions[i];
    if (spectrumDefinition.first >= 0) {
      const auto detectorIndex = static_cast<size_t>(spectrumDefinition.first);
      const auto timeIndex = static_cast<size_t>(spectrumDefinition.second);
      if (detectorMap.size() > detectorIndex && detectorMap[detectorIndex].first != 0) {
        spectrumIndices.emplace_back(i);
        if (detectorMap[detectorIndex].first == 1) {
          ++detectorMap[detectorIndex].first;
        }
        if (detectorMap[detectorIndex].second.size() <= timeIndex) {
          detectorMap[detectorIndex].second.resize(timeIndex + 1, {0});
          detectorMap[detectorIndex].second[timeIndex] = 1;
        } else {
          ++detectorMap[detectorIndex].second[timeIndex];
        }
      }
    }
    if (spectrumDefinition.first == -2)
      throw std::runtime_error("SpectrumDefinition contains multiple entries. "
                               "No unique mapping from detector to spectrum "
                               "possible");
  }

  if (std::any_of(detectorMap.begin(), detectorMap.end(),
                  [](const std::pair<char, std::vector<char>> &p) { return p.first == 1; })) {
    throw std::runtime_error("Some of the requested detectors do not have a "
                             "corresponding spectrum");
  }
  if (std::any_of(detectorMap.begin(), detectorMap.end(), [](const std::pair<char, std::vector<char>> &p) {
        return std::any_of(p.second.begin(), p.second.end(), [](char c) { return c > 1; });
      })) {
    throw std::runtime_error("Some of the spectra map to the same detector "
                             "at the same time index");
  }

  if (detectorIndices.size() > spectrumIndices.size())
    throw std::runtime_error("Some of the requested detectors do not have a "
                             "corresponding spectrum");
  return spectrumIndices;
}

/// Returns true if the given global index is on this partition.
bool IndexInfo::isOnThisPartition(GlobalSpectrumIndex globalIndex) const {
  // A map from global index to partition might be faster, consider adding this
  // if it is used a lot and has performance issues.
  const auto helperSet = makeIndexSet(globalIndex, globalIndex);
  return helperSet.size() == 1;
}

void IndexInfo::makeSpectrumNumberTranslator(std::vector<SpectrumNumber> &&spectrumNumbers) const {
  PartitionIndex partition = PartitionIndex(0);
  int numberOfPartitions = 1;

  auto partitioner = std::make_unique<RoundRobinPartitioner>(numberOfPartitions, partition,
                                                             Partitioner::MonitorStrategy::TreatAsNormalSpectrum);
  m_spectrumNumberTranslator =
      Kernel::make_cow<SpectrumNumberTranslator>(std::move(spectrumNumbers), *partitioner, partition);
}

template MANTID_INDEXING_DLL IndexInfo::IndexInfo(std::vector<SpectrumNumber>, const IndexInfo &);
template MANTID_INDEXING_DLL IndexInfo::IndexInfo(std::vector<GlobalSpectrumIndex>, const IndexInfo &);

} // namespace Mantid::Indexing
