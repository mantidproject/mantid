#include "MantidIndexing/IndexInfo.h"
#include "MantidIndexing/RoundRobinPartitioner.h"
#include "MantidIndexing/SpectrumNumberTranslator.h"
#include "MantidParallel/Communicator.h"
#include "MantidKernel/make_cow.h"
#include "MantidKernel/make_unique.h"
#include "MantidTypes/SpectrumDefinition.h"

#include <algorithm>
#include <functional>
#include <numeric>

namespace Mantid {
namespace Indexing {

/// Construct a default IndexInfo, with contiguous spectrum numbers starting at
/// 1 and no detector IDs.
IndexInfo::IndexInfo(const size_t globalSize,
                     const Parallel::StorageMode storageMode)
    : IndexInfo(globalSize, storageMode, Parallel::Communicator{}) {}

/// Construct a default IndexInfo, with contiguous spectrum numbers starting at
/// 1 and no detector IDs.
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

/// Construct with given spectrum number and vector of detector IDs for each
/// index.
IndexInfo::IndexInfo(std::vector<SpectrumNumber> spectrumNumbers,
                     const Parallel::StorageMode storageMode)
    : IndexInfo(std::move(spectrumNumbers), storageMode,
                Parallel::Communicator{}) {}

/// Construct with given spectrum number and vector of detector IDs for each
/// index.
IndexInfo::IndexInfo(std::vector<SpectrumNumber> spectrumNumbers,
                     const Parallel::StorageMode storageMode,
                     const Parallel::Communicator &communicator)
    : m_storageMode(storageMode),
      m_communicator(
          Kernel::make_unique<Parallel::Communicator>(communicator)) {
  makeSpectrumNumberTranslator(std::move(spectrumNumbers));
}

IndexInfo::IndexInfo(const IndexInfo &other)
    : m_storageMode(other.m_storageMode),
      m_communicator(
          Kernel::make_unique<Parallel::Communicator>(*other.m_communicator)),
      m_spectrumDefinitions(other.m_spectrumDefinitions),
      m_spectrumNumberTranslator(other.m_spectrumNumberTranslator) {}

IndexInfo::IndexInfo(IndexInfo &&) = default;

// Defined as default in source for forward declaration with std::unique_ptr.
IndexInfo::~IndexInfo() = default;

IndexInfo &IndexInfo::operator=(const IndexInfo &other) {
  auto copy(other);
  return *this = std::move(copy);
}

IndexInfo &IndexInfo::operator=(IndexInfo &&) = default;

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

/// Returns the spectrum number for given index.
SpectrumNumber IndexInfo::spectrumNumber(const size_t index) const {
  return m_spectrumNumberTranslator->spectrumNumber(index);
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
Parallel::Communicator IndexInfo::communicator() const {
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
      std::move(spectrumNumbers), std::move(partitioner), partition);
}

} // namespace Indexing
} // namespace Mantid
