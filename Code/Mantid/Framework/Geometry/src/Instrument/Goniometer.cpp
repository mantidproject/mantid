#include "MantidGeometry/Instrument/Goniometer.h"
#include <sstream>
#include <stdexcept>
#include <string>
#include "MantidKernel/Quat.h"
#include <vector>
#include <boost/algorithm/string.hpp>
#include <cstdlib>
#include "MantidKernel/Strings.h"

using namespace Mantid::Kernel;
using Mantid::Kernel::Strings::toString;

namespace Mantid {
namespace Geometry {
using Kernel::DblMatrix;
using Kernel::V3D;
using Kernel::Quat;

void GoniometerAxis::saveNexus(::NeXus::File *file,
                               const std::string &group) const {
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

/// Copy constructor
/// @param other :: Goniometer from which to copy information
Goniometer::Goniometer(const Goniometer &other)
    : R(other.R), motors(other.motors), initFromR(other.initFromR) {}

/// Constructor from a rotation matrix
/// @param rot :: DblMatrix matrix that is going to be the internal rotation
/// matrix of the goniometer. Cannot push additional axes
Goniometer::Goniometer(DblMatrix rot) {
  DblMatrix ide(3, 3), rtr(3, 3);
  rtr = rot.Tprime() * rot;
  ide.identityMatrix();
  if (rtr == ide) {
    R = rot;
    initFromR = true;
  } else
    throw std::invalid_argument("rot is not a rotation matrix");
}

/// Default destructor
Goniometer::~Goniometer() {}

/// Return global rotation matrix
/// @return R :: 3x3 rotation matrix
const Kernel::DblMatrix &Goniometer::getR() const { return R; }

/// Set the new rotation matrix
/// @param rot :: DblMatrix matrix that is going to be the internal rotation
/// matrix of the goniometer.
void Goniometer::setR(Kernel::DblMatrix rot) { R = rot; }

/// Function reports if the goniometer is defined
bool Goniometer::isDefined() const { return initFromR || (!motors.empty()); }

/// Return information about axes.
/// @return str :: string that contains on each line one motor information (axis
/// name, direction, sense, angle)
/// The angle units shown is degrees
std::string Goniometer::axesInfo() {
  if (initFromR == true) {
    return std::string("Goniometer was initialized from a rotation matrix. No "
                       "information about axis is available.\n");
  } else {
    std::stringstream info;
    std::vector<GoniometerAxis>::iterator it;
    std::string strCW("CW"), strCCW("CCW"), sense;

    if (motors.size() == 0) {
      info << "No axis is found\n";
    } else {
      info << "Name \t Direction \t Sense \t Angle \n";
      for (it = motors.begin(); it < motors.end(); ++it) {
        sense = ((*it).sense == CCW) ? strCCW : strCW;
        double angle = ((*it).angleunit == angDegrees)
                           ? ((*it).angle)
                           : ((*it).angle * rad2deg);
        info << (*it).name << "\t" << (*it).rotationaxis << "\t" << sense
             << "\t" << angle << std::endl;
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
void Goniometer::pushAxis(std::string name, double axisx, double axisy,
                          double axisz, double angle, int sense, int angUnit) {
  if (initFromR == true) {
    throw std::runtime_error(
        "Initialized from a rotation matrix, so no axes can be pushed.");
  } else {
    std::vector<GoniometerAxis>::iterator it;
    // check if such axis is already defined
    for (it = motors.begin(); it < motors.end(); ++it) {
      if (name.compare((*it).name) == 0)
        throw std::invalid_argument("Motor name already defined");
    }
    GoniometerAxis a(name, V3D(axisx, axisy, axisz), angle, sense, angUnit);
    motors.push_back(a);
  }
  recalculateR();
}

/** Set rotation angle for an axis using motor name
  @param name :: GoniometerAxis name
  @param value :: value in the units that the axis is set
*/
void Goniometer::setRotationAngle(std::string name, double value) {
  bool changed = false;
  std::vector<GoniometerAxis>::iterator it;
  for (it = motors.begin(); it < motors.end(); ++it) {
    if (name.compare((*it).name) == 0) {
      (*it).angle = value;
      changed = true;
    }
  }
  if (changed == false) {
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
    throw std::out_of_range(
        "Goniometer::setRotationAngle(): axis number specified is too large.");
  (motors.at(axisnumber)).angle = value; // it will throw out of range exception
                                         // if axisnumber is not in range
  recalculateR();
}

/// Get GoniometerAxis obfject using motor number
/// @param axisnumber :: axis number (from 0)
const GoniometerAxis &Goniometer::getAxis(size_t axisnumber) const {
  return motors.at(axisnumber); // it will throw out of range exception if
                                // axisnumber is not in range
}

/// Get GoniometerAxis object using motor name
/// @param axisname :: axis name
const GoniometerAxis &Goniometer::getAxis(std::string axisname) const {
  bool found = false;
  for (auto it = motors.begin(); it < motors.end(); ++it) {
    if (axisname.compare((*it).name) == 0) {
      found = true;
      return (*it);
    }
  }
  if (found == false) {
    throw std::invalid_argument("Motor name " + axisname + " not found");
  }
  return motors.at(0);
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
  this->pushAxis("omega", 0., 1., 0., 0., Mantid::Geometry::CCW,
                 Mantid::Geometry::angDegrees);
  this->pushAxis("chi", 0., 0., 1., 0., Mantid::Geometry::CCW,
                 Mantid::Geometry::angDegrees);
  this->pushAxis("phi", 0., 1., 0., 0., Mantid::Geometry::CCW,
                 Mantid::Geometry::angDegrees);
}

/** Return Euler angles acording to a convention
* @param convention :: the convention used to calculate Euler Angles. The
* UniversalGoniometer is YZY, a triple axis goniometer at HFIR is YZX
*/
std::vector<double> Goniometer::getEulerAngles(std::string convention) {
  return Quat(getR()).getEulerAngles(convention);
}

/// Private function to recalculate the rotation matrix of the goniometer
void Goniometer::recalculateR() {
  std::vector<GoniometerAxis>::iterator it;
  std::vector<double> elements;
  Quat QGlobal, QCurrent;

  for (it = motors.begin(); it < motors.end(); ++it) {
    double ang = (*it).angle;
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
void Goniometer::saveNexus(::NeXus::File *file,
                           const std::string &group) const {
  file->makeGroup(group, "NXpositioner", 1);
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
  for (int i = 0; i < num_axes; i++) {
    GoniometerAxis newAxis;
    newAxis.loadNexus(file, "axis" + Strings::toString(i));
    motors.push_back(newAxis);
  }
  file->closeGroup();
  // Refresh cached values
  recalculateR();
}

} // Namespace Geometry
} // Namespace Mantid
