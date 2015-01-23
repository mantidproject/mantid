#include "MantidAPI/SpectrumDetectorMapping.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {
namespace API {
/** Constructor that fills the map from the spectrum-detector relationships in
 * the given workspace.
 *  @throws std::invalid_argument if a null workspace pointer is passed in
 */
SpectrumDetectorMapping::SpectrumDetectorMapping(
    const MatrixWorkspace *const workspace) {
  if (!workspace) {
    throw std::invalid_argument(
        "SpectrumDetectorMapping: Null workspace pointer passed");
  }

  for (size_t i = 0; i < workspace->getNumberHistograms(); ++i) {
    auto spectrum = workspace->getSpectrum(i);
    m_mapping[spectrum->getSpectrumNo()] = spectrum->getDetectorIDs();
  }
}

/** Constructor that fills the map from a pair of vectors, ignoring the IDs in
 * the optional ignore list
 *  @throws std::invalid_argument if the spectrumNumbers & detectorIDs vectors
 * are not of equal length
 */
SpectrumDetectorMapping::SpectrumDetectorMapping(
    const std::vector<specid_t> &spectrumNumbers,
    const std::vector<detid_t> &detectorIDs,
    const std::vector<detid_t> &ignoreDetIDs) {
  if (spectrumNumbers.size() != detectorIDs.size()) {
    throw std::invalid_argument("SpectrumDetectorMapping: Different length "
                                "spectrum number & detector ID array passed");
  }

  fillMapFromVector(spectrumNumbers, detectorIDs, ignoreDetIDs);
}

/** Constructor that fills the map from a pair c-style arrays.
 *  Not safe! Prefer the vector constructor where possible!
 *  @throws std::invalid_argument if a null array pointer is passed in
 */
SpectrumDetectorMapping::SpectrumDetectorMapping(
    const specid_t *const spectrumNumbers, const detid_t *const detectorIDs,
    size_t arrayLengths) {
  if (spectrumNumbers == NULL || detectorIDs == NULL) {
    throw std::invalid_argument(
        "SpectrumDetectorMapping: Null array pointer passed");
  }

  fillMapFromArray(spectrumNumbers, detectorIDs, arrayLengths);
}

/// Called by the c-array constructors to do the actual filling
void
SpectrumDetectorMapping::fillMapFromArray(const specid_t *const spectrumNumbers,
                                          const detid_t *const detectorIDs,
                                          const size_t arrayLengths) {
  for (size_t i = 0; i < arrayLengths; ++i) {
    m_mapping[spectrumNumbers[i]].insert(detectorIDs[i]);
  }
}

/// Called by the vector constructors to do the actual filling
void SpectrumDetectorMapping::fillMapFromVector(
    const std::vector<specid_t> &spectrumNumbers,
    const std::vector<detid_t> &detectorIDs,
    const std::vector<detid_t> &ignoreDetIDs) {
  std::set<detid_t> ignoreIDs(ignoreDetIDs.begin(), ignoreDetIDs.end());
  const size_t nspec(spectrumNumbers.size());
  for (size_t i = 0; i < nspec; ++i) {
    auto id = detectorIDs[i];
    if (ignoreIDs.count(id) == 0)
      m_mapping[spectrumNumbers[i]].insert(id);
  }
}
/// Default constructor;
SpectrumDetectorMapping::SpectrumDetectorMapping() {}

/// Destructor
SpectrumDetectorMapping::~SpectrumDetectorMapping() {}

/// @returns An ordered set of the unique spectrum numbers
std::set<specid_t> SpectrumDetectorMapping::getSpectrumNumbers() const {
  std::set<specid_t> specs;
  auto itend = m_mapping.end();
  for (auto it = m_mapping.begin(); it != itend; ++it) {
    specs.insert(it->first);
  }
  return specs;
}

const std::set<detid_t> &SpectrumDetectorMapping::getDetectorIDsForSpectrumNo(
    const specid_t spectrumNo) const {
  return m_mapping.at(spectrumNo);
}

const SpectrumDetectorMapping::sdmap &
SpectrumDetectorMapping::getMapping() const {
  return m_mapping;
}

} // namespace API
} // namespace Mantid
