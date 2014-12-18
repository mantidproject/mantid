//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidKernel/Exception.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include <cfloat>

namespace Mantid {
namespace Geometry {
using Kernel::V3D;
using Kernel::Quat;

/** Constructor for a parametrized ObjComponent
 * @param base: the base (un-parametrized) IComponent
 * @param map: pointer to the ParameterMap
 * */
ObjComponent::ObjComponent(const IComponent *base, const ParameterMap *map)
    : Component(base, map), m_shape(), m_material() {}

/** Constructor
*  @param name ::   The name of the component
*  @param parent :: The Parent geometry object of this component
*/
ObjComponent::ObjComponent(const std::string &name, IComponent *parent)
    : IObjComponent(), Component(name, parent), m_shape(), m_material() {}

/** Constructor
*  @param name ::   The name of the component
*  @param shape ::  A pointer to the object describing the shape of this
* component
*  @param parent :: The Parent geometry object of this component
*  @param material :: An optional pointer to the material object of this
* component
*/
ObjComponent::ObjComponent(const std::string &name, Object_const_sptr shape,
                           IComponent *parent, Kernel::Material_sptr material)
    : IObjComponent(), Component(name, parent), m_shape(shape),
      m_material(material) {}

/// Destructor
ObjComponent::~ObjComponent() {}

/** Return the shape of the component
 */
const Object_const_sptr ObjComponent::shape() const {
  if (m_map)
    return dynamic_cast<const ObjComponent *>(m_base)->m_shape;
  else
    return m_shape;
}

/// Set a new shape on the component
void ObjComponent::setShape(Object_const_sptr newShape) {
  if (m_map)
    throw std::runtime_error("ObjComponent::setShape - Cannot change the shape "
                             "of a parameterized object");
  else
    m_shape = newShape;
}

/**
 * Return the material of the component. Currently
 * unaffected by parametrization
 */
const Kernel::Material_const_sptr ObjComponent::material() const {
  return m_material;
}

/// Does the point given lie within this object component?
bool ObjComponent::isValid(const V3D &point) const {
  // If the form of this component is not defined, just treat as a point
  if (!shape())
    return (this->getPos() == point);

  // Otherwise pass through the shifted point to the Object::isValid method
  V3D scaleFactor = this->getScaleFactor();
  return shape()->isValid(factorOutComponentPosition(point) / scaleFactor) != 0;
}

/// Does the point given lie on the surface of this object component?
bool ObjComponent::isOnSide(const V3D &point) const {
  // If the form of this component is not defined, just treat as a point
  if (!shape())
    return (this->getPos() == point);
  // Otherwise pass through the shifted point to the Object::isOnSide method
  V3D scaleFactor = this->getScaleFactor();
  return shape()->isOnSide(factorOutComponentPosition(point) / scaleFactor) !=
         0;
}

/** Checks whether the track given will pass through this Component.
*  @param track :: The Track object to test (N.B. Will be modified if hits are
* found)
*  @returns The number of track segments added (i.e. 1 if the track enters and
* exits the object once each)
*  @throw NullPointerException if the underlying geometrical Object has not been
* set
*/
int ObjComponent::interceptSurface(Track &track) const {
  // If the form of this component is not defined, throw NullPointerException
  if (!shape())
    throw Kernel::Exception::NullPointerException(
        "ObjComponent::interceptSurface", "shape");

  // TODO: If scaling parameters are ever enabled, would they need need to be
  // used here?
  V3D trkStart = factorOutComponentPosition(track.startPoint());
  V3D trkDirection = takeOutRotation(track.direction());

  Track probeTrack(trkStart, trkDirection);
  int intercepts = shape()->interceptSurface(probeTrack);

  Track::LType::const_iterator it;
  for (it = probeTrack.begin(); it != probeTrack.end(); ++it) {
    V3D in = it->entryPoint;
    this->getRotation().rotate(in);
    // use the scale factor
    in *= getScaleFactor();
    in += this->getPos();
    V3D out = it->exitPoint;
    this->getRotation().rotate(out);
    // use the scale factor
    out *= getScaleFactor();
    out += this->getPos();
    track.addLink(in, out, out.distance(track.startPoint()), *(this->shape()),
                  this->getComponentID());
  }

  return intercepts;
}

/** Finds the approximate solid angle covered by the component when viewed from
* the point given
*  @param observer :: The position from which the component is being viewed
*  @returns The solid angle in steradians
*  @throw NullPointerException if the underlying geometrical Object has not been
* set
*/
double ObjComponent::solidAngle(const V3D &observer) const {
  // If the form of this component is not defined, throw NullPointerException
  if (!shape())
    throw Kernel::Exception::NullPointerException("ObjComponent::solidAngle",
                                                  "shape");
  // Otherwise pass through the shifted point to the Object::solidAngle method
  V3D scaleFactor = this->getScaleFactor();
  if ((scaleFactor - V3D(1.0, 1.0, 1.0)).norm() < 1e-12)
    return shape()->solidAngle(factorOutComponentPosition(observer));
  else {
    // This is the observer position in the shape's coordinate system.
    V3D relativeObserver = factorOutComponentPosition(observer);
    // This function will scale the object shape when calculating the solid
    // angle.
    return shape()->solidAngle(relativeObserver, scaleFactor);
  }
}

/**
  * Given an input estimate of the axis aligned (AA) bounding box (BB), return
 * an improved set of values.
  * The AA BB is determined in the frame of the object and the initial estimate
 * will be transformed there.
  * The returned BB will be the frame of the ObjComponent and may not be
 * optimal.
  * @param absoluteBB :: [InOut] The bounding box for this object component will
 * be stored here.
  * if BB alignment is different from axis alignment, the system of coordinates
 * to alighn is taken fron
  * the absoluteBB
  */
void ObjComponent::getBoundingBox(BoundingBox &absoluteBB) const {

  // Start with the box in the shape's coordinates
  const Object_const_sptr s = shape();
  if (!s) {
    absoluteBB.nullify();
    return;
  }
  const BoundingBox &shapeBox = s->getBoundingBox();
  if (shapeBox.isNull()) {
    absoluteBB.nullify();
    return;
  }
  std::vector<V3D> Coord_system;
  if (!absoluteBB.isAxisAligned()) { // copy coordinate system (it is better
                                     // then copying the whole BB later)
    Coord_system.assign(absoluteBB.getCoordSystem().begin(),
                        absoluteBB.getCoordSystem().end());
  }
  absoluteBB = BoundingBox(shapeBox);
  // modify in place for speed
  V3D scaleFactor = getScaleFactor();
  // Scale
  absoluteBB.xMin() *= scaleFactor.X();
  absoluteBB.xMax() *= scaleFactor.X();
  absoluteBB.yMin() *= scaleFactor.Y();
  absoluteBB.yMax() *= scaleFactor.Y();
  absoluteBB.zMin() *= scaleFactor.Z();
  absoluteBB.zMax() *= scaleFactor.Z();
  // Rotate
  (this->getRotation()).rotateBB(absoluteBB.xMin(), absoluteBB.yMin(),
                                 absoluteBB.zMin(), absoluteBB.xMax(),
                                 absoluteBB.yMax(), absoluteBB.zMax());

  // Shift
  const V3D localPos = this->getPos();
  absoluteBB.xMin() += localPos.X();
  absoluteBB.xMax() += localPos.X();
  absoluteBB.yMin() += localPos.Y();
  absoluteBB.yMax() += localPos.Y();
  absoluteBB.zMin() += localPos.Z();
  absoluteBB.zMax() += localPos.Z();

  if (!Coord_system.empty()) {
    absoluteBB.realign(&Coord_system);
  }
}

/**
* Gets the Height of the object by querying the underlying BoundingBox.
* @return height of object
*/
double ObjComponent::getHeight() const {
  const BoundingBox &bbox = shape()->getBoundingBox();
  return (bbox.yMax() - bbox.yMin()) / getScaleFactor().Y();
}

/**
* Gets the Width of the object by querying the underlying BoundingBox.
* @return width of object
*/
double ObjComponent::getWidth() const {
  const BoundingBox &bbox = shape()->getBoundingBox();
  return (bbox.xMax() - bbox.xMin()) / getScaleFactor().X();
}

/**
* Gets the Depth of the object by querying the underlying BoundingBox.
* @return depth of object
*/
double ObjComponent::getDepth() const {
  const BoundingBox &bbox = shape()->getBoundingBox();
  return (bbox.zMax() - bbox.zMin()) / getScaleFactor().Z();
}

/**
* Try to find a point that lies within (or on) the object
* @param point :: On exit, set to the point value (if found)
* @return 1 if point found, 0 otherwise
*/
int ObjComponent::getPointInObject(V3D &point) const {
  // If the form of this component is not defined, throw NullPointerException
  if (!shape())
    throw Kernel::Exception::NullPointerException(
        "ObjComponent::getPointInObject", "shape");
  // Call the Object::getPointInObject method, which may give a point in Object
  // coordinates
  int result = shape()->getPointInObject(point);
  // transform point back to component space
  if (result) {
    // Scale up
    V3D scaleFactor = this->getScaleFactor();
    point *= scaleFactor;
    Quat Rotate = this->getRotation();
    Rotate.rotate(point);
    point += this->getPos();
  }
  return result;
}

/// Find the point that's in the same place relative to the constituent
/// geometrical Object
/// if the position and rotation introduced by the Component is ignored
const V3D ObjComponent::factorOutComponentPosition(const V3D &point) const {
  // First subtract the component's position, then undo the rotation
  return takeOutRotation(point - this->getPos());
}

/// Rotates a point by the reverse of the component's rotation
const V3D ObjComponent::takeOutRotation(V3D point) const {
  // Get the total rotation of this component and calculate the inverse (reverse
  // rotation)
  Quat unRotate = this->getRotation();
  unRotate.inverse();
  // Now rotate our point by the angle calculated above
  unRotate.rotate(point);

  // Can not Consider scaling factor here as this transform used by solidAngle
  // as well
  // as IsValid etc. While this would work for latter, breaks former

  return point;
}

/**
* Draws the objcomponent, If the handler is not set then this function does
* nothing.
*/
void ObjComponent::draw() const {
  if (Handle() == NULL)
    return;
  // Render the ObjComponent and then render the object
  Handle()->Render();
}

/**
* Draws the Object
*/
void ObjComponent::drawObject() const {
  if (shape() != NULL)
    shape()->draw();
}

/**
* Initializes the ObjComponent for rendering, this function should be called
* before rendering.
*/
void ObjComponent::initDraw() const {
  if (Handle() == NULL)
    return;
  // Render the ObjComponent and then render the object
  if (shape() != NULL)
    shape()->initDraw();
  Handle()->Initialize();
}

} // namespace Geometry
} // namespace Mantid
