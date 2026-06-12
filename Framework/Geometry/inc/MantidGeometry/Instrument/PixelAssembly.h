// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/IDTypes.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/IVirtualBank.h"
#include "MantidGeometry/Objects/IObject.h"
#include <array>
#include <memory>
#include <string>

namespace Mantid {
namespace Geometry {

class BoundingBox;
class ComponentVisitor;
class Detector;
class Track;

/**
 * PixelAssembly — a detector bank for regular 3-D grids of pixels.
 *
 * Inherits from Component (not CompAssembly) and creates NO per-pixel child
 * Detector objects.  All pixel properties (detector ID, relative position,
 * bounding box) are computed on demand from the grid parameters stored via
 * IVirtualBank.
 *
 * Intended for large-pixel-count instruments (e.g. IMAGINE with ~20 M pixels)
 * where allocating one Detector object per pixel would be prohibitive.
 *
 * Constraints vs GridDetector:
 *  - IDs must be packed (no gaps): the row stride equals
 *    idstep * pixels_along_fastest_axis.
 *  - zpixels must be >= 1 (a flat bank uses zpixels = 1).
 *
 * @class  PixelAssembly
 * @brief  Virtual-pixel 3D detector bank
 * @author rboston628
 */
class MANTID_GEOMETRY_DLL PixelAssembly : public Component, public IObjComponent, public IVirtualBank {

public:
  /// String description of the type of component
  std::string type() const override { return "PixelAssembly"; }

  //! Constructor with a name and optional parent reference
  explicit PixelAssembly(const std::string &name, IComponent *reference = nullptr);

  //! Parametrized constructor
  PixelAssembly(const PixelAssembly *base, const ParameterMap *map);

  /// Returns true if the proposed name matches the PixelAssembly naming convention.
  static bool compareName(const std::string &proposedMatch);

  /**
   * Store the grid parameters.  No child Detector objects are created.
   * Call this after setting the assembly's name, position and rotation.
   *
   * @param shape      Shape shared by every pixel.
   * @param xpixels    Number of pixels in X.
   * @param xstart     X coordinate of the referent pixel (0,0,0), local frame.
   * @param xstep      Step between adjacent pixels in X, local frame.
   * @param ypixels    Number of pixels in Y.
   * @param ystart     Y coordinate of the referent pixel, local frame.
   * @param ystep      Step between adjacent pixels in Y, local frame.
   * @param zpixels    Number of pixels in Z (pass 0 or 1 for a flat bank).
   * @param zstart     Z coordinate of the referent pixel, local frame.
   * @param zstep      Step between adjacent pixels in Z, local frame.
   * @param idstart    Detector ID of the referent pixel (0,0,0).
   * @param idFillOrder Three-character string: e.g. "xyz" means X varies fastest.
   * @param idstep     ID increment between adjacent pixels along the fastest axis.
   */
  void initialize(std::shared_ptr<IObject> shape, size_t xpixels, double xstart, double xstep, size_t ypixels,
                  double ystart, double ystep, size_t zpixels, double zstart, double zstep, detid_t idstart,
                  const std::string &idFillOrder, int idstep = 1);

  //! Make a clone of the present component
  PixelAssembly *clone() const override;

  // ---- IVirtualBank pure virtuals ----

  size_t xpixels() const override;
  size_t ypixels() const override;
  size_t zpixels() const override;

  double xstep() const override;
  double ystep() const override;
  double zstep() const override;

  double xstart() const override;
  double ystart() const override;
  double zstart() const override;

  detid_t referentDetectorID() const override;
  std::array<char, 3> idFillOrder() const override;
  int idstep() const override;

  /// Returns a Detector object for the referent pixel (0,0,0), constructed on demand.
  std::shared_ptr<Detector> referentDetector() const override;

  // ---- Position helpers ----

  /// Absolute position of pixel (x,y,z) in the instrument frame.
  Kernel::V3D getPosAtXYZ(int x, int y, int z) const;

  // ---- Per-pixel bounding box ----

  void getBoundingBoxAtXYZ(int x, int y, int z, BoundingBox &box) const;

  // ---- Component lookup ----
  // NOTE: IComponent does not declare getComponentByName; only ICompAssembly does.
  // PixelAssembly intentionally does not inherit ICompAssembly, so this method
  // is not available through the standard IComponent pointer interface.
  // Direct callers (e.g. InstrumentDefinitionParser) should hold a PixelAssembly*.
  std::shared_ptr<const IComponent> getComponentByName(const std::string &cname) const;

  // ---- Visitor registration ----

  size_t registerContents(class ComponentVisitor &componentVisitor) const override;

  // ---- Pixel shape (separate from the overall rendering shape) ----

  std::shared_ptr<const IObject> pixelShape() const;

  // ---- IObjComponent methods ----

  /// Does the point lie within this component?  (Not implemented — throws.)
  bool isValid(const Kernel::V3D &point) const override;
  /// Does the point lie on the surface?  (Not implemented — throws.)
  bool isOnSide(const Kernel::V3D &point) const override;
  /// Check whether a track passes through this component.  (Not implemented — throws.)
  int interceptSurface(Track &track) const override;
  /// Approximate solid angle.  (Not implemented — throws.)
  double solidAngle(const Geometry::SolidAngleParams &params) const override;
  /// Retrieve or compute the bounding box for the whole assembly.
  void getBoundingBox(BoundingBox &assemblyBox) const override;
  /// Find a point inside the object.  (Not implemented — throws.)
  int getPointInObject(Kernel::V3D &point) const override;

  // Rendering
  void draw() const override;
  void drawObject() const override;
  void initDraw() const override;

  /// Returns a cuboid representing the whole assembly (for rendering).
  const std::shared_ptr<const IObject> shape() const override;
  /// Returns an empty material.
  const Kernel::Material material() const override;

  // ---- End of IObjComponent methods ----

protected:
  /// Initialise all members to safe defaults.
  void init();

private:
  void validateInput() const;
  void initializeValues(std::shared_ptr<IObject> shape, size_t xpixels, double xstart, double xstep, size_t ypixels,
                        double ystart, double ystep, size_t zpixels, double zstart, double zstep, detid_t idstart,
                        const std::string &idFillOrder, int idstep);

  /// True when this is a parametrized view of m_gridBase.
  bool isParametrized() const override { return m_map && m_gridBase; }

  /// Pointer to the base (unparametrized) assembly; nullptr when not parametrized.
  const PixelAssembly *m_gridBase;

  /// Deleted copy-assignment.
  PixelAssembly &operator=(const PixelAssembly &) = delete;

  // ---- Grid geometry ----
  size_t m_xpixels; ///< Number of pixels in X
  size_t m_ypixels; ///< Number of pixels in Y
  size_t m_zpixels; ///< Number of pixels in Z (always >= 1)

  double m_xstart; ///< X coordinate of the referent pixel, local frame
  double m_ystart; ///< Y coordinate of the referent pixel, local frame
  double m_zstart; ///< Z coordinate of the referent pixel, local frame

  double m_xstep; ///< Step between adjacent pixels in X, local frame
  double m_ystep; ///< Step between adjacent pixels in Y, local frame
  double m_zstep; ///< Step between adjacent pixels in Z, local frame

  /// Shape shared by every pixel in this assembly.
  std::shared_ptr<IObject> m_shape;

  // ---- ID scheme ----
  detid_t m_idstart;                 ///< ID of the referent pixel (0,0,0)
  std::array<char, 3> m_idFillOrder; ///< Fill order, e.g. {'x','y','z'}
  int m_idstep;                      ///< ID increment per pixel along fastest axis
};

MANTID_GEOMETRY_DLL std::ostream &operator<<(std::ostream &, const PixelAssembly &);

using PixelAssembly_sptr = std::shared_ptr<PixelAssembly>;
using PixelAssembly_const_sptr = std::shared_ptr<const PixelAssembly>;

} // namespace Geometry
} // namespace Mantid
