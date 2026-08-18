// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/V3D.h"
#include <memory>

namespace Mantid {
namespace Geometry {

class IObject;
class CSGObject;

/// Holds the information used for doing numerical integrations of object in the
/// beam
struct MANTID_GEOMETRY_DLL Raster {
public:
  void reserve(size_t numVolumeElements);

  std::vector<double> l1;            ///< Cached L1 distances
  std::vector<double> volume;        ///< Cached element volumes
  std::vector<Kernel::V3D> position; ///< Cached element positions
  double totalvolume;                ///< Volume of the object
};

namespace Rasterize {

MANTID_GEOMETRY_DLL Raster calculate(const Kernel::V3D &beamDirection, const IObject &integShape,
                                     const IObject &sampleShape, const double cubeSizeInMetre);

/// Rasterise integShape in the lab frame while tracing through sampleShape in its own frame.
///
/// calculate() assumes both shapes share one frame. That does not hold when the integration
/// volume is authored in the lab frame - as a gauge volume is - and the sample shape carries a
/// goniometer rotation: the sample's own frame is related to the lab frame by `rotation`, so
/// testing a lab-frame point directly against the sample silently uses the wrong geometry.
///
/// Here integShape is diced in the lab frame and each candidate voxel is mapped back through
/// rotation.Invert() before being tested against, and traced through, sampleShape. Dicing the
/// integration shape in its own frame rather than rotating it into the sample's keeps its
/// axis-aligned bounding box tight; rotating the shape would inflate the box and admit voxels
/// lying outside the actual integration volume.
///
/// The returned positions are in the LAB frame; l1 is the path length through sampleShape.
MANTID_GEOMETRY_DLL Raster calculateInLabFrame(const Kernel::V3D &beamDirection, const IObject &integShape,
                                               const IObject &sampleShape, const double cubeSizeInMetre,
                                               const Kernel::Matrix<double> &rotation);

MANTID_GEOMETRY_DLL Raster calculateCylinder(const Kernel::V3D &beamDirection, const IObject &integShape,
                                             const IObject &sampleShape, const size_t numSlices,
                                             const size_t numAnnuli);

MANTID_GEOMETRY_DLL Raster calculateHollowCylinder(const Kernel::V3D &beamDirection, const IObject &integShape,
                                                   const IObject &sampleShape, const size_t numSlices,
                                                   const size_t numAnnuli);

} // namespace Rasterize
} // namespace Geometry
} // namespace Mantid
