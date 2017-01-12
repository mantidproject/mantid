#include "MantidBeamline/SpectrumDefinition.h"

namespace Mantid {
namespace Beamline {

/// Returns the size of the SpectrumDefinition, i.e., the number of detectors
/// (or rather detector positions) that the spectrum comprises.
size_t SpectrumDefinition::size() const { return m_data.size(); }

const std::pair<size_t, size_t> &SpectrumDefinition::
operator[](const size_t index) const {
  return m_data[index];
}

void SpectrumDefinition::add(const size_t detectorIndex,
                             const size_t timeIndex) {
  auto index = std::make_pair(detectorIndex, timeIndex);
  auto it = std::lower_bound(m_data.begin(), m_data.end(), index);
  if ((it == m_data.end()) || (*it != index))
    m_data.emplace(it, index);
}

} // namespace Beamline
} // namespace Mantid
