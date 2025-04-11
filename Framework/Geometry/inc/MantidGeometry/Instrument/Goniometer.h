// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Crystal/AngleUnits.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/V3D.h"
#include "MantidNexus/NeXusFile.hpp"
#include <string>
#include <utility>

namespace Mantid {
namespace Geometry {

/**
  Class to represent a particular goniometer setting, which is described by the
  rotation matrix.
  For a particular sample environment, it stores the rotation motors (names,
  angles, ...). If copied from
  one workspace to another, one might need to just change the angle value for
  one particular axis.

  @class Goniometer
  @author Andrei Savici, SNS, ORNL
  @date 04/13/2011
 */

enum RotationSense {
  CW = -1, /// Clockwise rotation
  CCW = 1
}; /// Counter clockwise rotation

/// Structure containing a rotation axis: name, direction, angle, sense
struct GoniometerAxis {
  std::string name;         /// GoniometerAxis name
  Kernel::V3D rotationaxis; /// GoniometerAxis direction
  double angle;             /// Rotation angle
  int sense;                /// Rotation sense (1 for CCW, -1 for CW)
  int angleunit;            /// angle units are angDegrees or angRadians (see AngleUnits.h)
  /// Constructor
  GoniometerAxis(std::string initname, const Kernel::V3D &initrotationaxis, double initangle, int initsense,
                 int initangleunit)
      : name(std::move(initname)), rotationaxis(initrotationaxis), angle(initangle), sense(initsense),
        angleunit(initangleunit) {}
  GoniometerAxis() : name(""), rotationaxis(), angle(0.), sense(0), angleunit(0) {}

  void saveNexus(::NeXus::File *file, const std::string &group) const;
  void loadNexus(::NeXus::File *file, const std::string &group);
};

class MANTID_GEOMETRY_DLL Goniometer {
public:
  // Default constructor
  Goniometer();
  // Constructor from a rotation matrix
  Goniometer(const Kernel::DblMatrix &rot);
  // Default destructor
  virtual ~Goniometer() = default;
  // Return rotation matrix
  const Kernel::DblMatrix &getR() const;
  // Set the rotation matrix
  void setR(Kernel::DblMatrix rot);
  // Return information about axes
  std::string axesInfo();
  // Add axis to goniometer
  void pushAxis(const std::string &name, double axisx, double axisy, double axisz, double angle = 0., int sense = CCW,
                int angUnit = angDegrees);
  // Set rotation angle for an axis in the units the angle is set (default --
  // degrees)
  void setRotationAngle(const std::string &name, double value);
  // Set rotation angle for an axis in the units the angle is set (default --
  // degrees)
  void setRotationAngle(size_t axisnumber, double value);
  // Calculate goniometer for rotation around y-axis for constant wavelength
  // from Q Sample
  void calcFromQSampleAndWavelength(const Mantid::Kernel::V3D &Q, double wavelength, bool flip_x = false,
                                    bool inner = false);
  // Get axis object
  const GoniometerAxis &getAxis(size_t axisnumber) const;
  // Get axis object
  const GoniometerAxis &getAxis(const std::string &axisname) const;
  // Return the number of axes
  size_t getNumberAxes() const;
  // Make a default universal goniometer
  void makeUniversalGoniometer();
  // Return Euler angles acording to a convention
  std::vector<double> getEulerAngles(const std::string &convention = "YZX");

  // determine the convention from the motor axes
  std::string getConventionFromMotorAxes() const;

  void saveNexus(::NeXus::File *file, const std::string &group) const;
  void loadNexus(::NeXus::File *file, const std::string &group);
  /// the method reports if the goniometer was defined with some parameters
  bool isDefined() const;
  bool operator==(const Goniometer &other) const;
  bool operator!=(const Goniometer &other) const;

private:
  /// Global rotation matrix of the goniometer
  Kernel::DblMatrix R;
  /// Motors vector contains GoniometerAxis objects, the last one is the closest
  /// to the sample
  std::vector<GoniometerAxis> motors;
  /// Flag to specify if the goniometer is initialized from a rotation matrix
  bool initFromR;
  /// Private function to recalculate R when setRotationAngle is called
  void recalculateR();
};
} // namespace Geometry
} // namespace Mantid
