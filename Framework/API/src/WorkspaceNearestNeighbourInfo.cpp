// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/WorkspaceNearestNeighbourInfo.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceNearestNeighbours.h"
#include "MantidGeometry/IDetector.h"


namespace Mantid {
namespace API {

/** Creates WorkspaceNearestNeighbourInfo.
 *
 * @param workspace :: Reference to workspace providing instrument and
 * spectrum-detector mapping
 * @param ignoreMaskedDetectors :: if true, masked detectors are ignored
 * @param nNeighbours :: number of neighbours to include
 */
WorkspaceNearestNeighbourInfo::WorkspaceNearestNeighbourInfo(
    const MatrixWorkspace &workspace, const bool ignoreMaskedDetectors,
    const int nNeighbours)
    : m_workspace(workspace) {
  std::vector<specnum_t> spectrumNumbers;
  const auto nhist = m_workspace.getNumberHistograms();
  spectrumNumbers.reserve(nhist);
  for (size_t i = 0; i < nhist; ++i)
    spectrumNumbers.emplace_back(m_workspace.getSpectrum(i).getSpectrumNo());

  m_nearestNeighbours = std::make_unique<WorkspaceNearestNeighbours>(
      nNeighbours, workspace.spectrumInfo(), std::move(spectrumNumbers),
      ignoreMaskedDetectors);
}

// Defined as default in source for forward declaration with std::unique_ptr.
WorkspaceNearestNeighbourInfo::~WorkspaceNearestNeighbourInfo() = default;

/** Queries the WorkspaceNearestNeighbours object for the selected detector.
 * NOTE! getNeighbours(spectrumNumber, radius) is MUCH faster.
 *
 * @param comp :: pointer to the querying detector
 * @param radius :: distance from detector on which to filter results
 * @return map of DetectorID to distance for the nearest neighbours
 */
std::map<specnum_t, Kernel::V3D>
WorkspaceNearestNeighbourInfo::getNeighbours(const Geometry::IDetector *comp,
                                             const double radius) const {
  // Find the spectrum number
  std::vector<specnum_t> spectra = m_workspace.getSpectraFromDetectorIDs(
      std::vector<detid_t>(1, comp->getID()));
  if (spectra.empty()) {
    throw Kernel::Exception::NotFoundError("MatrixWorkspace::getNeighbours - "
                                           "Cannot find spectrum number for "
                                           "detector",
                                           comp->getID());
  }
  return m_nearestNeighbours->neighboursInRadius(spectra[0], radius);
}

/** Queries the WorkspaceNearestNeighbours object for the selected spectrum
 * number.
 *
 * @param spec :: spectrum number of the detector you are looking at
 * @param radius :: distance from detector on which to filter results
 * @return map of DetectorID to distance for the nearest neighbours
 */
std::map<specnum_t, Kernel::V3D>
WorkspaceNearestNeighbourInfo::getNeighbours(specnum_t spec,
                                             const double radius) const {
  return m_nearestNeighbours->neighboursInRadius(spec, radius);
}

/** Queries the WorkspaceNearestNeighbours object for the selected spectrum
 * number.
 *
 * @param spec :: spectrum number of the detector you are looking at
 * @return map of DetectorID to distance for the nearest neighbours
 */
std::map<specnum_t, Kernel::V3D>
WorkspaceNearestNeighbourInfo::getNeighboursExact(specnum_t spec) const {
  return m_nearestNeighbours->neighbours(spec);
}

} // namespace API
} // namespace Mantid
