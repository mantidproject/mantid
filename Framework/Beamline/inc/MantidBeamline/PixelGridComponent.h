// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <array>

namespace Mantid {
namespace Beamline {

/** PixelGridComponent : structural metadata for a Rectangular/Grid detector
 * bank, i.e. the legacy Geometry::RectangularDetector/GridDetector fields
 * that have no other home on ComponentInfo. Pixel counts and detector-ID
 * numbering are never parametrized (unlike position/rotation/scale), so this
 * is captured once, at instrument-build time, and never changes afterwards.
 */
struct PixelGridComponent {
  /// Number of pixels in the X (horizontal) direction
  int nX = 0;
  /// Number of pixels in the Y (vertical) direction
  int nY = 0;
  /// Number of pixels in the Z (usually beam) direction; 0 for a 2D (rectangular) bank
  int nZ = 0;
  /// Position (in the bank's own local, unrotated, unscaled frame) of pixel (0, 0, 0)
  double xStart = 0;
  double yStart = 0;
  double zStart = 0;
  /// Step size between neighbouring pixels; captured once at instrument-build
  /// time so any parametrized scaling (e.g. "scalex") already baked in.
  double xStep = 0;
  double yStep = 0;
  double zStep = 0;
  /// Detector ID of the first pixel
  int idStart = 0;
  /// Step size in ID for each pixel along the first-filled axis
  int idStep = 0;
  /// Step size in ID for each row along the second-filled axis
  int idStepByRow = 0;
  /// Axis order (each of 'x', 'y', 'z') in which detector IDs are filled
  std::array<char, 3> idFillOrder{{'x', 'y', 'z'}};
  /// Minimum detector ID in the bank
  int minDetectorID = 0;
  /// Maximum detector ID in the bank
  int maxDetectorID = 0;
};

} // namespace Beamline
} // namespace Mantid
