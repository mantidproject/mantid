#include "MantidIndexing/IndexTranslator.h"

#include <algorithm>
#include <functional>

namespace Mantid {
namespace Indexing {

IndexTranslator::IndexTranslator(
    std::vector<int32_t> &&spectrumNumbers,
    std::vector<std::vector<int32_t>> &&detectorIDs) {
  if (spectrumNumbers.size() != detectorIDs.size())
    throw std::runtime_error("IndexTranslator: Size mismatch between spectrum "
                             "number vector and detector ID vector");
  if (!std::is_sorted(spectrumNumbers.begin(), spectrumNumbers.end(),
                      std::less_equal<int32_t>()))
    throw std::runtime_error(
        "IndexTranslator: Spectrum numbers are not strictly ascending");

  m_spectrumNumbers = std::move(spectrumNumbers);
  m_detectorIDs = std::move(detectorIDs);
  for (auto &ids : m_detectorIDs) {
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
  }
}

size_t IndexTranslator::size() const { return m_spectrumNumbers.size(); }

const std::vector<int32_t> &IndexTranslator::spectrumNumbers() const {
  return m_spectrumNumbers;
}
const std::vector<std::vector<int32_t>> &IndexTranslator::detectorIDs() const {
  return m_detectorIDs;
}

} // namespace Indexing
} // namespace Mantid
