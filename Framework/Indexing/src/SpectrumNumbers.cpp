#include "MantidIndexing/SpectrumNumbers.h"

#include <algorithm>
#include <functional>

namespace Mantid {
namespace Indexing {

SpectrumNumbers::SpectrumNumbers(std::vector<specnum_t> &&spectrumNumbers) {
  if (!std::is_sorted(spectrumNumbers.begin(), spectrumNumbers.end(),
                      std::less_equal<specnum_t>()))
    throw std::runtime_error(
        "SpectrumNumbers: Spectrum numbers are not strictly ascending");

  m_data = std::move(spectrumNumbers);
}

SpectrumNumbers::SpectrumNumbers(std::initializer_list<specnum_t> &&ilist)
    : SpectrumNumbers(std::vector<specnum_t>(std::move(ilist))) {}

size_t SpectrumNumbers::size() const { return m_data.size(); }

const std::vector<specnum_t> &SpectrumNumbers::data() const { return m_data; }

} // namespace Indexing
} // namespace Mantid
