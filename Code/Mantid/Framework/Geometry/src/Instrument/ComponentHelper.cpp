//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidGeometry/Instrument/ComponentHelper.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/IComponent.h"

#include <boost/lexical_cast.hpp>

namespace Mantid {
namespace Geometry {
namespace ComponentHelper {
using Kernel::V3D;
using Kernel::Quat;

/**
 * Add/modify an entry in the parameter map for the given component
 * to update its position. The component is const
 * as the move doesn't actually change the object
 * @param comp A reference to the component to move
 * @param pmap A reference to the ParameterMap that will hold the new position
 * @param pos The new position
 * @param positionType Defines how the given position should be interpreted @see
 * TransformType enumeration
 */
void moveComponent(const IComponent &comp, ParameterMap &pmap,
                   const Kernel::V3D &pos, const TransformType positionType) {
  //
  // This behaviour was copied from how MoveInstrumentComponent worked
  //

  // First set it to the new absolute position
  V3D newPos = pos;
  switch (positionType) {
  case Absolute: // Do nothing
    break;
  case Relative:
    newPos += comp.getPos();
    break;
  default:
    throw std::invalid_argument("moveComponent -  Unknown positionType: " +
                                boost::lexical_cast<std::string>(positionType));
  }

  // Then find the corresponding relative position
  auto parent = comp.getParent();
  if (parent) {
    newPos -= parent->getPos();
    Quat rot = parent->getRotation();
    rot.inverse();
    rot.rotate(newPos);
  }

  // Add a parameter for the new position
  pmap.addV3D(comp.getComponentID(), "pos", newPos);
}

/**
 * Add/modify an entry in the parameter map for the given component
 * to update its rotation. The component is const
 * as the move doesn't actually change the object
 * @param comp A reference to the component to move
 * @param pmap A reference to the ParameterMap that will hold the new position
 * @param rot The rotation quaternion
 * @param rotType Defines how the given rotation should be interpreted @see
 * TransformType enumeration
 */
void rotateComponent(const IComponent &comp, ParameterMap &pmap,
                     const Kernel::Quat &rot, const TransformType rotType) {
  //
  // This behaviour was copied from how RotateInstrumentComponent worked
  //

  Quat newRot = rot;
  if (rotType == Absolute) {
    // Find the corresponding relative position
    auto parent = comp.getParent();
    if (parent) {
      Quat rot0 = parent->getRelativeRot();
      rot0.inverse();
      newRot = rot * rot0;
    }
  } else if (rotType == Relative) {
    const Quat &Rot0 = comp.getRelativeRot();
    newRot = Rot0 * rot;
  } else {
    throw std::invalid_argument("rotateComponent -  Unknown rotType: " +
                                boost::lexical_cast<std::string>(rotType));
  }

  // Add a parameter for the new rotation
  pmap.addQuat(comp.getComponentID(), "rot", newRot);
}

} // namespace ComponentHelper
}
} // namespace Mantid::Geometry
