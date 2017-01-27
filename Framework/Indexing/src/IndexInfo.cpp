#include "MantidIndexing/IndexInfo.h"
#include "MantidIndexing/SpectrumNumberTranslator.h"
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
    : m_size(globalSize),
      m_spectrumNumbers(
          Kernel::make_cow<std::vector<SpectrumNumber>>(globalSize)),
      m_detectorIDs(
          Kernel::make_cow<std::vector<std::vector<DetectorID>>>(globalSize)) {
  // Default to spectrum numbers 1...globalSize
  auto &specNums = m_spectrumNumbers.access();
  std::iota(specNums.begin(), specNums.end(), 1);
}

/// Construct with given spectrum number and vector of detector IDs for each
/// index.
IndexInfo::IndexInfo(std::vector<SpectrumNumber> &&spectrumNumbers,
                     std::vector<std::vector<DetectorID>> &&detectorIDs)
    : m_size(spectrumNumbers.size()) {
  if (spectrumNumbers.size() != detectorIDs.size())
    throw std::runtime_error("IndexInfo: Size mismatch between spectrum number "
                             "and detector ID vectors");
  m_spectrumNumbers.access() = std::move(spectrumNumbers);
  m_detectorIDs.access() = std::move(detectorIDs);
}

// Defined as default in source for forward declaration with std::unique_ptr.
IndexInfo::~IndexInfo() = default;

/// The *local* size, i.e., the number of spectra in this partition.
size_t IndexInfo::size() const { return m_size; }

/// Returns the spectrum number for given index.
SpectrumNumber IndexInfo::spectrumNumber(const size_t index) const {
  return (*m_spectrumNumbers)[index];
}

/// Return a vector of the detector IDs for given index.
const std::vector<DetectorID> &
IndexInfo::detectorIDs(const size_t index) const {
  return (*m_detectorIDs)[index];
}

/// Set a spectrum number for each index.
void IndexInfo::setSpectrumNumbers(
    std::vector<SpectrumNumber> &&spectrumNumbers) & {
  if (size() != spectrumNumbers.size())
    throw std::runtime_error(
        "IndexInfo: Size mismatch when setting new spectrum numbers");
  m_spectrumNumbers.access() = std::move(spectrumNumbers);
}

void IndexInfo::setSpectrumNumbers(const SpectrumNumber min,
                                   const SpectrumNumber max) & {
  if (static_cast<int64_t>(size()) !=
      static_cast<int32_t>(max) - static_cast<int32_t>(min) + 1)
    throw std::runtime_error(
        "IndexInfo: Size mismatch when setting new spectrum numbers");
  auto &data = m_spectrumNumbers.access();
  std::iota(data.begin(), data.end(), static_cast<int32_t>(min));
}

/// Set a single detector ID for each index.
void IndexInfo::setDetectorIDs(const std::vector<DetectorID> &detectorIDs) & {
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
    std::vector<std::vector<DetectorID>> &&detectorIDs) & {
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
