// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/System.h"
#include "MantidKernel/V3D.h"
#include <string>

namespace Mantid {
namespace Geometry {
/// Type to describe pointing along options
enum PointingAlong { X = 0, Y = 1, Z = 2 };
/// Type to distingusih between l and r handedness
enum Handedness { Left, Right };

/** ReferenceFrame : Holds reference frame information from the geometry
  description file.

  @date 2012-01-27
*/

class DLLExport ReferenceFrame {
public:
  /// Default constructor
  ReferenceFrame();
  /// Constructor
  ReferenceFrame(PointingAlong up, PointingAlong alongBeam, Handedness handedness, std::string origin);
  /// Alternative constructor with theta sign axis
  ReferenceFrame(PointingAlong up, PointingAlong alongBeam, PointingAlong thetaSign, Handedness handedness,
                 std::string origin);
  /// Gets the pointing up direction
  PointingAlong pointingUp() const;
  /// Gets the beam pointing along direction
  PointingAlong pointingAlongBeam() const;
  /// Gets the pointing horizontal direction, i.e perpendicular to up & along
  /// beam
  PointingAlong pointingHorizontal() const;
  /// Gets the handedness
  Handedness getHandedness() const;
  /// Gets the origin
  std::string origin() const;
  /// Destructor
  virtual ~ReferenceFrame() = default;
  /// Convert up axis into a 3D direction
  Mantid::Kernel::V3D vecPointingUp() const;
  /// Convert along beam axis into a 3D direction
  Mantid::Kernel::V3D vecPointingAlongBeam() const;
  /// Convert along horizontal axis into a 3D direction
  Mantid::Kernel::V3D vecPointingHorizontal() const;
  /// Convert along the axis defining the 2theta sign
  Mantid::Kernel::V3D vecThetaSign() const;
  /// Pointing up axis as a string
  std::string pointingUpAxis() const;
  /// Pointing along beam axis as a string
  std::string pointingAlongBeamAxis() const;
  /// Pointing horizontal to beam as a string
  std::string pointingHorizontalAxis() const;
  /// Test whether or not a vector is in the beam direction
  bool isVectorPointingAlongBeam(const Mantid::Kernel::V3D &v) const;

private:
  /// Disabled assignment
  ReferenceFrame &operator=(const ReferenceFrame &);
  /// Pointing up axis
  PointingAlong m_up;
  /// Beam pointing along axis
  PointingAlong m_alongBeam;
  /// Axis defining the 2theta sign
  PointingAlong m_thetaSign;
  /// Handedness
  Handedness m_handedness;
  /// Origin
  std::string m_origin;
  /// Vector pointing along the beam
  Mantid::Kernel::V3D m_vecPointingAlongBeam;
  /// Vector pointing up instrument
  Mantid::Kernel::V3D m_vecPointingUp;
  /// Vector denoting the direction defining the 2theta sign
  Mantid::Kernel::V3D m_vecThetaSign;
};

} // namespace Geometry
} // namespace Mantid
