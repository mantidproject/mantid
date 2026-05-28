// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidBeamline/DllConfig.h"
#include <Eigen/Geometry>
#include <cstddef>

namespace Mantid {
namespace Beamline {

/**
 * Parameters for one contiguous block of virtual (analytically computed)
 * detector indices produced by a PixelAssembly bank.
 *
 * Every logical detector index in [firstIndex, lastIndex] is "virtual":
 *  - Its world position is computed on demand from the grid formula:
 *        pos = bankPos + bankRot * { xstart + ix*xstep, ystart + iy*ystep, 0 }
 *    where ix = local / ny,  iy = local % ny,  local = index - firstIndex.
 *    (Fill order is Y-fastest / "yxz", matching PixelAssembly IDF convention.)
 *  - Its parent component is bankCompIdx.
 *  - It has the identity scale factor and an empty name string.
 *
 * Segments are stored sorted by ascending firstIndex so that O(N_banks)
 * linear scans can break early.
 */
struct MANTID_BEAMLINE_DLL VirtualBankSegment {
  size_t firstIndex = 0; ///< first logical detector index (inclusive)
  size_t lastIndex = 0;  ///< last  logical detector index (inclusive)

  /// Component index of the parent bank assembly in Beamline::ComponentInfo.
  /// Set by InstrumentVisitor after the bank is registered.
  size_t bankCompIdx = 0;

  // --- Fields used by Beamline::DetectorInfo (position / rotation) ----------
  Eigen::Vector3d bankPos{0.0, 0.0, 0.0};
  Eigen::Quaterniond bankRot{Eigen::Quaterniond::Identity()};
  int nx = 1;          ///< pixels along X (fast axis = Y, so X is the outer loop)
  int ny = 1;          ///< pixels along Y (fast axis)
  double xstart = 0.0; ///< X centre of pixel (ix=0)
  double xstep = 0.0;  ///< X step between pixels
  double ystart = 0.0;
  double ystep = 0.0;
};

} // namespace Beamline
} // namespace Mantid
