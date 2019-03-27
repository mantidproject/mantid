// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_IOBJECT_H_
#define MANTID_GEOMETRY_IOBJECT_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Rendering/ShapeInfo.h"
#include <boost/shared_ptr.hpp>
#include <map>
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
  virtual IObject *
  cloneWithMaterial(const Kernel::Material &material) const = 0;

  virtual int getName() const = 0;

  virtual int interceptSurface(Geometry::Track &) const = 0;
  // Solid angle
  virtual double solidAngle(const Kernel::V3D &observer) const = 0;
  // Solid angle with a scaling of the object
  virtual double solidAngle(const Kernel::V3D &observer,
                            const Kernel::V3D &scaleFactor) const = 0;
  /// Return cached value of axis-aligned bounding box
  virtual const BoundingBox &getBoundingBox() const = 0;
  /// Calculate (or return cached value of) Axis Aligned Bounding box
  /// (DEPRECATED)
  virtual void getBoundingBox(double &xmax, double &ymax, double &zmax,
                              double &xmin, double &ymin,
                              double &zmin) const = 0;
  virtual double volume() const = 0;

  virtual int getPointInObject(Kernel::V3D &point) const = 0;
  virtual Kernel::V3D
  generatePointInObject(Kernel::PseudoRandomNumberGenerator &rng,
                        const size_t) const = 0;
  virtual Kernel::V3D
  generatePointInObject(Kernel::PseudoRandomNumberGenerator &rng,
                        const BoundingBox &activeRegion,
                        const size_t) const = 0;

  virtual detail::ShapeInfo::GeometryShape shape() const = 0;
  virtual void GetObjectGeom(detail::ShapeInfo::GeometryShape &type,
                             std::vector<Kernel::V3D> &vectors, double &innerRadius,
                             double &radius, double &height) const = 0;
  // Rendering
  virtual void draw() const = 0;
  virtual void initDraw() const = 0;

  virtual const Kernel::Material &material() const = 0;
  virtual const std::string &id() const = 0;

  virtual boost::shared_ptr<GeometryHandler> getGeometryHandler() const = 0;
};

/// Typdef for a shared pointer
using IObject_sptr = boost::shared_ptr<IObject>;
/// Typdef for a shared pointer to a const object
using IObject_const_sptr = boost::shared_ptr<const IObject>;
/// Typdef for a unique pointer
using IObject_uptr = std::unique_ptr<IObject>;
/// Typdef for a unique pointer to a const object
using IObject_const_uptr = std::unique_ptr<const IObject>;

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_IOBJECT_H_ */
