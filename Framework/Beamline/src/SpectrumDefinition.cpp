#include "MantidBeamline/SpectrumDefinition.h"

namespace Mantid {
namespace Beamline {

/// Returns the size of the SpectrumDefinition, i.e., the number of detectors
/// (or rather detector positions) that the spectrum comprises.
size_t SpectrumDefinition::size() const { return m_data.size(); }

/// Returns a const reference to the pair of detector index and time index at
/// the given `index` in the spectrum definition.
const std::pair<size_t, size_t> &SpectrumDefinition::
operator[](const size_t index) const {
  return m_data[index];
}

/// Adds a pair of detector index and time index to the spectrum definition. The
/// time index defaults to zero when not specified.
void SpectrumDefinition::add(const size_t detectorIndex,
                             const size_t timeIndex) {
  auto index = std::make_pair(detectorIndex, timeIndex);
  auto it = std::lower_bound(m_data.begin(), m_data.end(), index);
  if ((it == m_data.end()) || (*it != index))
    m_data.emplace(it, index);
}

} // namespace Beamline
} // namespace Mantid
