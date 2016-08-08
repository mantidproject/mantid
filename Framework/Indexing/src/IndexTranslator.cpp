#include "MantidIndexing/IndexTranslator.h"
#include "MantidKernel/make_cow.h"

#include <algorithm>
#include <functional>
#include <numeric>

namespace Mantid {
namespace Indexing {

IndexTranslator::IndexTranslator(const size_t globalSize) {
  // Default to spectrum numbers 1...globalSize
  auto &specNums = m_spectrumNumbers.access();
  specNums.resize(globalSize);
  std::iota(specNums.begin(), specNums.end(), 1);

  // Default to detector IDs 1..globalSize, with 1:1 mapping to spectra
  auto &detIDs = m_detectorIDs.access();
  for (size_t i = 0; i < globalSize; ++i)
    detIDs.emplace_back(1, static_cast<detid_t>(i));
}

void IndexTranslator::setSpectrumNumbers(
    std::vector<specnum_t> &&spectrumNumbers) & {
  if (m_spectrumNumbers->size() != spectrumNumbers.size())
    throw std::runtime_error(
        "IndexTranslator: Size mismatch when setting new spectrum numbers");
  m_spectrumNumbers.access() = std::move(spectrumNumbers);
}

void IndexTranslator::setDetectorIDs(const std::vector<detid_t> &detectorIDs) &
{
  if (m_detectorIDs->size() != detectorIDs.size())
    throw std::runtime_error(
        "IndexTranslator: Size mismatch when setting new detector IDs");

  auto &detIDs = m_detectorIDs.access();
  for (size_t i = 0; i < detectorIDs.size(); ++i)
    detIDs[i] = {detectorIDs[i]};
}

void IndexTranslator::setDetectorIDs(
    std::vector<std::vector<detid_t>> &&detectorIDs) & {
  if (m_detectorIDs->size() != detectorIDs.size())
    throw std::runtime_error(
        "IndexTranslator: Size mismatch when setting new detector IDs");

  auto &detIDs = m_detectorIDs.access();
  detIDs = std::move(detectorIDs);
  for (auto &ids : detIDs) {
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
  }
}

size_t IndexTranslator::size() const { return m_spectrumNumbers->size(); }

} // namespace Indexing
} // namespace Mantid
