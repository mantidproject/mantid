// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidBeamline/SpectrumInfo.h"
#include "MantidKernel/make_cow.h"
#include "MantidTypes/SpectrumDefinition.h"

#include <numeric>

namespace Mantid::Beamline {

SpectrumInfo::SpectrumInfo(const size_t numberOfDetectors)
    : m_spectrumDefinition(Kernel::make_cow<std::vector<SpectrumDefinition>>(numberOfDetectors)) {}

SpectrumInfo::SpectrumInfo(Kernel::cow_ptr<std::vector<SpectrumDefinition>> spectrumDefinition)
    : m_spectrumDefinition(std::move(spectrumDefinition)) {}

/// Returns the size of the SpectrumInfo, i.e., the number of spectra.
size_t SpectrumInfo::size() const {
  if (!m_spectrumDefinition)
    return 0;
  return m_spectrumDefinition->size();
}

/// Return count of all detectors used within spectrum
size_t SpectrumInfo::detectorCount() const {
  return std::accumulate(m_spectrumDefinition->begin(), m_spectrumDefinition->end(), size_t(0),
                         [](size_t x, const Mantid::SpectrumDefinition &spec) { return x + spec.size(); });
}

/// Returns a const reference to the SpectrumDefinition of the spectrum.
const SpectrumDefinition &SpectrumInfo::spectrumDefinition(const size_t index) const {
  return (*m_spectrumDefinition)[index];
}

/// Sets the SpectrumDefinition of the spectrum.
void SpectrumInfo::setSpectrumDefinition(const size_t index, SpectrumDefinition def) {
  m_spectrumDefinition.access()[index] = std::move(def);
}

const Kernel::cow_ptr<std::vector<SpectrumDefinition>> &SpectrumInfo::sharedSpectrumDefinitions() const {
  return m_spectrumDefinition;
}

} // namespace Mantid::Beamline
