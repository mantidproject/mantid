// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAlgorithms/DllConfig.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidGeometry/Objects/IObject.h"

namespace Mantid {

namespace Geometry {
class IDetector;
class IObject;
} // namespace Geometry

namespace Algorithms {

/** MultipleScatteringCorrectionDistGraber :
 * This is a helper class to calculate the distance from source to sample voxel.
 * TODO:
 * - Add support for containers (see PaalmanPingAbsorption for example)
 */
class MANTID_ALGORITHMS_DLL MultipleScatteringCorrectionDistGraber {
public:
  MultipleScatteringCorrectionDistGraber(const Geometry::IObject &sampleShape, const double elementSize);
  ~MultipleScatteringCorrectionDistGraber() = default;
  // Pre-calculate (cache) all the distances from source to each individual voxel
  // inside the sample.
  // This function is borrowed from AnyShapeAbsorption.initialiseCachedDistances()
  void cacheLS1(const Mantid::Kernel::V3D &beamDirection);

  // cached LS1 related data for multiple scattering correction
  std::vector<double> m_LS1;                   ///< Cached L1 distances
  std::vector<double> m_elementVolumes;        ///< Cached element volumes
  std::vector<Kernel::V3D> m_elementPositions; ///< Cached element positions
  size_t m_numVolumeElements;                  ///< The number of volume elements
  double m_totalVolume;                        ///< The total volume of the sample

private:
  const Geometry::IObject *m_sampleShape; ///< The shape pointer is handeled by sample object, not distGraber
  const double m_elementSize;             ///< Size of the sample in m.
};

} // namespace Algorithms
} // namespace Mantid
