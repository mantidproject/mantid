#include "MantidAPI/NearestNeighbourInfo.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumDetectorMapping.h"

namespace Mantid {
namespace API {

/** Creates NearestNeighbourInfo.
*
* @param workspace :: Reference to workspace providing instrument and
* spectrum-detector mapping
* @param nNeighbours :: unsigned int, number of neighbours to include.
* @param ignoreMaskedDetectors :: flag indicating that masked detectors should
*/
NearestNeighbourInfo::NearestNeighbourInfo(const MatrixWorkspace &workspace,
                                           const bool ignoreMaskedDetectors,
                                           const int nNeighbours)
    : m_workspace(workspace),
      m_nearestNeighbours(nNeighbours, workspace.getInstrument(),
                          SpectrumDetectorMapping(&workspace).getMapping(),
                          ignoreMaskedDetectors) {}

/** Queries the NearestNeighbours object for the selected detector.
* NOTE! getNeighbours(spectrumNumber, radius) is MUCH faster.
*
* @param comp :: pointer to the querying detector
* @param radius :: distance from detector on which to filter results
*be ignored. True to ignore detectors.
* @return map of DetectorID to distance for the nearest neighbours
*/
std::map<specnum_t, Kernel::V3D>
NearestNeighbourInfo::getNeighbours(const Geometry::IDetector *comp,
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
  return m_nearestNeighbours.neighboursInRadius(spectra[0], radius);
}

/** Queries the NearestNeighbours object for the selected spectrum number.
*
* @param spec :: spectrum number of the detector you are looking at
* @param radius :: distance from detector on which to filter results
*be ignored. True to ignore detectors.
* @return map of DetectorID to distance for the nearest neighbours
*/
std::map<specnum_t, Kernel::V3D>
NearestNeighbourInfo::getNeighbours(specnum_t spec, const double radius) const {
  return m_nearestNeighbours.neighboursInRadius(spec, radius);
}

/** Queries the NearestNeighbours object for the selected spectrum number.
*
* @param spec :: spectrum number of the detector you are looking at
*be ignored. True to ignore detectors.
* @return map of DetectorID to distance for the nearest neighbours
*/
std::map<specnum_t, Kernel::V3D>
NearestNeighbourInfo::getNeighboursExact(specnum_t spec) const {
  return m_nearestNeighbours.neighbours(spec);
}

} // namespace API
} // namespace Mantid
