// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidBeamline/DllConfig.h"
#include <Eigen/Geometry>
#include <array>
#include <cstddef>
#include <cstdint>

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
  int nz = 1;          ///< pixels along Z
  double xstart = 0.0; ///< X centre of pixel (ix=0)
  double xstep = 0.0;  ///< X step between pixels
  double ystart = 0.0;
  double ystep = 0.0;

  // --- Fields used for ID-based lookup (detector ID → logical index) ---------
  /// Detector ID at grid position (0, 0, 0) — same as IVirtualBank::referentDetectorID().
  int32_t idstart = 0;
  /// Detector ID at the last grid position — used for range checks in binary search.
  int32_t lastDetId = 0;
  /// ID increment per pixel along the fastest fill axis (= IVirtualBank::idstep()).
  int idstep = 1;
  /// Fill order: idFillOrder[0] is the fastest axis, [2] is slowest (each is 'x', 'y', or 'z').
  std::array<char, 3> idFillOrder{{'y', 'x', 'z'}};

  /** Return the logical detector index for grid position (@p ix, @p iy, @p iz).
   *  Produces the same result as indexOf(idAtXYZ(ix, iy, iz)) but avoids the
   *  ID round-trip.  Iteration order in registerVirtualBank: z-outer, x-middle,
   *  y-inner — so local = iz*nx*ny + ix*ny + iy. */
  size_t indexAtXYZ(int ix, int iy, int iz = 0) const noexcept {
    const size_t local = static_cast<size_t>(iz) * static_cast<size_t>(nx) * static_cast<size_t>(ny) +
                         static_cast<size_t>(ix) * static_cast<size_t>(ny) + static_cast<size_t>(iy);
    return firstIndex + local;
  }

  /** Return the detector ID for logical index @p logicalIndex.
   *
   *  Inverse of indexOf(): decodes (iz, ix, iy) from the iteration-order offset
   *  (z-outer, x-middle, y-inner), maps through idFillOrder, and computes the ID. */
  int32_t idAtIndex(size_t logicalIndex) const noexcept {
    const size_t local = logicalIndex - firstIndex;
    const size_t nxy = static_cast<size_t>(nx) * static_cast<size_t>(ny);
    const int iz = static_cast<int>(local / nxy);
    const int ix = static_cast<int>((local % nxy) / static_cast<size_t>(ny));
    const int iy = static_cast<int>(local % static_cast<size_t>(ny));
    // Map (ix, iy, iz) → fill-order coordinates (reverse of what indexOf does).
    const int coords[3] = {ix, iy, iz}; // 0=x, 1=y, 2=z
    const auto axisIdx = [](char axis) noexcept { return axis == 'x' ? 0 : axis == 'y' ? 1 : 2; };
    const int c0 = coords[axisIdx(idFillOrder[0])];
    const int c1 = coords[axisIdx(idFillOrder[1])];
    const int c2 = coords[axisIdx(idFillOrder[2])];
    const int n0 = idFillOrder[0] == 'x' ? nx : idFillOrder[0] == 'y' ? ny : nz;
    const int n1 = idFillOrder[1] == 'x' ? nx : idFillOrder[1] == 'y' ? ny : nz;
    const int fill_index = c0 + n0 * (c1 + n1 * c2);
    return idstart + static_cast<int32_t>(fill_index) * static_cast<int32_t>(idstep);
  }

  /** Return the logical detector index for detector ID @p id.
   *
   *  Precondition: id is a valid pixel ID in this bank (no bounds check performed).
   *  The formula inverts IVirtualBank::getDetectorIDAtXYZ and then converts the
   *  resulting (ix, iy, iz) grid position to the iteration-order offset used by
   *  InstrumentVisitor::registerVirtualBank (z-outer, x-middle, y-inner). */
  size_t indexOf(int32_t id) const noexcept {
    const int offset = static_cast<int>(id - idstart);
    const int n0 = idFillOrder[0] == 'x' ? nx : idFillOrder[0] == 'y' ? ny : nz;
    const int n1 = idFillOrder[1] == 'x' ? nx : idFillOrder[1] == 'y' ? ny : nz;
    const int step0 = idstep;
    const int step1 = n0 * step0;
    const int step2 = n1 * step1;
    const int c0 = (offset / step0) % n0;
    const int c1 = (offset / step1) % n1;
    const int c2 = offset / step2;
    const int ix = (idFillOrder[0] == 'x' ? c0 : idFillOrder[1] == 'x' ? c1 : c2);
    const int iy = (idFillOrder[0] == 'y' ? c0 : idFillOrder[1] == 'y' ? c1 : c2);
    const int iz = (idFillOrder[0] == 'z' ? c0 : idFillOrder[1] == 'z' ? c1 : c2);
    // Iteration order in registerVirtualBank: z-outer, x-middle, y-inner.
    const size_t local = static_cast<size_t>(iz) * static_cast<size_t>(nx) * static_cast<size_t>(ny) +
                         static_cast<size_t>(ix) * static_cast<size_t>(ny) + static_cast<size_t>(iy);
    return firstIndex + local;
  }
};

} // namespace Beamline
} // namespace Mantid
