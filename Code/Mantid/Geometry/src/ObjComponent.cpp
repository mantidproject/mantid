//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidGeometry/ObjComponent.h"
#include "MantidGeometry/Object.h"
#include "MantidKernel/Exception.h"

namespace Mantid
{
namespace Geometry
{

/** Constructor
 *  @param name   The name of the component
 *  @param parent The Parent geometry object of this component
 */
ObjComponent::ObjComponent(const std::string& name, Component* parent) :
  Component(name,parent), shape()
{
}

/** Constructor
 *  @param name   The name of the component
 *  @param shape  A pointer to the object describing the shape of this component
 *  @param parent The Parent geometry object of this component
 */
ObjComponent::ObjComponent(const std::string& name, boost::shared_ptr<Object> shape, Component* parent) :
  Component(name,parent), shape(shape)
{
}

/// Copy constructor
ObjComponent::ObjComponent(const ObjComponent& rhs) :
  Component(rhs), shape(rhs.shape)
{
}

/// Destructor
ObjComponent::~ObjComponent()
{
}

/// Does the point given lie within this object component?
bool ObjComponent::isValid(const V3D& point) const
{
  // If the form of this component is not defined, just treat as a point
  if (!shape) return (this->getPos() == point);
  // Otherwise pass through the shifted point to the Object::isValid method
  return shape->isValid( factorOutComponentPosition(point) );
}

/// Does the point given lie on the surface of this object component?
bool ObjComponent::isOnSide(const V3D& point) const
{
  // If the form of this component is not defined, just treat as a point
  if (!shape) return (this->getPos() == point);
  // Otherwise pass through the shifted point to the Object::isOnSide method
  return shape->isOnSide( factorOutComponentPosition(point) );
}

/** Checks whether the track given will pass through this Component.
 *  @param track The Track object to test (N.B. Will be modified if hits are found)
 *  @returns The number of track segments added (i.e. 1 if the track enters and exits the object once each)
 *  @throw NullPointerException if the underlying geometrical Object has not been set
 */
int ObjComponent::interceptSurface(Track& track) const
{
  // If the form of this component is not defined, throw NullPointerException
  if (!shape) throw Kernel::Exception::NullPointerException("ObjComponent::interceptSurface","shape");

  V3D trkStart = factorOutComponentPosition(track.getInit());
  V3D trkDirection = takeOutRotation(track.getUVec());

  Track probeTrack(trkStart, trkDirection);
  int intercepts = shape->interceptSurface(probeTrack);

  Track::LType::const_iterator it;
  for (it = probeTrack.begin(); it < probeTrack.end(); ++it)
  {
    V3D in = it->PtA;
    this->getRotation().rotate(in);
    in += this->getPos();
    V3D out = it->PtB;
    this->getRotation().rotate(out);
    out += this->getPos();
    track.addTUnit(shape->getName(),in,out,it->Dist);
  }

  return intercepts;
}

/** Finds the approximate solid angle covered by the component when viewed from the point given
 *  @param observer The position from which the component is being viewed
 *  @returns The solid angle in steradians
 *  @throw NullPointerException if the underlying geometrical Object has not been set
 */
double ObjComponent::solidAngle(const V3D& observer)
{
  // If the form of this component is not defined, throw NullPointerException
  if (!shape) throw Kernel::Exception::NullPointerException("ObjComponent::solidAngle","shape");
  // Otherwise pass through the shifted point to the Object::solidAngle method
  return shape->solidAngle( factorOutComponentPosition(observer) );
}

/// Find the point that's in the same place relative to the constituent geometrical Object
/// if the position and rotation introduced by the Component is ignored
const V3D ObjComponent::factorOutComponentPosition(const V3D& point) const
{
  // First subtract the component's position, then undo the rotation
  return takeOutRotation( point - this->getPos() );
}

/// Rotates a point by the reverse of the component's rotation
const V3D ObjComponent::takeOutRotation(V3D point) const
{
  // Get the total rotation of this component and calculate the inverse (reverse rotation)
  Quat unRotate = this->getRotation();
  unRotate.inverse();
  // Now rotate our point by the angle calculated above
  unRotate.rotate(point);

  return point;
}

} // namespace Geometry
} // namespace Mantid
