// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/MultipleScatteringCorrectionDistGraber.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidGeometry/Rasterize.h"

// --------------------------
//   DECLARATIONS
// --------------------------
// This class is a helper class for MultipleScatteringCorrection.
// Its main purpose is to calculate and cache the distance for
// - L_s1: source to first voxel in sample
// - L_2d: second voxel in sample to detector

namespace Mantid {
namespace Algorithms {

using namespace Geometry;
using namespace Kernel;

/**
 * @brief Construct a new Multiple Scattering Correction Dist Graber
 *
 * @param sampleBbject
 */
MultipleScatteringCorrectionDistGraber::MultipleScatteringCorrectionDistGraber(const Geometry::IObject &sampleOBbject,
                                                                               const double elementSize)
    : m_sampleObject(&sampleOBbject), m_elementSize(elementSize * 1e-3) {}

/**
 * @brief pre-calculate all distances from source to each voxels inside the sample
 *
 * @note this function is borrowed from AnyShapeAbsorption.initialiseCachedDistances()
 *
 */
void MultipleScatteringCorrectionDistGraber::cacheLS1(const V3D &beamDirection) {
  auto integrationVolume = std::shared_ptr<const IObject>(m_sampleObject->clone());

  // create the raster
  auto raster = Geometry::Rasterize::calculate(beamDirection, *integrationVolume, m_elementSize);

  // check if the raster is empty
  if (raster.l1.size() == 0) {
    throw std::runtime_error("Failed to rasterize sample shape.");
  }

  // compute and save
  m_numVolumeElements = raster.l1.size();
  m_LS1 = std::move(raster.l1);
  m_elementPositions = std::move(raster.position);
  m_elementVolumes = std::move(raster.volume);
}
} // namespace Algorithms
} // namespace Mantid
