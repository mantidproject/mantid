// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/Strings.h"
#include <boost/algorithm/string.hpp>
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include <vector>

using namespace Mantid::Kernel;
using Mantid::Kernel::Strings::toString;

namespace Mantid::Geometry {
using Kernel::DblMatrix;
using Kernel::Quat;
using Kernel::V3D;

Mantid::Kernel::Logger g_log("Goniometer");

void GoniometerAxis::saveNexus(::NeXus::File *file, const std::string &group) const {
  file->makeGroup(group, "NXmotor", true);
  file->writeData("name", name);
  file->writeData("angle", angle);
  file->openData("angle");
  std::string unit = (angleunit == angDegrees) ? "deg" : "rad";
  file->putAttr("unit", unit);
  std::string sen = (sense == CW) ? "CW" : "CCW";
  file->putAttr("sense", sen);
  file->closeData();
  rotationaxis.saveNexus(file, "rotationaxis");
  file->closeGroup();
}

void GoniometerAxis::loadNexus(::NeXus::File *file, const std::string &group) {
  file->openGroup(group, "NXmotor");
  file->readData("name", name);
  file->readData("angle", angle);
  file->openData("angle");
  std::string s;
  file->getAttr("sense", s);
  sense = (s == "CW") ? CW : CCW;
  file->getAttr("unit", s);
  angleunit = (s == "rad") ? angRadians : angDegrees;
  file->closeData();
  rotationaxis.loadNexus(file, "rotationaxis");
  file->closeGroup();
}

/// Default constructor
/// The rotation matrix is initialized to identity
Goniometer::Goniometer() : R(3, 3, true), initFromR(false) {}

/// Constructor from a rotation matrix
/// @param rot :: DblMatrix matrix that is going to be the internal rotation
/// matrix of the goniometer. Cannot push additional axes
Goniometer::Goniometer(const DblMatrix &rot) {
  DblMatrix ide(3, 3), rtr(3, 3);
  rtr = rot.Tprime() * rot;
  ide.identityMatrix();
  if (rtr == ide) {
    R = rot;
    initFromR = true;
  } else
    throw std::invalid_argument("rot is not a rotation matrix");
}

/// Return global rotation matrix
/// @return R :: 3x3 rotation matrix
const Kernel::DblMatrix &Goniometer::getR() const { return R; }

/// Set the new rotation matrix
/// @param rot :: DblMatrix matrix that is going to be the internal rotation
/// matrix of the goniometer.
void Goniometer::setR(Kernel::DblMatrix rot) {
  R = std::move(rot);
  initFromR = true;
}

/// Function reports if the goniometer is defined
bool Goniometer::isDefined() const { return initFromR || (!motors.empty()); }

bool Goniometer::operator==(const Goniometer &other) const { return this->R == other.R; }

bool Goniometer::operator!=(const Goniometer &other) const { return this->R != other.R; }

/// Return information about axes.
/// @return str :: string that contains on each line one motor information (axis
/// name, direction, sense, angle)
/// The angle units shown is degrees
std::string Goniometer::axesInfo() {
  if (initFromR) {
    return std::string("Goniometer was initialized from a rotation matrix. No "
                       "information about axis is available.\n");
  } else {
    std::stringstream info;
    std::vector<GoniometerAxis>::iterator it;

    if (motors.empty()) {
      info << "No axis is found\n";
    } else {
      info << "Name \t Direction \t Sense \t Angle \n";
      std::string strCW("CW"), strCCW("CCW");
      for (it = motors.begin(); it < motors.end(); ++it) {
        std::string sense = ((*it).sense == CCW) ? strCCW : strCW;
        double angle = ((*it).angleunit == angDegrees) ? ((*it).angle) : ((*it).angle * rad2deg);
        info << (*it).name << "\t" << (*it).rotationaxis << "\t" << sense << "\t" << angle << '\n';
      }
    }
    return info.str();
  }
}

/**Add an additional axis to the goniometer, closer to the sample
  @param name :: GoniometerAxis name
  @param axisx :: the x component of the rotation axis
  @param axisy :: the y component of the rotation axis
  @param axisz :: the z component of the rotation axis
  @param angle :: rotation angle, 0 by default
  @param sense :: rotation sense (CW or CCW), CCW by default
  @param angUnit :: units for angle of type#AngleUnit, angDegrees by default
*/
void Goniometer::pushAxis(const std::string &name, double axisx, double axisy, double axisz, double angle, int sense,
                          int angUnit) {
  if (initFromR) {
    throw std::runtime_error("Initialized from a rotation matrix, so no axes can be pushed.");
  } else {
    if (!std::isfinite(axisx) || !std::isfinite(axisy) || !std::isfinite(axisz) || !std::isfinite(angle)) {
      g_log.warning() << "NaN encountered while trying to push axis to "
                         "goniometer, Operation aborted"
                      << "\naxis name" << name << "\naxisx" << axisx << "\naxisy" << axisx << "\naxisz" << axisz
                      << "\nangle" << angle;
      return;
    }
    // check if such axis is already defined
    const auto it =
        std::find_if(motors.cbegin(), motors.cend(), [&name](const auto &axis) { return axis.name == name; });
    if (it != motors.cend()) {
      throw std::invalid_argument("Motor name already defined");
    }
    GoniometerAxis a(name, V3D(axisx, axisy, axisz), angle, sense, angUnit);
    motors.emplace_back(a);
  }
  recalculateR();
}

/** Set rotation angle for an axis using motor name
  @param name :: GoniometerAxis name
  @param value :: value in the units that the axis is set
*/
void Goniometer::setRotationAngle(const std::string &name, double value) {
  bool changed = false;
  std::vector<GoniometerAxis>::iterator it;
  for (it = motors.begin(); it < motors.end(); ++it) {
    if (name == it->name) {
      it->angle = value;
      changed = true;
    }
  }
  if (!changed) {
    throw std::invalid_argument("Motor name " + name + " not found");
  }
  recalculateR();
}

/**Set rotation angle for an axis using motor name
  @param axisnumber :: GoniometerAxis number (from 0)
  @param value :: value in the units that the axis is set
*/
void Goniometer::setRotationAngle(size_t axisnumber, double value) {
  if (axisnumber >= motors.size())
    throw std::out_of_range("Goniometer::setRotationAngle(): axis number specified is too large.");
  (motors.at(axisnumber)).angle = value; // it will throw out of range exception
                                         // if axisnumber is not in range
  recalculateR();
}

/**Calculate goniometer for rotation around y-asix for constant wavelength from
 * Q Sample
 * @param position :: Q Sample position in reciprocal space
 * @param wavelength :: wavelength
 * @param flip_x :: option if the q_lab x value should be negative, hence the
 * detector of the other side of the beam
 * @param inner :: whether the goniometer is the most inner (phi) or most outer
 * (omega)
 */
void Goniometer::calcFromQSampleAndWavelength(const Mantid::Kernel::V3D &position, double wavelength, bool flip_x,
                                              bool inner) {
  V3D Q(position);
  if (Kernel::ConfigService::Instance().getString("Q.convention") == "Crystallography")
    Q *= -1;

  Matrix<double> starting_goniometer = getR();

  if (!inner)
    Q = starting_goniometer * Q;

  double wv = 2.0 * M_PI / wavelength;
  double norm_q2 = Q.norm2();
  double theta = acos(1 - norm_q2 / (2 * wv * wv)); // [0, pi]
  double phi = asin(-Q[1] / (wv * sin(theta)));     // [-pi/2, pi/2]
  V3D Q_lab(-wv * sin(theta) * cos(phi), -wv * sin(theta) * sin(phi), wv * (1 - cos(theta)));

  if (flip_x)
    Q_lab[0] *= -1;

  if (inner) {
    starting_goniometer.Invert();
    Q_lab = starting_goniometer * Q_lab;
  }

  // Solve to find rotation matrix, assuming only rotation around y-axis
  // A * X = B
  Matrix<double> A({Q[0], Q[2], Q[2], -Q[0]}, 2, 2);
  A.Invert();
  std::vector<double> B{Q_lab[0], Q_lab[2]};
  std::vector<double> X = A * B;
  double rot = atan2(X[1], X[0]);

  Matrix<double> goniometer(3, 3, true);
  goniometer[0][0] = cos(rot);
  goniometer[0][2] = sin(rot);
  goniometer[2][0] = -sin(rot);
  goniometer[2][2] = cos(rot);
  if (inner) {
    starting_goniometer.Invert();
    setR(starting_goniometer * goniometer);
  } else {
    setR(goniometer * starting_goniometer);
  }
}

/// Get GoniometerAxis obfject using motor number
/// @param axisnumber :: axis number (from 0)
const GoniometerAxis &Goniometer::getAxis(size_t axisnumber) const {
  return motors.at(axisnumber); // it will throw out of range exception if
                                // axisnumber is not in range
}

/// Get GoniometerAxis object using motor name
/// @param axisname :: axis name
const GoniometerAxis &Goniometer::getAxis(const std::string &axisname) const {
  const auto it =
      std::find_if(motors.cbegin(), motors.cend(), [&axisname](const auto &axis) { return axis.name == axisname; });
  if (it != motors.cend()) {
    return *it;
  }
  throw std::invalid_argument("Motor name " + axisname + " not found");
}

/// @return the number of axes
size_t Goniometer::getNumberAxes() const { return motors.size(); }

/** Make a default universal goniometer with phi,chi,omega angles
 * according to SNS convention.
 * The rotations occur in this order:
 *  1. Closest to sample is phi, around the +Y (vertical) axis
 *  2. Chi, around the +Z (beam direction) axis
 *  3. Omega, around the +Y (vertical) axis
 */
void Goniometer::makeUniversalGoniometer() {
  motors.clear();
  this->pushAxis("omega", 0., 1., 0., 0., Mantid::Geometry::CCW, Mantid::Geometry::angDegrees);
  this->pushAxis("chi", 0., 0., 1., 0., Mantid::Geometry::CCW, Mantid::Geometry::angDegrees);
  this->pushAxis("phi", 0., 1., 0., 0., Mantid::Geometry::CCW, Mantid::Geometry::angDegrees);
}

/** Return Euler angles acording to a convention
 * @param convention :: the convention used to calculate Euler Angles. The
 * UniversalGoniometer is YZY, a triple axis goniometer at HFIR is YZX
 */
std::vector<double> Goniometer::getEulerAngles(const std::string &convention) {
  return Quat(getR()).getEulerAngles(convention);
}

/** return the goniometer convention used determine from the motor axes
 *
 * @returns string with the convention
 */
std::string Goniometer::getConventionFromMotorAxes() const {
  if (motors.size() != 3) {
    g_log.warning() << "Goniometerdoes not have 3 axes. Cannot determine "
                       "convention.\n";
    return "";
  }

  std::string convention;
  for (const auto &motor : motors) {
    switch (std::abs(motor.rotationaxis.masterDir())) {
    case 1:
      convention += "X";
      break;
    case 2:
      convention += "Y";
      break;
    case 3:
      convention += "Z";
      break;
    default:
      g_log.warning() << "Goniometer has an axis with a non-standard "
                         "rotation axis.\n";
      return "";
    }
  }
  return convention;
}

/// Private function to recalculate the rotation matrix of the goniometer
void Goniometer::recalculateR() {
  if (initFromR) {
    g_log.warning() << "Goniometer was initialized from a rotation matrix. No "
                    << "recalculation from motors will be done.\n";
    return;
  }
  std::vector<GoniometerAxis>::iterator it;
  std::vector<double> elements;
  Quat QGlobal, QCurrent;

  for (it = motors.begin(); it < motors.end(); ++it) {
    double ang = (*it).angle * (*it).sense;
    if ((*it).angleunit == angRadians)
      ang *= rad2deg;
    QCurrent = Quat(ang, (*it).rotationaxis);
    QGlobal *= QCurrent;
  }
  elements = QGlobal.getRotation();
  R = DblMatrix(elements);
}

//--------------------------------------------------------------------------------------------
/** Save the object to an open NeXus file.
 * @param file :: open NeXus file
 * @param group :: name of the group to create
 */
void Goniometer::saveNexus(::NeXus::File *file, const std::string &group) const {
  file->makeGroup(group, "NXpositioner", true);
  file->putAttr("version", 1);
  // Because the order of the axes is very important, they have to be written
  // and read out in the same order
  file->writeData("num_axes", int(motors.size()));
  for (size_t i = 0; i < motors.size(); i++)
    motors[i].saveNexus(file, "axis" + Strings::toString(i));
  file->closeGroup();
}

//--------------------------------------------------------------------------------------------
/** Load the object from an open NeXus file.
 * @param file :: open NeXus file
 * @param group :: name of the group to open
 */
void Goniometer::loadNexus(::NeXus::File *file, const std::string &group) {
  file->openGroup(group, "NXpositioner");
  int num_axes;
  file->readData("num_axes", num_axes);
  motors.clear();
  motors.reserve(num_axes);
  for (int i = 0; i < num_axes; i++) {
    GoniometerAxis newAxis;
    newAxis.loadNexus(file, "axis" + Strings::toString(i));
    motors.emplace_back(newAxis);
  }
  file->closeGroup();
  // Refresh cached values
  recalculateR();
}

} // namespace Mantid::Geometry
