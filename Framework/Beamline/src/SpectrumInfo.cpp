#include "MantidBeamline/SpectrumDefinition.h"
#include "MantidBeamline/SpectrumInfo.h"
#include "MantidKernel/make_cow.h"

namespace Mantid {
namespace Beamline {

SpectrumInfo::SpectrumInfo(const size_t numberOfDetectors)
    : m_spectrumDefinition(Kernel::make_cow<std::vector<SpectrumDefinition>>(
          numberOfDetectors)) {}

/// Returns the size of the SpectrumInfo, i.e., the number of spectra.
size_t SpectrumInfo::size() const {
  if (!m_spectrumDefinition)
    return 0;
  return m_spectrumDefinition->size();
}

/// Returns a const reference to the SpectrumDefinition of the spectrum.
const SpectrumDefinition &
SpectrumInfo::spectrumDefinition(const size_t index) const {
  return (*m_spectrumDefinition)[index];
}

/** Returns a non-const reference to the SpectrumDefinition of the spectrum.
*
* This method might be removed in the future since it breaks encapsulation and
* should thus be used sparingly, in cases where direct modification is necessary
* for adequate performance. */
SpectrumDefinition &
SpectrumInfo::mutableSpectrumDefinition(const size_t index) {
  return m_spectrumDefinition.access()[index];
}

/// Sets the SpectrumDefinition of the spectrum.
void SpectrumInfo::setSpectrumDefinition(const size_t index,
                                         SpectrumDefinition def) {
  m_spectrumDefinition.access()[index] = std::move(def);
}

} // namespace Beamline
} // namespace Mantid
