#include "MantidGeometry/Instrument/Goniometer.h"
#include <sstream>
#include <stdexcept>
#include <string>
#include "MantidGeometry/Quat.h" 
#include <vector>

namespace Mantid
{
namespace Geometry
{

/// Default constructor
/// The rotation matrix is initialized to identity
Goniometer::Goniometer():R(3,3),initFromR(false)
{
  R[0][0]=1.;
  R[1][1]=1.;
  R[2][2]=1.;
}

/// Copy constructor
/// @param other :: Goniometer from which to copy information
Goniometer::Goniometer(const Goniometer& other):R(other.R),motors(other.motors),initFromR(other.initFromR)
{
}

/// Constructor from a rotation matrix
/// @param rot :: #MantidMat matrix that is going to be the internal rotation matrix of the goniometer. Cannot push additional axes
Goniometer::Goniometer(MantidMat rot)
{
  MantidMat ide(3,3),rtr(3,3);
  rtr=rot.Tprime()*rot;
  ide.identityMatrix();
  if (rtr==ide)
  {
    R=rot;
    initFromR=true;
  }
  else
    throw std::invalid_argument("rot is not a rotation matrix");
}

/// Default destructor
Goniometer::~Goniometer()
{
}

/// Return global rotation matrix
/// @return R :: 3x3 rotation matrix 
const Geometry::MantidMat& Goniometer::getR() const
{
  return R;
}

/// Return information about axes. 
/// @return str :: string that contains on each line one motor information (axis name, direction, sense, angle)  
/// The angle units shown is degrees
std::string Goniometer::axesInfo()
{
  if (initFromR==true) 
  {
    return std::string("Goniometer was initialized from a rotation matrix. No information about axis is available.\n");
  }
  else
  {
    std::stringstream info;
    std::vector<GoniometerAxis>::iterator it;
    std::string strCW("CW"),strCCW("CCW"),sense;
    double angle;

    if (motors.size() == 0) 
    {
      info<<"No axis is found\n";
    }
    else
    {
      info<<"Name \t Direction \t Sense \t Angle \n";
      for(it=motors.begin(); it<motors.end(); it++)
      {
        sense=((*it).sense==CCW)?strCCW:strCW;
        angle=((*it).angleunit==angDegrees)?((*it).angle): ((*it).angle*rad2deg); 
        info<<(*it).name<<"\t"<<(*it).rotationaxis<<"\t"<<sense<<"\t"<<angle<<std::endl;
      }  
    }
    return info.str();
  }
}

/**Add an additional axis to the goniometer, closer to the sample
  @param name :: GoniometerAxis name
  @param axisx, axisy axisz :: the x, y, z components of the rotation axis
  @param angle :: rotation angle, 0 by default
  @param sense :: rotation sense (CW or CCW), CCW by default
  @param angUnit :: units for angle of type#AngleUnit, angDegrees by default 
*/
void Goniometer::pushAxis(std::string name, double axisx, double axisy, double axisz, double angle, int sense, int angUnit)
{
  if (initFromR==true) 
  {
    throw std::runtime_error("Initialized from a rotation matrix, so no axes can be pushed.");
  }
  else
  {
    std::vector<GoniometerAxis>::iterator it;
    // check if such axis is already defined
    for(it=motors.begin(); it<motors.end(); it++)
    {
      if(name.compare((*it).name)==0) throw std::invalid_argument("Motor name already defined");
    }
    GoniometerAxis a(name,V3D(axisx,axisy,axisz),angle,sense,angUnit);
    motors.push_back(a);
  }
  recalculateR();
}

/** Set rotation angle for an axis using motor name
  @param name :: GoniometerAxis name
  @param value :: value in the units that the axis is set
*/
void Goniometer::setRotationAngle( std::string name, double value)
{
  bool changed=false;
  std::vector<GoniometerAxis>::iterator it;
  for(it=motors.begin(); it<motors.end(); it++)
  {
    if(name.compare((*it).name)==0)
    {
      (*it).angle=value;
      changed=true;
    }
  }
  if(changed==false)
  {
    throw std::invalid_argument("Motor name "+name+" not found");
  }
  recalculateR();
}

/**Set rotation angle for an axis using motor name
  @param axisnumber :: GoniometerAxis number (from 0)
  @param value :: value in the units that the axis is set
*/
void Goniometer::setRotationAngle( size_t axisnumber, double value)
{
  (motors.at(axisnumber)).angle=value;//it will throw out of range exception if axisnumber is not in range
  recalculateR();
}

/// Get GoniometerAxis obfject using motor number
/// @param axisnumber :: axis number (from 0)
GoniometerAxis Goniometer::getAxis( size_t axisnumber)
{
  return motors.at(axisnumber);//it will throw out of range exception if axisnumber is not in range
}
/// Get GoniometerAxis obfject using motor number
/// @param axisnumber :: axis number (from 0)
const GoniometerAxis Goniometer::getAxis( size_t axisnumber) const
{
  return motors.at(axisnumber);//it will throw out of range exception if axisnumber is not in range
}

/// Get GoniometerAxis obfject using motor name
/// @param axisname :: axis name
GoniometerAxis Goniometer::getAxis( std::string axisname)
{
  bool found=false;
  std::vector<GoniometerAxis>::iterator it;
  for(it=motors.begin(); it<motors.end(); it++)
  {
    if(axisname.compare((*it).name)==0)
    {
      return (*it);
      found=true;
    }
  }
  if(found==false)
  {
    throw std::invalid_argument("Motor name "+axisname+" not found");
  }  
  return motors.at(0);
}

/// @return the number of axes
size_t Goniometer::getNumberAxes() const
{
  return motors.size();
}

/** Make a default universal goniometer with phi,chi,omega angles
 * according to SNS convention.
 */
void Goniometer::makeUniversalGoniometer()
{
  motors.clear();
  this->pushAxis("phi",   0., 1., 0.,   0., Mantid::Geometry::CCW, Mantid::Geometry::angDegrees);
  this->pushAxis("chi",   1., 0., 0.,   0., Mantid::Geometry::CCW, Mantid::Geometry::angDegrees);
  this->pushAxis("omega", 0., 1., 0.,   0., Mantid::Geometry::CCW, Mantid::Geometry::angDegrees);
}


/// Private function to recalculate the rotation matrix of the goniometer
void Goniometer::recalculateR()
{
  std::vector<GoniometerAxis>::iterator it;
  std::vector<double> elements;
  Quat QGlobal,QCurrent;

  double ang;
  for(it=motors.begin(); it<motors.end(); it++)
  {
    ang=(*it).angle;
    if((*it).angleunit==angRadians) ang*=rad2deg;
    QCurrent=Quat(ang,(*it).rotationaxis);
    QGlobal*=QCurrent;
  }  
  elements=QGlobal.getRotation();
  R[0][0]=elements[0];
  R[1][0]=elements[1];
  R[2][0]=elements[2];
  R[0][1]=elements[3];
  R[1][1]=elements[4];
  R[2][1]=elements[5];
  R[0][2]=elements[6];
  R[1][2]=elements[7];
  R[2][2]=elements[8];
}

} //Namespace Geometry
} //Namespace Mantid
