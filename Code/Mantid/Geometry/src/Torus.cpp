#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <sstream>
#include <cmath>
#include <list>
#include <vector>
#include <map>
#include <stack>
#include <algorithm>

#include "Logger.h"
#include "Exception.h"
#include "XMLattribute.h"
#include "XMLobject.h"
#include "XMLgroup.h"
#include "XMLread.h"
#include "XMLcollect.h"
#include "IndexIterator.h"
#include "Support.h"
#include "Matrix.h"
#include "Vec3D.h"
#include "BaseVisit.h"
#include "Surface.h"
#include "Torus.h"

namespace Mantid
{

namespace Geometry
{

Logger& Torus::PLog = Logger::get("Torus");

int 
Torus::possibleLine(const std::string& Line)
  /*! 
     Checks to see if the input string is a torus.
     Valid input is: 
     - number {transformNumber} tz cen_x cen_y cen_z A B C
     \retval 1 :: all parts for a torus
     \retval -1 :: extension possible
     \retval 0 :: no a Torus object 
  */
{
  // Split line
  std::vector<std::string> Items=StrFunc::StrParts(Line);
  if (Items.size()<3)           //Indecyferable line
    return 0;

  unsigned int ix(1);

  if (tolower(Items[ix][0])!='t' &&         //Not a cone
      tolower(Items[++ix][0])!='t')
    return 0;                    

  if (Items[ix].length()<2)        // too short
    return 0;

  if (tolower(Items[ix][1])>='x' && 
      tolower(Items[ix][1])<='z')   // Simple  ?
    {
      return (Items.size()>ix+1) ? 1 : -1;     
    }

  return 0;
}


Torus::Torus() : Surface(),TTolerance(1e-6),
		 Centre(), Normal(1,0,0), 
		 Iradius(0),Dradius(0),Displacement(0)
  /*!
    Constructor with centre line along X axis 
    and centre on origin
  */
{
  setBaseEqn();
}

Torus::Torus(const Torus& A) : 
  Surface(A),TTolerance(A.TTolerance),
  Centre(A.Centre), Normal(A.Normal), 
  Iradius(A.Iradius), Dradius(A.Dradius),
  Displacement(A.Displacement)
  /*!
    Standard Copy Constructor
    \param A :: Torus to copy
  */
{}

Torus*
Torus::clone() const
  /*! 
    Makes a clone (implicit virtual copy constructor) 
    \return new(*this)
  */
{
  return new Torus(*this);
}

Torus&
Torus::operator=(const Torus& A)
  /*!
    Assignment operator
    \param A :: Torus to copy
    \return *this
  */
{
  if(this!=&A)
    {
      Surface::operator=(A);
      Centre=A.Centre;
      Normal=A.Normal;
      Iradius=A.Iradius;
      Dradius=A.Dradius;
      Displacement=A.Displacement;
    }
  return *this;
}


Torus::~Torus()
  /*!
    Destructor
  */
{} 

int
Torus::operator==(const Torus& A) const
  /*!
    Equality operator. Checks angle,centre and 
    normal separately
    \param A :: Torus to compare
    \return A==this within TTolerance
  */
{
  if(this==&A)
    return 1;
  if ( (fabs(Displacement-A.Displacement)>TTolerance) ||
       (fabs(Iradius-A.Iradius)>TTolerance) ||
       (fabs(Dradius-A.Dradius)>TTolerance) )
    return 0;

  if (Centre.Distance(A.Centre)>TTolerance)
    return 0;
  if (Normal.Distance(A.Normal)>TTolerance)
    return 0;

  return 1;
}

int 
Torus::setSurface(const std::string& Pstr)
  /*! 
    processes a standard MCNPX cone string    
    Recall that cones can only be specified on an axis
     Valid input is: 
     - number {transformNumber} t/x cen_x cen_y cen_z a,b,c
    \return : 0 on success, neg of failure 
  */
{
  std::vector<std::string> Items=StrFunc::StrParts(Pstr);
  if (Items.size()<8)           //Indecyferable line
    return -1;

  int nx;
  std::vector<std::string>::iterator ic=Items.begin();
  if (!StrFunc::convert(*ic++,nx))            // Get name 
    return -2;

  setName(nx);                       // Assign the name

  if (tolower((*ic)[0])!='t')       // Not a torus
    return -3;

  if (ic->length()<2)               // Not a torus
    return -4;

  // Toruss on X/Y/Z axis

  int ptype=static_cast<int>(tolower((*ic)[1])-'x');
  double norm[3]={0.0,0.0,0.0};
  double cent[3]={0.0,0.0,0.0};
  if (ptype<0 || ptype>2)
    return -4;

  ic++;   // Go to next element in vector 
  norm[ptype]=1.0;
  for(int i=0;i<3;i++,ic++)
     
    if(ic==Items.end() || !StrFunc::convert(*ic,cent[i]))
      return -5;
  
  Centre=Geometry::Vec3D(cent);
  Normal=Geometry::Vec3D(norm);
  double X[3];
  for(int i=0;i<3;i++,ic++)
    if(ic==Items.end() || !StrFunc::convert(*ic,X[i]))
      return -6;

  Iradius=X[1];
  Dradius=X[2];
  Displacement=X[0];
  return 0;
} 


void
Torus::setBaseEqn()
  /*!
    Sets an equation of type 
    \f[ Ax^2+By^2+Cz^2+Dxy+Exz+Fyz+Gx+Hy+Jz+K=0 \f]
  */
{
  for(int i=0;i<10;i++)
    BaseEqn[i]=0;
  return;
}

void
Torus::rotate(const Geometry::Matrix<double>& R)
  /*!
    Rotate both the centre and the normal direction 
    \param R :: Matrix for rotation. 
  */
{
  Centre.rotate(R);
  Normal.rotate(R);
  setBaseEqn();
  return;
}

void 
Torus::displace(const Geometry::Vec3D& A)
  /*!
    Displace the centre
    Only need to update the centre position 
    \param A :: Point to add
  */
{
    Centre+=A;
    setBaseEqn();
    return;
}

void 
Torus::setCentre(const Geometry::Vec3D& A)
  /*!
    Sets the central point and the Base Equation
    \param A :: New Centre point
  */
{
  Centre=A;
  setBaseEqn();
  return;
}

void 
Torus::setNorm(const Geometry::Vec3D& A)
  /*!
    Sets the Normal and the Base Equation
    \param A :: New Normal direction
  */
{
  if (A.abs()>TTolerance)
    {
      Normal=A;
      Normal.makeUnit();
      setBaseEqn();
    }
  return;
}


double
Torus::distance(const Geometry::Vec3D& Pt) const
  /*!
    Calculates the distance from the point to the Torus
    does not calculate the point on the Torusthat is closest
    \param Pt :: Point to calcuate from

    - normalise to a cone vertex at the origin
    - calculate the angle between the axis and the Point
    - Calculate the distance to P
    \return distance to Pt
  */
{
  const Geometry::Vec3D Px=Pt-Centre;
  // test is the centre to point distance is zero
  if(Px.abs()<TTolerance)
    return Px.abs();
  return Px.abs();
}

int
Torus::side(const Geometry::Vec3D& R) const

  /*!
    Calculate if the point R is within
    the torus (return -1) or outside, 
    (return 1)
    \todo NEEDS ON SURFACE CALCULATING::
    waiting for a point to cone distance function

    \param R :: Point to determine if in/out of cone
    \return Side of R
  */
{
  return -1;
}

int
Torus::onSurface(const Geometry::Vec3D& R) const
{
  /*! 
     Calculate if the point R is on
     the cone (Note: have to be careful here
     since angle calcuation calcuates an angle.
     We need a distance for tolerance!)
     \param R :: Point to check
     \return 1 if on surface and -1 if not no surface
  */

  return -1;
}

void
Torus::write(std::ostream& OX) const
  /*!
    Write out the cone class in an mcnpx
    format.
    \param OX :: Output Stream (required for multiple std::endl)
  */
{
  //               -3 -2 -1 0 1 2 3        
  const char Tailends[]="zyx xyz";
  const int Ndir=Normal.masterDir(TTolerance);
  if (Ndir==0)
    {
      Surface::write(OX);
      return;
    }
  std::ostringstream cx;
  Surface::writeHeader(cx);
  cx<<" t"<<Tailends[Ndir+3]<<" ";
  cx.precision(Surface::Nprecision);
  // Name and transform 
   
  cx<<Centre<<" "<<Displacement<<" "<<Dradius<<" "<<Iradius<<std::endl;
  StrFunc::writeMCNPX(cx.str(),OX);
  return;
}

}  // NAMESPACE MonteCarlo

}  // NAMESPACE Mantid
