// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/Instrument/Component.h"

#ifdef _WIN32
#pragma warning(disable : 4250)
#endif

namespace Mantid {
namespace Geometry {
class Objects;
//----------------------------------------------------------------------
// Forward Declaration
//----------------------------------------------------------------------

/** Object Component class, this class brings together the physical attributes
   of the component
    to the positioning and geometry tree.

    @author Laurent C Chapon, ISIS, RAL
    @date 26/09/2007
    @author Russell Taylor, Tessella Support Services plc
    @date 26/06/2008
*/
class MANTID_GEOMETRY_DLL ObjComponent : public virtual IObjComponent, public Component {
public:
  /// type string
  std::string type() const override { return "PhysicalComponent"; }

  /// Constructor for parametrized component
  ObjComponent(const IComponent *base, const ParameterMap *map);
  // Looking to get rid of the first of these constructors in due course (and
  // probably add others)
  explicit ObjComponent(const std::string &name, IComponent *parent = nullptr);
  explicit ObjComponent(const std::string &name, std::shared_ptr<const IObject> shape, IComponent *parent = nullptr);

  /** Virtual Copy Constructor
   *  @returns A pointer to a copy of the input ObjComponent
   */
  IComponent *clone() const override { return new ObjComponent(*this); }

  bool isValid(const Kernel::V3D &point) const override;
  bool isOnSide(const Kernel::V3D &point) const override;
  int interceptSurface(Track &track) const override;
  double solidAngle(const Geometry::SolidAngleParams &params) const override;
  ///@todo This should go in favour of just the class related one.
  void boundingBox(double &xmax, double &ymax, double &zmax, double &xmin, double &ymin, double &zmin) const;
  /// get bounding box, which may or may not be axis aligned;
  void getBoundingBox(BoundingBox &absoluteBB) const override;
  /// get Height (Y-dimension) value for component
  virtual double getHeight() const;
  /// get Width (X-dimension) value for component
  virtual double getWidth() const;
  /// get Depth (Z-dimension) value for component
  virtual double getDepth() const;

  int getPointInObject(Kernel::V3D &point) const override;
  // Rendering member functions
  void draw() const override;
  void drawObject() const override;
  void initDraw() const override;

  /// Return the shape of the component
  const std::shared_ptr<const IObject> shape() const override;
  /// Set a new shape on the component
  /// void setShape(std::shared_ptr<const IObject> newShape);
  void setShape(std::shared_ptr<const IObject> newShape);
  /// Return the material this component is made from
  const Kernel::Material material() const override;

  virtual size_t registerContents(class ComponentVisitor &componentVisitor) const override;

protected:
  /// The physical geometry representation
  // Made a pointer to a const object. Since this is a shared object we
  // shouldn't be
  // exposing non-const methods of Object through this class.
  std::shared_ptr<const IObject> m_shape;

  const Kernel::V3D factorOutComponentPosition(const Kernel::V3D &point) const;
  const Kernel::V3D takeOutRotation(Kernel::V3D point) const;

private:
  /// common part of the two Bounding box functions above;
  void getRelativeBoundingBox(BoundingBox &RelativeBB) const;
};

} // namespace Geometry
} // namespace Mantid
