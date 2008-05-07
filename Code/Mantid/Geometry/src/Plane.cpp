#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <cmath>
#include <list>
#include <vector>
#include <map>
#include <stack>
#include <string>
#include <algorithm>
#include <boost/regex.hpp>

#include "MantidKernel/Logger.h"
#include "MantidKernel/Exception.h"

#include "MantidKernel/Support.h"
#include "MantidGeometry/Matrix.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/BaseVisit.h"
#include "MantidGeometry/Surface.h"
#include "MantidGeometry/Quadratic.h"
#include "MantidGeometry/Plane.h"


namespace Mantid
{

namespace Geometry
{

Kernel::Logger& Plane::PLog(Kernel::Logger::get("Plane"));

/// Numerical tolerance
const double PTolerance(1e-6); 

Plane::Plane() : Quadratic(),
  NormV(1.0,0.0,0.0),Dist(0)
  /*!
    Constructor: sets plane in y-z plane and throught origin
  */
{
  setBaseEqn();
}

Plane::Plane(const Plane& A) : Quadratic(A),
   NormV(A.NormV),Dist(A.Dist)
  /*!
    Copy Constructor
    \param A :: Plane to copy
  */
{}

Plane*
Plane::clone() const
  /*!
    Makes a clone (implicit virtual copy constructor) 
    \return new(this)
  */
{
  return new Plane(*this);
}

Plane&
Plane::operator=(const Plane& A) 
  /*!
    Assignment operator
    \param A :: Plane to copy
    \return *this
  */
{
  if (&A!=this)
    {
      this->Quadratic::operator=(A);
      NormV=A.NormV;
      Dist=A.Dist;
    }
  return *this;
}

Plane::~Plane()
  /*!
    Destructor
  */
{}

int
Plane::setSurface(const std::string& Pstr)
  /*! 
     processes a standard MCNPX plane string:
     There are three types : 
     - (A) px Distance
     - (B) p A B C D (equation Ax+By+Cz=D)
     - (C) p V3D V3D V3D
     \param Pstr :: String to make into a plane of type p{xyz} or p 
     \return 0 on success, -ve of failure
  */
{
  // Two types of plane string p[x-z]  and p
  std::string Line=Pstr;
  std::string item;
  
  if (!StrFunc::section(Line,item) || tolower(item[0])!='p')
    return -1;
  // Only 3 need to be declared
  double surf[9]={0.0,0,0,0,0};
      
  if (item.size()==1)  // PROCESS BASIC PLANE:
    {
      int cnt;
      for(cnt=0;cnt<9 && StrFunc::section(Line,surf[cnt]);cnt++);

      if (cnt!=4 && cnt!=9)
		return -3;
      if (cnt==9)          // V3D type
        {
	  Geometry::V3D A=Geometry::V3D(surf[0],surf[1],surf[2]);
	  Geometry::V3D B=Geometry::V3D(surf[3],surf[4],surf[5]);
	  Geometry::V3D C=Geometry::V3D(surf[6],surf[7],surf[8]);
	  B-=A;
	  C-=A;
	  NormV = B*C;
	  NormV.normalize();
	  Dist=A.scalar_prod(NormV);
	}
      else        // Norm Equation:
        { 
	  NormV=Geometry::V3D(surf[0],surf[1],surf[2]);
	  const double ll=NormV.normalize();
	  if (ll<PTolerance)   // avoid 
	    return -4;
	  Dist= surf[3]/ll;
	}
    }
  else if (item.size()==2)  //  PROCESS px type PLANE
    {
      const int ptype=static_cast<int>(tolower(item[1])-'x');
      if (ptype<0 || ptype>2)         // Not x,y,z
	return -5;
      surf[ptype]=1.0;
      if (!StrFunc::convert(Line,Dist))
	return -6;                      //Too short or no number
      NormV=Geometry::V3D(surf[0],surf[1],surf[2]);
    }
  else
    return -3;       // WRONG NAME

  setBaseEqn();
  return 0;
}

int
Plane::setPlane(const Geometry::V3D& P,const Geometry::V3D& N) 
  /*!
    Given a point and a normal direction set the plane
    \param P :: Point for plane to pass thought
    \param N :: Normal for the plane
    \retval 0 :: success
  */
{
  NormV=N;
  NormV.normalize();
  Dist=P.scalar_prod(NormV);
  setBaseEqn();
  return 0;
}

void
Plane::rotate(const Geometry::Matrix<double>& MA) 
  /*!
    Rotate the plane about the origin by MA 
    \param MA direct rotation matrix (3x3)
  */
{
  NormV.rotate(MA);
  NormV.normalize();
  Quadratic::rotate(MA);
  return;
}

void
Plane::displace(const Geometry::V3D& Sp) 
  /*!
    Displace the plane by Point Sp.  
    i.e. r+sp now on the plane 
    \param Sp :: point value of displacement
  */
{
  Dist+=NormV.scalar_prod(Sp);
  Quadratic::displace(Sp);
  return;
}

double
Plane::distance(const Geometry::V3D& A) const
  /*!
    Determine the distance of point A from the plane 
    returns a value relative to the normal
    \param A :: point to get distance from 
    \returns singed distance from point
  */
{
  return A.scalar_prod(NormV)-Dist;
}

double
Plane::dotProd(const Plane& A) const
  /*!
    \param A :: plane to calculate the normal distance from x
    \returns the Normal.A.Normal dot product
  */
{
  return NormV.scalar_prod(A.NormV);
}

Geometry::V3D
Plane::crossProd(const Plane& A) const
  /*!
    Take the cross produce of the normals
    \param A :: plane to calculate the cross product from 
    \returns the Normal x A.Normal cross product 
  */
{
	return NormV.cross_prod(A.NormV);
}



int
Plane::side(const Geometry::V3D& A) const
  /*!
    Calcualates the side that the point is on
    \param A :: test point
    \retval +ve :: on the same side as the normal
    \retval -ve :: the  opposite side 
    \retval 0 :: A is on the plane itself (within tolerence) 
  */
{
  double Dp=NormV.scalar_prod(A);
  Dp-=Dist;
  if (PTolerance<fabs(Dp))
    return (Dp>0) ? 1 : -1;
  return 0;
}

int
Plane::onSurface(const Geometry::V3D& A) const
  /*! 
     Calcuate the side that the point is on
     and returns success if it is on the surface.
     - Uses PTolerance to determine the closeness
     \retval 1 if on the surface 
     \retval 0 if off the surface 
     
  */
{
  return (side(A)!=0) ? 0 : 1;
}

void 
Plane::print() const
  /*!
    Prints out the surface info and
    the Plane info.
  */
{
  Quadratic::print();
  std::cout<<"NormV == "<<NormV<<" : "<<Dist<<std::endl;
  return;
}

int
Plane::planeType() const
  /*! 
     Find if the normal vector allows it to be a special
     type of plane (x,y,z direction) 
     (Assumes NormV is a unit vector)
     \retval 1-3 :: on the x,y,z axis
     \retval 0 :: general plane
  */
{
  for(int i=0;i<3;i++)
    if (fabs(NormV[i])>(1.0-PTolerance))
      return i+1;
  return 0;
}


void 
Plane::setBaseEqn()
  /*!
    Sets the general equation for a plane
  */
{
  BaseEqn[0]=0.0;     // A x^2
  BaseEqn[1]=0.0;     // B y^2
  BaseEqn[2]=0.0;     // C z^2 
  BaseEqn[3]=0.0;     // D xy
  BaseEqn[4]=0.0;     // E xz
  BaseEqn[5]=0.0;     // F yz
  BaseEqn[6]=NormV[0];     // G x
  BaseEqn[7]=NormV[1];     // H y
  BaseEqn[8]=NormV[2];     // J z
  BaseEqn[9]= -Dist;        // K const
  return;
}

void 
Plane::write(std::ostream& OX) const
  /*! 
    Object of write is to output a MCNPX plane info 
    \param OX :: Output stream (required for multiple std::endl)  
    \todo (Needs precision) 
  */
{
  std::ostringstream cx;
  Surface::writeHeader(cx);
  cx.precision(Surface::Nprecision);
  const int ptype=planeType();
  if (!ptype)
    cx<<"p "<<NormV[0]<<" "
       <<NormV[1]<<" "
       <<NormV[2]<<" "
      <<Dist;
  else if(NormV[ptype-1]<0)
    cx<<"p"<<"xyz"[ptype-1]<<" "<<-Dist;
  else 
    cx<<"p"<<"xyz"[ptype-1]<<" "<<Dist;

  StrFunc::writeMCNPX(cx.str(),OX);
  return;
}



} // NAMESPACE MonteCarlo

}  // NAMESPACE Mantid
