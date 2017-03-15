#include "MantidIndexing/IndexInfo.h"
#include "MantidIndexing/RoundRobinPartitioner.h"
#include "MantidIndexing/SpectrumNumberTranslator.h"
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
IndexInfo::IndexInfo(const size_t globalSize) {
  // Default to spectrum numbers 1...globalSize
  std::vector<SpectrumNumber> specNums(globalSize);
  std::iota(specNums.begin(), specNums.end(), 1);
  makeSpectrumNumberTranslator(std::move(specNums));
  m_detectorIDs = Kernel::make_cow<std::vector<std::vector<DetectorID>>>(
      m_spectrumNumberTranslator->localSize());
}

/// Construct with given spectrum number and vector of detector IDs for each
/// index.
IndexInfo::IndexInfo(std::vector<SpectrumNumber> &&spectrumNumbers,
                     std::vector<std::vector<DetectorID>> &&detectorIDs) {
  if (spectrumNumbers.size() != detectorIDs.size())
    throw std::runtime_error("IndexInfo: Size mismatch between spectrum number "
                             "and detector ID vectors");
  m_detectorIDs.access() = std::move(detectorIDs);
  makeSpectrumNumberTranslator(std::move(spectrumNumbers));
}

/// The *local* size, i.e., the number of spectra in this partition.
size_t IndexInfo::size() const {
  if (!m_detectorIDs)
    return 0;
  return m_detectorIDs->size();
}

/// Returns the spectrum number for given index.
SpectrumNumber IndexInfo::spectrumNumber(const size_t index) const {
  return m_spectrumNumberTranslator->spectrumNumber(index);
}

/// Return a vector of the detector IDs for given index.
const std::vector<DetectorID> &
IndexInfo::detectorIDs(const size_t index) const {
  return (*m_detectorIDs)[index];
}

/// Set a spectrum number for each index.
void IndexInfo::setSpectrumNumbers(
    std::vector<SpectrumNumber> &&spectrumNumbers) {
  if (m_spectrumNumberTranslator->globalSize() != spectrumNumbers.size())
    throw std::runtime_error(
        "IndexInfo: Size mismatch when setting new spectrum numbers");
  makeSpectrumNumberTranslator(std::move(spectrumNumbers));
}

/// Set a contiguous range of spectrum numbers.
void IndexInfo::setSpectrumNumbers(const SpectrumNumber min,
                                   const SpectrumNumber max) {
  auto newSize = static_cast<int32_t>(max) - static_cast<int32_t>(min) + 1;
  if (static_cast<int64_t>(m_spectrumNumberTranslator->globalSize()) != newSize)
    throw std::runtime_error(
        "IndexInfo: Size mismatch when setting new spectrum numbers");
  std::vector<SpectrumNumber> specNums(newSize);
  std::iota(specNums.begin(), specNums.end(), static_cast<int32_t>(min));
  makeSpectrumNumberTranslator(std::move(specNums));
}

/// Set a single detector ID for each index.
void IndexInfo::setDetectorIDs(const std::vector<DetectorID> &detectorIDs) {
  if (size() != detectorIDs.size())
    throw std::runtime_error(
        "IndexInfo: Size mismatch when setting new detector IDs");

  auto &detIDs = m_detectorIDs.access();
  for (size_t i = 0; i < detectorIDs.size(); ++i)
    detIDs[i] = {detectorIDs[i]};
  // Setting new detector ID grouping makes definitions outdated.
  m_spectrumDefinitions =
      Kernel::cow_ptr<std::vector<SpectrumDefinition>>(nullptr);
}

/// Set a vector of detector IDs for each index.
void IndexInfo::setDetectorIDs(
    std::vector<std::vector<DetectorID>> &&detectorIDs) {
  if (size() != detectorIDs.size())
    throw std::runtime_error(
        "IndexInfo: Size mismatch when setting new detector IDs");

  auto &detIDs = m_detectorIDs.access();
  detIDs = std::move(detectorIDs);
  for (auto &ids : detIDs) {
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
  }
  // Setting new detector ID grouping makes definitions outdated.
  m_spectrumDefinitions =
      Kernel::cow_ptr<std::vector<SpectrumDefinition>>(nullptr);
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

void IndexInfo::makeSpectrumNumberTranslator(
    std::vector<SpectrumNumber> &&spectrumNumbers) const {
  // TODO We are not setting monitors currently. This is ok as long as we have
  // exactly one partition.
  PartitionIndex partition = 0;
  auto partitioner = Kernel::make_unique<RoundRobinPartitioner>(
      1, partition, Partitioner::MonitorStrategy::CloneOnEachPartition,
      std::vector<GlobalSpectrumIndex>{});
  m_spectrumNumberTranslator = Kernel::make_cow<SpectrumNumberTranslator>(
      std::move(spectrumNumbers), std::move(partitioner), partition);
}

} // namespace Indexing
} // namespace Mantid
