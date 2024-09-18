// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Instrument/SolidAngleParams.h"
#include "MantidGeometry/Rendering/ShapeInfo.h"
#include <map>
#include <memory>
#include <optional>
#include <vector>

namespace Mantid {

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
namespace Kernel {
class PseudoRandomNumberGenerator;
class Material;
class V3D;
} // namespace Kernel

namespace Geometry {
class BoundingBox;
class GeometryHandler;
class Surface;
class Track;
class vtkGeometryCacheReader;
class vtkGeometryCacheWriter;

/** IObject : Interface for geometry objects
 */

/**
 * Interface for Geometry Objects
 */
class MANTID_GEOMETRY_DLL IObject {
public:
  virtual ~IObject() = default;
  virtual bool isValid(const Kernel::V3D &) const = 0;
  virtual bool isOnSide(const Kernel::V3D &) const = 0;
  virtual bool isFiniteGeometry() const { return true; }
  virtual void setFiniteGeometryFlag(bool) {}
  virtual bool hasValidShape() const = 0;
  virtual IObject *clone() const = 0;
  virtual IObject *cloneWithMaterial(const Kernel::Material &material) const = 0;

  virtual int getName() const = 0;

  virtual int interceptSurface(Geometry::Track &) const = 0;
  virtual double distance(const Geometry::Track &) const = 0;
  // Solid angle
  virtual double solidAngle(const SolidAngleParams &params) const = 0;
  // Solid angle with a scaling of the object
  virtual double solidAngle(const SolidAngleParams &params, const Kernel::V3D &scaleFactor) const = 0;
  /// Return cached value of axis-aligned bounding box
  virtual const BoundingBox &getBoundingBox() const = 0;
  /// Calculate (or return cached value of) Axis Aligned Bounding box
  /// (DEPRECATED)
  virtual void getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin, double &ymin,
                              double &zmin) const = 0;
  virtual double volume() const = 0;

  virtual int getPointInObject(Kernel::V3D &point) const = 0;

  virtual std::optional<Kernel::V3D> generatePointInObject(Kernel::PseudoRandomNumberGenerator &rng,
                                                           const size_t) const = 0;
  virtual std::optional<Kernel::V3D> generatePointInObject(Kernel::PseudoRandomNumberGenerator &rng,
                                                           const BoundingBox &activeRegion, const size_t) const = 0;

  virtual detail::ShapeInfo::GeometryShape shape() const = 0;
  virtual const detail::ShapeInfo &shapeInfo() const = 0;
  virtual void GetObjectGeom(detail::ShapeInfo::GeometryShape &type, std::vector<Kernel::V3D> &vectors,
                             double &innerRadius, double &radius, double &height) const = 0;
  // Rendering
  virtual void draw() const = 0;
  virtual void initDraw() const = 0;

  virtual const Kernel::Material &material() const = 0;
  virtual void setMaterial(const Kernel::Material &material) = 0;
  virtual const std::string &id() const = 0;
  virtual void setID(const std::string &id) = 0;

  virtual std::shared_ptr<GeometryHandler> getGeometryHandler() const = 0;
};

/// Typdef for a shared pointer
using IObject_sptr = std::shared_ptr<IObject>;
/// Typdef for a shared pointer to a const object
using IObject_const_sptr = std::shared_ptr<const IObject>;
/// Typdef for a unique pointer
using IObject_uptr = std::unique_ptr<IObject>;
/// Typdef for a unique pointer to a const object
using IObject_const_uptr = std::unique_ptr<const IObject>;

} // namespace Geometry
} // namespace Mantid
