#include "MantidIndexing/IndexInfo.h"
#include "MantidIndexing/RoundRobinPartitioner.h"
#include "MantidIndexing/SpectrumNumberTranslator.h"
#include "MantidKernel/make_cow.h"
#include "MantidKernel/make_unique.h"
#include "MantidParallel/Collectives.h"
#include "MantidParallel/Communicator.h"
#include "MantidTypes/SpectrumDefinition.h"

#include <algorithm>
#include <functional>
#include <numeric>

namespace Mantid {
namespace Indexing {

/// Construct a default IndexInfo, with contiguous spectrum numbers starting at
/// 1 and no spectrum definitions.
IndexInfo::IndexInfo(const size_t globalSize,
                     const Parallel::StorageMode storageMode)
    : IndexInfo(globalSize, storageMode, Parallel::Communicator{}) {}

/// Construct a default IndexInfo, with contiguous spectrum numbers starting at
/// 1 and no spectrum definitions.
IndexInfo::IndexInfo(const size_t globalSize,
                     const Parallel::StorageMode storageMode,
                     const Parallel::Communicator &communicator)
    : m_storageMode(storageMode),
      m_communicator(
          Kernel::make_unique<Parallel::Communicator>(communicator)) {
  // Default to spectrum numbers 1...globalSize
  std::vector<SpectrumNumber> specNums(globalSize);
  std::iota(specNums.begin(), specNums.end(), 1);
  makeSpectrumNumberTranslator(std::move(specNums));
}

/// Construct with given spectrum number for each index and no spectrum
/// definitions.
IndexInfo::IndexInfo(std::vector<SpectrumNumber> spectrumNumbers,
                     const Parallel::StorageMode storageMode)
    : IndexInfo(std::move(spectrumNumbers), storageMode,
                Parallel::Communicator{}) {}

/// Construct with given spectrum number for each index and no spectrum
/// definitions.
IndexInfo::IndexInfo(std::vector<SpectrumNumber> spectrumNumbers,
                     const Parallel::StorageMode storageMode,
                     const Parallel::Communicator &communicator)
    : m_storageMode(storageMode),
      m_communicator(
          Kernel::make_unique<Parallel::Communicator>(communicator)) {
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
template <class IndexType>
IndexInfo::IndexInfo(std::vector<IndexType> indices, const IndexInfo &parent)
    : m_storageMode(parent.m_storageMode),
      m_communicator(
          Kernel::make_unique<Parallel::Communicator>(*parent.m_communicator)) {
  if (const auto parentSpectrumDefinitions = parent.spectrumDefinitions()) {
    m_spectrumDefinitions = Kernel::make_cow<std::vector<SpectrumDefinition>>();
    auto &specDefs = m_spectrumDefinitions.access();
    for (const auto i : parent.makeIndexSet(indices))
      specDefs.push_back(parentSpectrumDefinitions->operator[](i));
  }
  m_spectrumNumberTranslator = Kernel::make_cow<SpectrumNumberTranslator>(
      std::move(indices), *parent.m_spectrumNumberTranslator);
}

IndexInfo::IndexInfo(const IndexInfo &other)
    : m_storageMode(other.m_storageMode),
      m_communicator(
          Kernel::make_unique<Parallel::Communicator>(*other.m_communicator)),
      m_spectrumDefinitions(other.m_spectrumDefinitions),
      m_spectrumNumberTranslator(other.m_spectrumNumberTranslator) {}

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
void IndexInfo::setSpectrumNumbers(
    std::vector<SpectrumNumber> &&spectrumNumbers) {
  if (m_spectrumNumberTranslator->globalSize() != spectrumNumbers.size())
    throw std::runtime_error("IndexInfo::setSpectrumNumbers: Size mismatch. "
                             "The vector must contain a spectrum number for "
                             "each spectrum (not just for the local "
                             "partition).");
  makeSpectrumNumberTranslator(std::move(spectrumNumbers));
}

/// Set a contiguous range of spectrum numbers.
void IndexInfo::setSpectrumNumbers(const SpectrumNumber min,
                                   const SpectrumNumber max) {
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
void IndexInfo::setSpectrumDefinitions(
    std::vector<SpectrumDefinition> spectrumDefinitions) {
  if (size() != spectrumDefinitions.size())
    throw std::runtime_error(
        "IndexInfo: Size mismatch when setting new spectrum definitions");
  m_spectrumDefinitions = Kernel::make_cow<std::vector<SpectrumDefinition>>(
      std::move(spectrumDefinitions));
}

/** Set the spectrum definitions.
 *
 * Note that in principle the spectrum definitions contain the same information
 * as the groups of detector IDs. However, Mantid currently supports invalid
 * detector IDs in groups, whereas spectrum definitions contain only valid
 * indices. Validation requires access to the instrument and thus cannot be done
 * internally in IndexInfo, i.e., spectrum definitions must be set by hand. */
void IndexInfo::setSpectrumDefinitions(
    Kernel::cow_ptr<std::vector<SpectrumDefinition>> spectrumDefinitions) {
  if (!spectrumDefinitions || (size() != spectrumDefinitions->size()))
    throw std::runtime_error(
        "IndexInfo: Size mismatch when setting new spectrum definitions");
  m_spectrumDefinitions = spectrumDefinitions;
}

/// Returns the spectrum definitions.
const Kernel::cow_ptr<std::vector<SpectrumDefinition>> &
IndexInfo::spectrumDefinitions() const {
  return m_spectrumDefinitions;
}

/** Creates an index set containing all indices.
 *
 * If there are multiple partitions (MPI ranks), the returned set contains the
 * subset of indices on this partition. */
SpectrumIndexSet IndexInfo::makeIndexSet() const {
  return m_spectrumNumberTranslator->makeIndexSet();
}

/** Creates an index set containing all indices with spectrum number between
 * `min` and `max`.
 *
 * If there are multiple partitions (MPI ranks), the returned set contains the
 * subset of indices on this partition. */
SpectrumIndexSet IndexInfo::makeIndexSet(SpectrumNumber min,
                                         SpectrumNumber max) const {
  return m_spectrumNumberTranslator->makeIndexSet(min, max);
}

/** Creates an index set containing all indices with global index between `min`
 * and `max`.
 *
 * If there are multiple partitions (MPI ranks), the returned set contains the
 * subset of indices on this partition. */
SpectrumIndexSet IndexInfo::makeIndexSet(GlobalSpectrumIndex min,
                                         GlobalSpectrumIndex max) const {
  return m_spectrumNumberTranslator->makeIndexSet(min, max);
}

/** Creates an index set containing all indices corresponding to the spectrum
 * numbers in the provided vector.
 *
 * If there are multiple partitions (MPI ranks), the returned set contains the
 * subset of indices on this partition. */
SpectrumIndexSet IndexInfo::makeIndexSet(
    const std::vector<SpectrumNumber> &spectrumNumbers) const {
  return m_spectrumNumberTranslator->makeIndexSet(spectrumNumbers);
}

/** Creates an index set containing all indices corresponding to the global
 * indices in the provided vector.
 *
 * If there are multiple partitions (MPI ranks), the returned set contains the
 * subset of indices on this partition. */
SpectrumIndexSet IndexInfo::makeIndexSet(
    const std::vector<GlobalSpectrumIndex> &globalIndices) const {
  return m_spectrumNumberTranslator->makeIndexSet(globalIndices);
}

/** Map a vector of detector indices to a vector of global spectrum indices.
 *
 * The mapping is based on the held spectrum definitions. Throws if any spectrum
 * maps to more than one detectors. Throws if there is no 1:1 mapping from
 * detectors to spectra, such as when some of the detectors have no matching
 * spectrum. */
std::vector<GlobalSpectrumIndex>
IndexInfo::globalSpectrumIndicesFromDetectorIndices(
    const std::vector<size_t> &detectorIndices) const {
  if (!m_spectrumDefinitions)
    throw std::runtime_error("IndexInfo::"
                             "globalSpectrumIndicesFromDetectorIndices -- no "
                             "spectrum definitions available");
  std::vector<char> detectorMap;
  for (const auto &index : detectorIndices) {
    // IndexInfo has no knowledge of the maximum detector index so we workaround
    // this knowledge gap by assuming below that any index beyond the end of the
    // map is 0.
    if (index >= detectorMap.size())
      detectorMap.resize(index + 1, 0);
    detectorMap[index] = 1;
  }

  // Global vector of spectrum definitions. For this purpose we do not need
  // actual definitions which would be hard to transmit via MPI (many small
  // vectors of unknown length). Either single detector or error flag.
  std::vector<std::vector<int64_t>> spectrumDefinitions(communicator().size());
  auto &thisRankSpectrumDefinitions =
      spectrumDefinitions[communicator().rank()];
  thisRankSpectrumDefinitions.resize(size());
  for (size_t i = 0; i < size(); ++i) {
    const auto &spectrumDefinition = m_spectrumDefinitions->operator[](i);
    if (spectrumDefinition.size() == 1) {
      const auto detectorIndex = spectrumDefinition[0].first;
      thisRankSpectrumDefinitions[i] = detectorIndex;
    }
    // detectorIndex is unsigned so we can use negative values as error flags.
    if (spectrumDefinition.size() == 0)
      thisRankSpectrumDefinitions[i] = -1;
    if (spectrumDefinition.size() > 1)
      thisRankSpectrumDefinitions[i] = -2;
  }

  std::vector<size_t> allSizes;
  Parallel::gather(communicator(), size(), allSizes, 0);
  std::vector<GlobalSpectrumIndex> spectrumIndices;
  int tag = 0;
  if (communicator().rank() == 0) {
    for (int rank = 1; rank < communicator().size(); ++rank) {
      spectrumDefinitions[rank].resize(allSizes[rank]);
      auto buffer = reinterpret_cast<char *>(spectrumDefinitions[rank].data());
      auto bytes = static_cast<int>(sizeof(int64_t) * allSizes[rank]);
      communicator().recv(rank, tag, buffer, bytes);
    }
    std::vector<size_t> currentIndex(communicator().size(), 0);
    for (size_t i = 0; i < globalSize(); ++i) {
      int rank = static_cast<int>(
          m_spectrumNumberTranslator->partitionOf(GlobalSpectrumIndex(i)));
      const auto spectrumDefinition =
          spectrumDefinitions[rank][currentIndex[rank]++];
      if (spectrumDefinition >= 0) {
        const auto detectorIndex = static_cast<size_t>(spectrumDefinition);
        if (detectorMap.size() > detectorIndex &&
            detectorMap[detectorIndex] != 0) {
          if (detectorMap[detectorIndex] > 1)
            throw std::runtime_error(
                "Multiple spectra correspond to the same detector");
          // Increment flag to catch two spectra mapping to same detector.
          ++detectorMap[detectorIndex];
          spectrumIndices.push_back(i);
        }
      }
      if (spectrumDefinition == -2)
        throw std::runtime_error(
            "SpectrumDefinition contains multiple entries. "
            "No unique mapping from detector to spectrum "
            "possible");
    }
    for (int rank = 1; rank < communicator().size(); ++rank) {
      auto buffer = reinterpret_cast<char *>(spectrumIndices.data());
      auto bytes = static_cast<int>(sizeof(int64_t) * spectrumIndices.size());
      communicator().send(rank, tag, buffer, bytes);
    }
  } else {
    auto buffer = reinterpret_cast<char *>(thisRankSpectrumDefinitions.data());
    auto bytes = static_cast<int>(sizeof(int64_t) * size());
    communicator().send(0, tag, buffer, bytes);
    spectrumIndices.resize(detectorIndices.size());
    buffer = reinterpret_cast<char *>(spectrumIndices.data());
    bytes = static_cast<int>(sizeof(int64_t) * spectrumIndices.size());
    const auto status = communicator().recv(0, tag, buffer, bytes);
    spectrumIndices.resize(*status.count<int64_t>());
  }

  if (detectorIndices.size() != spectrumIndices.size())
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

/// Returns the storage mode used in MPI runs.
Parallel::StorageMode IndexInfo::storageMode() const { return m_storageMode; }

/// Returns the communicator used in MPI runs.
const Parallel::Communicator &IndexInfo::communicator() const {
  return *m_communicator;
}

void IndexInfo::makeSpectrumNumberTranslator(
    std::vector<SpectrumNumber> &&spectrumNumbers) const {
  PartitionIndex partition;
  int numberOfPartitions;
  if (m_storageMode == Parallel::StorageMode::Distributed) {
    partition = PartitionIndex(m_communicator->rank());
    numberOfPartitions = m_communicator->size();
  } else if (m_storageMode == Parallel::StorageMode::Cloned) {
    partition = PartitionIndex(0);
    numberOfPartitions = 1;
  } else if (m_storageMode == Parallel::StorageMode::MasterOnly) {
    if (m_communicator->rank() != 0)
      throw std::runtime_error(
          "IndexInfo: storage mode is " + Parallel::toString(m_storageMode) +
          " but creation on non-master rank has been attempted");
    partition = PartitionIndex(0);
    numberOfPartitions = 1;
  } else {
    throw std::runtime_error("IndexInfo: unknown storage mode " +
                             Parallel::toString(m_storageMode));
  }
  auto partitioner = Kernel::make_unique<RoundRobinPartitioner>(
      numberOfPartitions, partition,
      Partitioner::MonitorStrategy::TreatAsNormalSpectrum);
  m_spectrumNumberTranslator = Kernel::make_cow<SpectrumNumberTranslator>(
      std::move(spectrumNumbers), *partitioner, partition);
}

template MANTID_INDEXING_DLL IndexInfo::IndexInfo(std::vector<SpectrumNumber>,
                                                  const IndexInfo &);
template MANTID_INDEXING_DLL
IndexInfo::IndexInfo(std::vector<GlobalSpectrumIndex>, const IndexInfo &);

} // namespace Indexing
} // namespace Mantid
