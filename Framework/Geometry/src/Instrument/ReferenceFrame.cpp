#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include <stdexcept>

using namespace Mantid::Kernel;

namespace Mantid {
namespace Geometry {

//----------------------------------------------------------------------------------------------
/** Constructor
@param up : pointing up axis
@param alongBeam : axis pointing along the beam
@param handedness : Handedness
@param origin : origin
*/
ReferenceFrame::ReferenceFrame(PointingAlong up, PointingAlong alongBeam,
                               Handedness handedness, std::string origin)
    : m_up(up), m_alongBeam(alongBeam), m_handedness(handedness),
      m_origin(origin) {
  if (up == alongBeam) {
    throw std::invalid_argument(
        "Cannot have up direction the same as the beam direction");
  }
  init();
}

//----------------------------------------------------------------------------------------------
/** Constructor
Default constructor
*/
ReferenceFrame::ReferenceFrame()
    : m_up(Y), m_alongBeam(Z), m_handedness(Right), m_origin("source") {
  init();
}

/**
Non-member helper method to convert x y z enumeration directions into proper
3D vectors.
@param direction : direction marker
@return 3D vector
*/
V3D directionToVector(const PointingAlong &direction) {
  Mantid::Kernel::V3D result;
  if (direction == X) {
    result = V3D(1, 0, 0);
  } else if (direction == Y) {
    result = V3D(0, 1, 0);
  } else {
    result = V3D(0, 0, 1);
  }
  return result;
}

/**
 * Non-member helper method to convert a direction enum to a string label.
 * @param direction
 * @return label
 */
std::string directionToString(const PointingAlong &direction) {
  std::string result;
  if (direction == X) {
    result = "X";
  } else if (direction == Y) {
    result = "Y";
  } else {
    result = "Z";
  }
  return result;
}

/// Perform common initalisation steps.
void ReferenceFrame::init() {
  m_vecPointingUp = directionToVector(m_up);
  m_vecPointingAlongBeam = directionToVector(m_alongBeam);
}

//----------------------------------------------------------------------------------------------
/** Destructor
*/
ReferenceFrame::~ReferenceFrame() {}

/**
Gets the pointing up direction
@return axis
*/
PointingAlong ReferenceFrame::pointingUp() const { return m_up; }

/** Gets the beam pointing along direction
@return axis
*/
PointingAlong ReferenceFrame::pointingAlongBeam() const { return m_alongBeam; }

/**
* Get the axis label for the pointing up direction.
* @return label for up
*/
std::string ReferenceFrame::pointingUpAxis() const {
  return directionToString(m_up);
}

/**
* Get the axis label for the pointing along direction.
* @return label for up
*/
std::string ReferenceFrame::pointingAlongBeamAxis() const {
  return directionToString(m_alongBeam);
}

/**
 * Get the axis label for the pointing horizontal direction.
 * @return Axis label for the axis that is perpendicular to the beam and up
 * direction in the instrument
 */
std::string ReferenceFrame::pointingHorizontalAxis() const {
  return directionToString(pointingHorizontal());
}

/**
 * Gets the pointing horizontal direction, i.e perpendicular to up & along beam
 * @return Axis that is perpendicular to Up & beam direction
 */
PointingAlong ReferenceFrame::pointingHorizontal() const {
  if (m_up == X) {
    if (m_alongBeam == Y)
      return Z;
    else
      return Y;
  } else if (m_up == Y) {
    if (m_alongBeam == Z)
      return X;
    else
      return Z;
  } else {
    if (m_alongBeam == Y)
      return X;
    else
      return Y;
  }
}

/** Gets the handedness
@return handedness
*/
Handedness ReferenceFrame::getHandedness() const { return m_handedness; }

/** Gets the origin
@return origin
*/
std::string ReferenceFrame::origin() const { return m_origin; }

/**
Getter for the up instrument direction
@return up direction.
*/
const V3D ReferenceFrame::vecPointingUp() const { return m_vecPointingUp; }

/**
Getter for the along beam vector.
@return along beam direction.
*/
const V3D ReferenceFrame::vecPointingAlongBeam() const {
  return m_vecPointingAlongBeam;
}

} // namespace Mantid
} // namespace Geometry
