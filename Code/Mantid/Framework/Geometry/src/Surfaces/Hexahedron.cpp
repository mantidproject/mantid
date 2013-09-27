#include "MantidGeometry/Surfaces/Hexahedron.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/Matrix.h"

namespace Mantid
{
namespace Geometry
{

Kernel::Logger& Hexahedron::g_log(Kernel::Logger::get("Hexahedron"));

const double GTolerance(1e-6);  ///< Tolerance

/**
 * Default constructor.
 */
Hexahedron::Hexahedron() : Quadratic()
{
}

/**
 * Copy constructor.
 */
Hexahedron::Hexahedron(const Hexahedron& other) : Quadratic(other)
{
}

/**
 * "Implicit virtual copy constructor", as seen in the other shape classes.
 */
Hexahedron* Hexahedron::clone() const
{
  return new Hexahedron(*this);
}

/**
 * Assignment operator.
 */
Hexahedron& Hexahedron::operator=(const Hexahedron& other)
{
  if (this != &other)
    Hexahedron::operator=(other);

  return *this;
}

Hexahedron::~Hexahedron()
{}

int Hexahedron::setSurface(const std::string& Pstr)
  /** 
    Processes a standard MCNPX general string (GQ/SQ types)
    Despite type, moves both to the general equation.

    NOTE: Check the gq version with the MCNPX source code
          since there are multiple version of xy xz yz parameter
	  read-in which swap xz and yz. [This code uses the first]

    @param Pstr :: String to process (with name and transform)
    @return 0 on success, neg of failure
  */
{
  std::string Line=Pstr;
  std::string item;
  if (!Mantid::Kernel::Strings::section(Line,item) || item.length()!=2 ||
      (tolower(item[0])!='g' &&  tolower(item[0]!='s')) ||
      tolower(item[1])!='q')
    return -1;
      
  double num[10];
  int index;
  for(index=0;index<10 && 
	Mantid::Kernel::Strings::section(Line,num[index]);index++);
  if (index!=10)
    return -2;

  if (tolower(item[0])=='g')
    {
      for(int i=0;i<10;i++)
	Quadratic::BaseEqn[i]=num[i];
    }
  else
    {
      Quadratic::BaseEqn[0]=num[0];
      Quadratic::BaseEqn[1]=num[1];
      Quadratic::BaseEqn[2]=num[2];
      Quadratic::BaseEqn[3]=0.0;;
      Quadratic::BaseEqn[4]=0.0;;
      Quadratic::BaseEqn[5]=0.0;;
      Quadratic::BaseEqn[6]=2*(num[3]-num[7]*num[0]);
      Quadratic::BaseEqn[7]=2*(num[4]-num[8]*num[1]);
      Quadratic::BaseEqn[8]=2*(num[5]-num[9]*num[2]);
      Quadratic::BaseEqn[9]=num[0]*num[7]*num[7]+
	num[1]*num[8]*num[8]+num[2]*num[9]*num[9]-
	2.0*(num[3]*num[7]+num[4]*num[8]+num[5]*num[9])+
	num[6];
    }
  return 0;
  
}

void Hexahedron::setBaseEqn()
  /**
    Set baseEqn (nothing to do) as it is 
    already a baseEqn driven system
  */
{
  return;
}

void Hexahedron::getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin, double &ymin, double &zmin)
{
  UNUSED_ARG(xmax);
  UNUSED_ARG(ymax);
  UNUSED_ARG(zmax);
  UNUSED_ARG(xmin);
  UNUSED_ARG(ymin);
  UNUSED_ARG(zmin);
}

}  // namespace Geometry
}  // namespace Mantid
