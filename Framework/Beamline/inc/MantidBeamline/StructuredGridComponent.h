// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <vector>

namespace Mantid {
namespace Beamline {

/** Metadata of a Structured bank: a grid of pixels that is topologically regular but
 * geometrically irregular, so its pixel boundaries are given as vertices rather than
 * as a start and a step.
 */
struct StructuredGridComponent {
  /// Number of pixels in the X (horizontal) direction
  int nX = 0;
  /// Number of pixels in the Y (vertical) direction
  int nY = 0;
  /// Detector ID of the first pixel
  int idStart = 0;
  /// Step size in ID for each pixel along the first-filled axis
  int idStep = 0;
  /// Step size in ID for each row along the second-filled axis
  int idStepByRow = 0;
  /// Whether detector IDs are filled along Y first
  bool idFillByFirstY = false;
  /// X and Y coordinates, in the bank's own local frame, of the (nX + 1) * (nY + 1) pixel
  /// vertices. Vertex (x, y) is at position y * (nX + 1) + x.
  std::vector<double> xVertices;
  std::vector<double> yVertices;
};

} // namespace Beamline
} // namespace Mantid
