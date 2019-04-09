// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include <stdexcept>

using namespace Mantid::Kernel;

namespace Mantid {
namespace Geometry {

namespace {
/**
 * Non-member helper method to convert x y z enumeration directions into proper
 * 3D vectors.
 * @param direction : direction marker
 * @return 3D vector
 */
V3D directionToVector(const PointingAlong &direction) {
  V3D result;
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
} // namespace

/**
 * Default constructor. up=Y, beam=Z, thetaSign=Y
 */
ReferenceFrame::ReferenceFrame() : ReferenceFrame(Y, Z, Y, Right, "source") {}

/** Constructor specifying thetaSign=up
 * @param up :pointing up axis
 * @param alongBeam : axis pointing along the beam
 * @param handedness : Handedness
 * @param origin : origin
 */
ReferenceFrame::ReferenceFrame(PointingAlong up, PointingAlong alongBeam,
                               Handedness handedness, std::string origin)
    : ReferenceFrame(up, alongBeam, up, handedness, std::move(origin)) {}

/**
 * Constructor specifying all attributes
 * @param up : pointing up axis
 * @param alongBeam : axis pointing along the beam
 * @param thetaSign : axis defining the sign of 2theta
 * @param handedness : Handedness
 * @param origin : origin
 */
ReferenceFrame::ReferenceFrame(PointingAlong up, PointingAlong alongBeam,
                               PointingAlong thetaSign, Handedness handedness,
                               std::string origin)
    : m_up(up), m_alongBeam(alongBeam), m_thetaSign(thetaSign),
      m_handedness(handedness), m_origin(std::move(origin)) {
  if (up == alongBeam) {
    throw std::invalid_argument(
        "Cannot have up direction the same as the beam direction");
  }
  if (thetaSign == alongBeam) {
    throw std::invalid_argument(
        "Scattering angle sign axis cannot be the same as the beam direction");
  }
  m_vecPointingUp = directionToVector(m_up);
  m_vecPointingAlongBeam = directionToVector(m_alongBeam);
  m_vecThetaSign = directionToVector(m_thetaSign);
}

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
V3D ReferenceFrame::vecPointingUp() const { return m_vecPointingUp; }

/**
Getter for the direction defining the theta sign
@return theta sign direction.
*/
V3D ReferenceFrame::vecThetaSign() const { return m_vecThetaSign; }

/**
Getter for the along beam vector.
@return along beam direction.
*/
V3D ReferenceFrame::vecPointingAlongBeam() const {
  return m_vecPointingAlongBeam;
}

/**
Calculate the horizontal vector.
@return horizontal direction.
*/
V3D ReferenceFrame::vecPointingHorizontal() const {
  return directionToVector(pointingHorizontal());
}

/**
Convenience method for checking whether or not a vector is aligned
with the along beam vector.
@param v vector to be tested against along beam vector
@return result of whther the along beam and test vector are parallel.
*/
bool ReferenceFrame::isVectorPointingAlongBeam(const V3D &v) const {
  // Normalized (unit) parallel vectors should produce a scalar product of 1
  return m_vecPointingAlongBeam.scalar_prod(normalize(v)) == 1;
}

} // namespace Geometry
} // namespace Mantid
