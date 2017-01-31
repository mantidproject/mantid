#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/make_cow.h"
#include "MantidTypes/SpectrumDefinition.h"

#include <algorithm>
#include <functional>
#include <numeric>

namespace Mantid {
namespace Indexing {

/// Construct a default IndexInfo, with contiguous spectrum numbers starting at
/// 1 and no detector IDs.
IndexInfo::IndexInfo(const size_t globalSize)
    : m_spectrumNumbers(Kernel::make_cow<std::vector<specnum_t>>(globalSize)),
      m_detectorIDs(
          Kernel::make_cow<std::vector<std::vector<detid_t>>>(globalSize)) {
  // Default to spectrum numbers 1...globalSize
  auto &specNums = m_spectrumNumbers.access();
  std::iota(specNums.begin(), specNums.end(), 1);
}

/// Construct with given spectrum number and vector of detector IDs for each
/// index.
IndexInfo::IndexInfo(std::vector<specnum_t> &&spectrumNumbers,
                     std::vector<std::vector<detid_t>> &&detectorIDs) {
  if (spectrumNumbers.size() != detectorIDs.size())
    throw std::runtime_error("IndexInfo: Size mismatch between spectrum number "
                             "and detector ID vectors");
  m_spectrumNumbers.access() = std::move(spectrumNumbers);
  m_detectorIDs.access() = std::move(detectorIDs);
}

/// Constructor for internal use by MatrixWorkspace (legacy mode).
IndexInfo::IndexInfo(
    std::function<size_t()> getSize,
    std::function<specnum_t(const size_t)> getSpectrumNumber,
    std::function<const std::set<specnum_t> &(const size_t)> getDetectorIDs)
    : m_isLegacy{true}, m_getSize(getSize),
      m_getSpectrumNumber(getSpectrumNumber), m_getDetectorIDs(getDetectorIDs) {
}

/// Copy constructor.
IndexInfo::IndexInfo(const IndexInfo &other) {
  if (other.m_isLegacy) {
    // Workaround while IndexInfo is not holding index data stored in
    // MatrixWorkspace: build IndexInfo based on data in ISpectrum.
    auto size = other.m_getSize();
    auto &specNums = m_spectrumNumbers.access();
    auto &detIDs = m_detectorIDs.access();
    for (size_t i = 0; i < size; ++i) {
      specNums.push_back(other.m_getSpectrumNumber(i));
      const auto &set = other.m_getDetectorIDs(i);
      detIDs.emplace_back(set.begin(), set.end());
    }
  } else {
    m_spectrumNumbers = other.m_spectrumNumbers;
    m_detectorIDs = other.m_detectorIDs;
  }
}

/// The *local* size, i.e., the number of spectra in this partition.
size_t IndexInfo::size() const {
  if (m_isLegacy)
    return m_getSize();
  return m_spectrumNumbers->size();
}

/// Returns the spectrum number for given index.
specnum_t IndexInfo::spectrumNumber(const size_t index) const {
  if (m_isLegacy)
    return m_getSpectrumNumber(index);
  return (*m_spectrumNumbers)[index];
}

/// Return a vector of the detector IDs for given index.
std::vector<detid_t> IndexInfo::detectorIDs(const size_t index) const {
  if (m_isLegacy) {
    const auto &ids = m_getDetectorIDs(index);
    return std::vector<detid_t>(ids.begin(), ids.end());
  }
  return (*m_detectorIDs)[index];
}

/// Set a spectrum number for each index.
void IndexInfo::setSpectrumNumbers(std::vector<specnum_t> &&spectrumNumbers) & {
  // No test of m_isLegacy, we cannot have non-const access in that case.
  if (m_spectrumNumbers->size() != spectrumNumbers.size())
    throw std::runtime_error(
        "IndexInfo: Size mismatch when setting new spectrum numbers");
  m_spectrumNumbers.access() = std::move(spectrumNumbers);
}

/// Set a single detector ID for each index.
void IndexInfo::setDetectorIDs(const std::vector<detid_t> &detectorIDs) & {
  // No test of m_isLegacy, we cannot have non-const access in that case.
  if (m_detectorIDs->size() != detectorIDs.size())
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
    // No test of m_isLegacy, we cannot have non-const access in that case.
    std::vector<std::vector<detid_t>> &&detectorIDs) & {
  if (m_detectorIDs->size() != detectorIDs.size())
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

void IndexInfo::setSpectrumDefinitions(
    Kernel::cow_ptr<std::vector<SpectrumDefinition>> spectrumDefinitions) {
  if (!spectrumDefinitions || (size() != spectrumDefinitions->size()))
    throw std::runtime_error(
        "IndexInfo: Size mismatch when setting new spectrum definitions");
  m_spectrumDefinitions = spectrumDefinitions;
}

const Kernel::cow_ptr<std::vector<SpectrumDefinition>> &
IndexInfo::spectrumDefinitions() const {
  return m_spectrumDefinitions;
}

} // namespace Indexing
} // namespace Mantid
