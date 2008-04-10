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

#include "MantidKernel/Logger.h"
#include "AuxException.h"
#include "XMLattribute.h"
#include "XMLobject.h"
#include "XMLgroup.h"
#include "XMLread.h"
#include "XMLcollect.h"
#include "IndexIterator.h"
#include "MantidKernel/Support.h"
#include "Matrix.h"
#include "Vec3D.h"
#include "BaseVisit.h"
#include "Surface.h"
#include "Torus.h"

namespace Mantid
{

namespace Geometry
{

Kernel::Logger& Torus::PLog( Kernel::Logger::get("Torus"));

const double TTolerance(1e-6);       ///< Tolerance to the surfaces.
  
Torus::Torus() : Surface(),
		 Centre(), Normal(1,0,0), 
		 Iradius(0),Dradius(0),Displacement(0)
  /*!
    Constructor with centre line along X axis 
    and centre on origin
  */
{}
 
Torus::Torus(const Torus& A) : Surface(A),
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
    Processes a standard MCNPX cone string    
    Recall that cones can only be specified on an axis
     Valid input is: 
     - number {transformNumber} t/x cen_x cen_y cen_z a,b,c
    \param Pstr :: String to process
    \return : 0 on success, neg of failure 
  */
{
  enum { errDesc=-1, errAxis=-2,
	 errCent=-3, errNormal=-4};

  std::string Line=Pstr;

  std::string item;
  if (!StrFunc::section(Line,item) || 
      tolower(item[0])!='t' || item.length()!=2) 
    return errDesc;

  // Torus on X/Y/Z axis
  const int ptype=static_cast<int>(tolower(item[2])-'x');
  if (ptype<0 || ptype>=3)
    return errAxis;

  Geometry::Vec3D Norm;
  Geometry::Vec3D Cent;
  Geometry::Vec3D PtVec;
  Norm[ptype]=1.0;

  // Torus on X/Y/Z axis
  Norm[ptype]=1.0;
  if (!StrFunc::section(Line,Centre))
    return errCent;
  if (!StrFunc::section(Line,PtVec))
    return errCent;

  for(int i=0;i<3;i++)
    return errCent;
  
  Iradius=PtVec[1];
  Dradius=PtVec[2];
  Displacement=PtVec[0];
  return 0;
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
    }
  return;
}

Geometry::Vec3D
Torus::surfaceNormal(const Geometry::Vec3D& Pt) const
  /*!
    Get the normal at a point
    \param A :: New Normal direction
    \todo Does not work
  */
{
  return Normal;
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
