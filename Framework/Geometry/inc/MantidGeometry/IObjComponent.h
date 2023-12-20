// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/SolidAngleParams.h"
#include "MantidGeometry/Objects/IObject.h"

namespace Mantid {

namespace Kernel {
class Material;
}

namespace Geometry {
class Track;
class IObject;
class GeometryHandler;

/** Object Component class, this class brings together the physical attributes
   of the component
    to the positioning and geometry tree.

    Notably, this contains a GeometryHandler and methods used to render the
   component in
    the instrument 3D view.

    @author Laurent C Chapon, ISIS, RAL
    @date 26/09/2007
    @author Russell Taylor, Tessella Support Services plc
    @date 26/06/2008
*/
class MANTID_GEOMETRY_DLL IObjComponent : public virtual IComponent {
public:
  /// type string
  std::string type() const override { return "IObjComponent"; }

  IObjComponent();

  IObjComponent(GeometryHandler *the_handler);

  IObjComponent(const IObjComponent &);

  IObjComponent &operator=(const IObjComponent &rhs);

  // Looking to get rid of the first of these constructors in due course (and
  // probably add others)
  ~IObjComponent() override;

  /** Virtual Copy Constructor
   *  @returns A pointer to a copy of the input ObjComponent
   */
  IComponent *clone() const override = 0;

  /// Does the point given lie within this object component?
  virtual bool isValid(const Kernel::V3D &point) const = 0;

  /// Does the point given lie on the surface of this object component?
  virtual bool isOnSide(const Kernel::V3D &point) const = 0;

  /// Checks whether the track given will pass through this Component.
  virtual int interceptSurface(Track &track) const = 0;

  /// Finds the approximate solid angle covered by the component when viewed
  /// from the point given
  virtual double solidAngle(const Geometry::SolidAngleParams &params) const = 0;

  /// Try to find a point that lies within (or on) the object
  virtual int getPointInObject(Kernel::V3D &point) const = 0;

  // Rendering member functions
  /// Draws the objcomponent.
  virtual void draw() const = 0;

  /// Draws the Object.
  virtual void drawObject() const = 0;

  /// Initializes the ObjComponent for rendering, this function should be called
  /// before rendering.
  virtual void initDraw() const = 0;

  /// Returns the shape of the Object
  virtual const std::shared_ptr<const IObject> shape() const = 0;
  /// Returns the material of the Object
  virtual const Kernel::Material material() const = 0;

  /// Gets the GeometryHandler
  GeometryHandler *Handle() const { return handle.get(); }

protected:
  /// Reset the current geometry handler
  void setGeometryHandler(GeometryHandler *h);

private:
  /// Geometry Handle for rendering
  std::unique_ptr<GeometryHandler> handle;

  friend class GeometryHandler;
};

/// Shared pointer to IObjComponent
using IObjComponent_sptr = std::shared_ptr<IObjComponent>;
/// Shared pointer to IObjComponent (const version)
using IObjComponent_const_sptr = std::shared_ptr<const IObjComponent>;

} // namespace Geometry
} // namespace Mantid
