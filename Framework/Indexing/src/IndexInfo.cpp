#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/make_cow.h"

#include <algorithm>
#include <functional>
#include <numeric>

namespace Mantid {
namespace Indexing {

IndexInfo::IndexInfo(const size_t globalSize) {
  // Default to spectrum numbers 1...globalSize
  auto &specNums = m_spectrumNumbers.access();
  specNums.resize(globalSize);
  std::iota(specNums.begin(), specNums.end(), 1);

  // Default to detector IDs 1..globalSize, with 1:1 mapping to spectra
  auto &detIDs = m_detectorIDs.access();
  for (size_t i = 0; i < globalSize; ++i)
    detIDs.emplace_back(1, static_cast<detid_t>(i));
}

IndexInfo::IndexInfo(std::vector<specnum_t> &&spectrumNumbers,
                     std::vector<std::vector<detid_t>> &&detectorIDs) {
  if (spectrumNumbers.size() != detectorIDs.size())
    throw std::runtime_error("IndexInfo: Size mismatch between spectrum number "
                             "and detector ID vectors");
  m_spectrumNumbers.access() = std::move(spectrumNumbers);
  m_detectorIDs.access() = std::move(detectorIDs);
}

IndexInfo::IndexInfo(
    const size_t globalSize,
    std::function<specnum_t(const size_t)> getSpectrumNumber,
    std::function<const std::set<specnum_t> &(const size_t)> getDetectorIDs)
    : m_isLegacy{true}, m_legacySize(globalSize),
      m_getSpectrumNumber(getSpectrumNumber), m_getDetectorIDs(getDetectorIDs) {
}

IndexInfo::IndexInfo(const IndexInfo &other) {
  if (m_isLegacy) {
    // Workaround while IndexInfo is not holding index data stored in
    // MatrixWorkspace: build IndexInfo based on data in ISpectrum.
    auto &specNums = m_spectrumNumbers.access();
    auto &detIDs = m_detectorIDs.access();
    for (size_t i = 0; i < other.m_legacySize; ++i) {
      specNums.push_back(other.m_getSpectrumNumber(i));
      const auto &set = other.m_getDetectorIDs(i);
      detIDs.emplace_back(set.begin(), set.end());
    }
  } else {
    m_spectrumNumbers = other.m_spectrumNumbers;
    m_detectorIDs = other.m_detectorIDs;
  }
}

void IndexInfo::setSpectrumNumbers(std::vector<specnum_t> &&spectrumNumbers) & {
  if (m_spectrumNumbers->size() != spectrumNumbers.size())
    throw std::runtime_error(
        "IndexInfo: Size mismatch when setting new spectrum numbers");
  m_spectrumNumbers.access() = std::move(spectrumNumbers);
}

void IndexInfo::setDetectorIDs(const std::vector<detid_t> &detectorIDs) & {
  if (m_detectorIDs->size() != detectorIDs.size())
    throw std::runtime_error(
        "IndexInfo: Size mismatch when setting new detector IDs");

  auto &detIDs = m_detectorIDs.access();
  for (size_t i = 0; i < detectorIDs.size(); ++i)
    detIDs[i] = {detectorIDs[i]};
}

void IndexInfo::setDetectorIDs(
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
}

size_t IndexInfo::size() const { return m_spectrumNumbers->size(); }

} // namespace Indexing
} // namespace Mantid
