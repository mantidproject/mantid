#include "MantidIndexing/IndexTranslator.h"

#include <algorithm>
#include <functional>

namespace Mantid {
namespace Indexing {

IndexTranslator::IndexTranslator(SpectrumNumbers &&spectrumNumbers,
                                 DetectorIDs &&detectorIDs)
    : m_spectrumNumbers(std::move(spectrumNumbers)),
      m_detectorIDs(std::move(detectorIDs)) {
  if (m_detectorIDs.size() != 0)
    if (m_spectrumNumbers.size() != m_detectorIDs.size())
      throw std::runtime_error(
          "IndexTranslator: Size mismatch between spectrum "
          "number vector and detector ID vector");
}

size_t IndexTranslator::size() const { return m_spectrumNumbers.size(); }

const std::vector<int32_t> &IndexTranslator::spectrumNumbers() const {
  return m_spectrumNumbers.data();
}
const std::vector<std::vector<int32_t>> &IndexTranslator::detectorIDs() const {
  return m_detectorIDs.data();
}

} // namespace Indexing
} // namespace Mantid
