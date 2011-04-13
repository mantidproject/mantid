#include "MantidGeometry/Instrument/Goniometer.h"
#include <sstream>

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
Goniometer::Goniometer(const Goniometer& other):R(other.R),motors(other.motors),initFromR(other.initFromR)
{
}

/// Constructor from a rotation matrix
Goniometer::Goniometer(MantidMat rot):R(rot),initFromR(true)
{
  //TODO check if rot is a rotation matrix
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
  if initFromR 
  {
    return string("Goniometer was initialized from a rotation matrix. No information about axis is available.\n";
  }
  else
  {
    std::stringstream info;
    std::vector<Axis>::iterator it;
    std::string CW("CW"),CCW("CCW"),sense;
    double angle;

    if (motors.size == 0) 
    {
      info<<"No axis is found\n";
    }
    else
    {
      info<<"Name \t Direction \t Sense \t Angle \n";
      for(it=motors.begin(); it<motors.end(); it++)
      {
        sense=(*it->sense==1)?CCW:CW;
        angle=(*it->angleunit==angDegrees)?*it->angle(): *it->angle()*rad2deg; 
        info<<*it->name<<"\t"<<*it->rotationaxis<<"\t"<<sense<<"\t"<<angle<<std::endl;
      }  
    }
    return info.str();
}

void Goniometer::pushAxis(std::string name, double axisx, double axisy, double axisz, double angle=0., int sense=CCW, int angUnit=angDegrees)
{
  if initFromR 
  {
    //TODO throw some exception
  }
  else
  {
    // check if such axis is already defined
    for(it=motors.begin(); it<motors.end(); it++)
    {
      if(compare(*it->name,name)==0){}//TODO throw exception
    }
    Axis a(name,V3D(axisx,axisy,axisz),angle,sense,angUnit);
    motors.push_back(a);
  }
}

/// Set rotation angle for an axis using motor name
void Goniometer::setRotationAngle( std::string name, double value)
{
  bool changed=false;
  for(it=motors.begin(); it<motors.end(); it++)
  {
    if(compare(*it->name,name)==0)
    {
      *it->value=value
    }
  }
  if(!changed)
  {
  //TODO throw error 
  }
  recalculateR();
}
/// Set rotation angle for an axis using motor number
void Goniometer::setRotationAngle( size_t axisnumber, double value)
{
  (motors.at(axisnumber)).value=value;//it will throw out of range exception if axisnumber is not in range
  recalculateR();
}

void Goniometer::recalculateR()
{
  
}

} //Namespace Geometry
} //Namespace Mantid
