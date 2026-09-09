// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidKernel/V3D.h"
#include <array>
#include <memory>
#include <tuple>

namespace Mantid {
namespace Geometry {

/**
 * IVirtualBank — interface for a regular 3-D grid of detector pixels.
 *
 * All pixel properties (detector ID, relative position) are computed on demand
 * from a small set of grid parameters; no per-pixel child objects are required.
 *
 * Axis convention:
 *   X = column index  [0, xpixels())
 *   Y = row index     [0, ypixels())
 *   Z = layer index   [0, zpixels())
 *
 * The **referent pixel** is the pixel at grid coordinates (0, 0, 0).  It has
 * detector ID referentDetectorID(), and its relative position in the bank's
 * local frame is (xstart(), ystart(), zstart()).  Every other pixel is located
 * and numbered relative to it.
 *
 * Subclasses must supply the seven pure-virtual groups below (pixel counts,
 * step sizes, start positions, ID scheme, and the referent Detector object).
 * The remaining methods are concrete and are implemented entirely from those
 * primitives.
 */
class MANTID_GEOMETRY_DLL IVirtualBank {
public:
  virtual ~IVirtualBank() = default;

  // ---- Pixel counts (pure virtual) ----------------------------------------

  virtual size_t xpixels() const = 0;
  virtual size_t ypixels() const = 0;
  virtual size_t zpixels() const = 0;

  /// Total pixel count
  size_t npixels() const { return xpixels() * ypixels() * zpixels(); }

  // ---- Step sizes (pure virtual) ------------------------------------------
  // Parametrized instruments may scale these via scalex / scaley / scalez.

  virtual double xstep() const = 0;
  virtual double ystep() const = 0;
  virtual double zstep() const = 0;

  // ---- Start positions of the referent pixel (pure virtual) ---------------
  // These are the referent pixel's coordinates in the bank's local frame.

  virtual double xstart() const = 0;
  virtual double ystart() const = 0;
  virtual double zstart() const = 0;

  // ---- Total span (concrete) -----------------------------------------------

  double xsize() const { return static_cast<double>(xpixels()) * xstep(); }
  double ysize() const { return static_cast<double>(ypixels()) * ystep(); }
  double zsize() const { return static_cast<double>(zpixels()) * zstep(); }

  // ---- ID scheme (pure virtual) --------------------------------------------

  /// Detector ID assigned to the referent pixel (0, 0, 0).
  virtual detid_t referentDetectorID() const = 0;

  /// Fill order: element [0] is the fastest-varying axis, [2] the slowest.
  /// Valid values for each element: 'x', 'y', or 'z'.
  /// Example: {'x', 'y', 'z'} means X varies fastest, Z slowest.
  virtual std::array<char, 3> idFillOrder() const = 0;

  /// ID increment between adjacent pixels along the fastest-varying axis.
  virtual int idstep() const = 0;

  // ---- Referent pixel object (pure virtual) --------------------------------

  /// Returns a Detector object for the referent pixel (0, 0, 0), constructed
  /// on demand.  The Detector's parent is this bank; its shape is the pixel
  /// shape.  Callers should not cache the result across re-initializations.
  virtual std::shared_ptr<Detector> referentDetector() const = 0;

  // ---- ID range (concrete) ------------------------------------------------

  detid_t minDetectorID() const { return referentDetectorID(); }
  detid_t maxDetectorID() const {
    return getDetectorIDAtXYZ(static_cast<int>(xpixels()) - 1, static_cast<int>(ypixels()) - 1,
                              static_cast<int>(zpixels()) - 1);
  }

  // ---- Bounds check (concrete) --------------------------------------------

  bool inBoundsXYZ(int x, int y, int z) const {
    return x >= 0 && static_cast<size_t>(x) < xpixels() && y >= 0 && static_cast<size_t>(y) < ypixels() && z >= 0 &&
           static_cast<size_t>(z) < zpixels();
  }

  // ---- ID / coordinate conversion (concrete) ------------------------------

  detid_t getDetectorIDAtXYZ(int x, int y, int z) const;
  std::tuple<int, int, int> getXYZForDetectorID(detid_t detectorID) const;

  // ---- Relative position (concrete) ---------------------------------------

  /// Position of pixel (x, y, z) in the bank's local coordinate frame,
  /// before the bank's rotation is applied.
  Kernel::V3D getRelativePosAtXYZ(int x, int y, int z) const {
    return Kernel::V3D(xstart() + static_cast<double>(x) * xstep(), ystart() + static_cast<double>(y) * ystep(),
                       zstart() + static_cast<double>(z) * zstep());
  }
};

} // namespace Geometry
} // namespace Mantid
