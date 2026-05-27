// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/IVirtualBank.h"
#include <stdexcept>

namespace Mantid {
namespace Geometry {

namespace {

/// Return the pixel count along a named axis.
size_t pixelCount(IVirtualBank const *bank, char axis) {
  switch (axis) {
  case 'x':
    return bank->xpixels();
  case 'y':
    return bank->ypixels();
  case 'z':
    return bank->zpixels();
  default:
    throw std::invalid_argument(std::string("IVirtualBank: unknown fill-order axis '") + axis + "'");
  }
}

/// Return the grid coordinate along a named axis for a given pixel.
int coordAlong(char axis, int x, int y, int z) {
  switch (axis) {
  case 'x':
    return x;
  case 'y':
    return y;
  default: // 'z'
    return z;
  }
}

} // anonymous namespace

/**
 * Compute the detector ID for pixel (x, y, z).
 *
 * The ID scheme is defined by:
 *   - referentDetectorID() : ID of the pixel at (0,0,0)
 *   - idFillOrder()        : which axis varies fastest / slowest
 *   - idstep()             : increment per pixel along the fastest axis
 *
 * The increment along the middle axis is  idstep() * pixels_along_fastest,
 * and along the slowest axis is  idstep() * pixels_fastest * pixels_middle.
 */
detid_t IVirtualBank::getDetectorIDAtXYZ(int x, int y, int z) const {
  auto const order = idFillOrder(); // order[0] = fastest, order[2] = slowest
  size_t const n0 = pixelCount(this, order[0]);
  size_t const n1 = pixelCount(this, order[1]);
  auto const step0 = static_cast<detid_t>(idstep());
  auto const step1 = static_cast<detid_t>(n0) * step0;
  auto const step2 = static_cast<detid_t>(n1) * step1;
  return referentDetectorID() + coordAlong(order[0], x, y, z) * step0 + coordAlong(order[1], x, y, z) * step1 +
         coordAlong(order[2], x, y, z) * step2;
}

/**
 * Invert getDetectorIDAtXYZ: return the (x, y, z) grid coordinates for a
 * given detector ID.
 *
 * Behaviour for IDs outside the valid range is undefined (no bounds check
 * is performed here; callers should use inBoundsXYZ on the result if needed).
 */
std::tuple<int, int, int> IVirtualBank::getXYZForDetectorID(detid_t detectorID) const {
  auto const order = idFillOrder();
  int const n0 = static_cast<int>(pixelCount(this, order[0]));
  int const n1 = static_cast<int>(pixelCount(this, order[1]));
  int const offset = static_cast<int>(detectorID - referentDetectorID());
  // Strides must account for idstep (increment per individual pixel).
  int const step0 = idstep();
  int const step1 = n0 * step0;
  int const step2 = n1 * step1;
  // Decompose offset into the three fill-order coordinates
  int const c2 = offset / step2;
  int const c1 = (offset % step2) / step1;
  int const c0 = (offset % step1) / step0;
  // Map fill-order coordinates back to (x, y, z)
  int x = (order[0] == 'x' ? c0 : order[1] == 'x' ? c1 : c2);
  int y = (order[0] == 'y' ? c0 : order[1] == 'y' ? c1 : c2);
  int z = (order[0] == 'z' ? c0 : order[1] == 'z' ? c1 : c2);
  return {x, y, z};
}

} // namespace Geometry
} // namespace Mantid
