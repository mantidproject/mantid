#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <cmath>
#include <complex>
#include <list>
#include <vector>
#include <map>
#include <stack>
#include <string>
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
#include "Line.h"
#include "BaseVisit.h"
#include "Surface.h"
#include "Quadratic.h"
#include "Cone.h"

namespace Mantid
{

namespace Geometry
{

Kernel::Logger& Cone::PLog(Kernel::Logger::get("Cone"));

/// Floating point tolerance
const double CTolerance(1e-6);

Cone::Cone() : Quadratic(),
  Centre(), Normal(1,0,0), alpha(0.0), cangle(1.0)
  /*!
    Constructor with centre line along X axis 
    and centre on origin
  */
{
  setBaseEqn();
}

Cone::Cone(const Cone& A) :Quadratic(A),
  Centre(A.Centre), Normal(A.Normal), 
  alpha(A.alpha), cangle(A.cangle)
  /*!
    Standard Copy Constructor
    \param A :: Cone to copy
  */
{}

Cone*
Cone::clone() const
  /*! 
    Makes a clone (implicit virtual copy constructor) 
    \return new(*this)
  */
{
  return new Cone(*this);
}

Cone&
Cone::operator=(const Cone& A)
  /*!
    Assignment operator
    \param A :: Cone to copy
    \return *this
  */
{
  if(this!=&A)
    {
      Quadratic::operator=(A);
      Centre=A.Centre;
      Normal=A.Normal;
      alpha=A.alpha;
      cangle=A.cangle;
    }
  return *this;
}


Cone::~Cone()
  /*!
    Destructor
  */
{} 

int
Cone::setSurface(const std::string& Pstr)
  /*! 
    Processes a standard MCNPX cone string    
    Recall that cones can only be specified on an axis
     Valid input is: 
     - k/x cen_x cen_y cen_z radius 
     - kx radius 
    \return : 0 on success, neg of failure 
  */
{
  std::string Line=Pstr;
  std::string item;
  if (!StrFunc::section(Line,item) || 
      tolower(item[0])!='k' || item.length()<2 || 
      item.length()>3)
    return -1;

  // Cones on X/Y/Z axis
  const int itemPt((item[1]=='/' && item.length()==3) ? 2 : 1);
  const int ptype=static_cast<int>(tolower(item[itemPt])-'x');
  if (ptype<0 || ptype>=3)
    return -2;
  double norm[3]={0.0,0.0,0.0};
  double cent[3]={0.0,0.0,0.0};
  norm[ptype]=1.0;

  if (itemPt==1)        // kx type cone
    {
      if (!StrFunc::section(Line,cent[ptype]))
	return -3;
    }
  else
    {
      int index;
      for(index=0;index<3 && StrFunc::section(Line,cent[index]);index++);
      if (index!=3)
	return -4;
    }
  // The user must enter t^2 which is tan^2(angle) for MCNPX
  double tanAng;
  if (!StrFunc::section(Line,tanAng))
    return -5;

  Centre=Geometry::Vec3D(cent);
  Normal=Geometry::Vec3D(norm);
  setTanAngle(sqrt(tanAng));
  setBaseEqn();
  return 0;
} 

int
Cone::operator==(const Cone& A) const
  /*!
    Equality operator. Checks angle,centre and 
    normal separately
    \param A :: Cone to compare
    \return A==this within CTolerance
  */
{
  if(this==&A)
    return 1;
  if (fabs(cangle-A.cangle)>CTolerance)
    return 0;
  if (Centre.Distance(A.Centre)>CTolerance)
    return 0;
  if (Normal.Distance(A.Normal)>CTolerance)
    return 0;

  return 1;
}

void
Cone::setBaseEqn()
  /*!
    Sets an equation of type 
    \f[ Ax^2+By^2+Cz^2+Dxy+Exz+Fyz+Gx+Hy+Jz+K=0 \f]
  */
{
  const double c2(cangle*cangle);
  const double CdotN(Centre.dotProd(Normal));
  BaseEqn[0]=c2-Normal[0]*Normal[0];     // A x^2
  BaseEqn[1]=c2-Normal[1]*Normal[1];     // B y^2
  BaseEqn[2]=c2-Normal[2]*Normal[2];     // C z^2 
  BaseEqn[3]= -2*Normal[0]*Normal[1];     // D xy
  BaseEqn[4]= -2*Normal[0]*Normal[2];     // E xz
  BaseEqn[5]= -2*Normal[1]*Normal[2];     // F yz
  BaseEqn[6]= 2.0*(Normal[0]*CdotN-Centre[0]*c2) ;     // G x
  BaseEqn[7]= 2.0*(Normal[1]*CdotN-Centre[1]*c2) ;     // H y
  BaseEqn[8]= 2.0*(Normal[2]*CdotN-Centre[2]*c2) ;     // J z
  BaseEqn[9]= c2*Centre.dotProd(Centre)-CdotN*CdotN;   // K const
  return;
}

void
Cone::rotate(const Geometry::Matrix<double>& R)
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
Cone::displace(const Geometry::Vec3D& A)
  /*!
    Displace the centre
    Only need to update the centre position 
    \param A :: Geometry::Vec3D to add
  */
{
    Centre+=A;
    setBaseEqn();
    return;
}

void 
Cone::setCentre(const Geometry::Vec3D& A)
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
Cone::setNorm(const Geometry::Vec3D& A)
  /*!
    Sets the Normal and the Base Equation
    \param A :: New Normal direction
  */
{
  if (A.abs()>CTolerance)
    {
      Normal=A;
      Normal.makeUnit();
      setBaseEqn();
    }
  return;
}

void
Cone::setAngle(const double A) 
  /*!
    Set the angle of the cone.
    \param A :: Angle in degrees.
    Resets the base equation
  */
{
  alpha=A;
  cangle=cos(M_PI*alpha/180.0);
  setBaseEqn();
  return;
}

void
Cone::setTanAngle(const double A) 
  /*! 
    Set the cone angle
    Resets the base equation 
    \param A :: Tan of the angle  (for MCNPX)
  */
{
  cangle=1.0/sqrt(A*A+1.0);        // convert tan(theta) to cos(theta)
  alpha=acos(cangle)*180.0/M_PI;
  setBaseEqn();
  return;
}

double
Cone::distance(const Geometry::Vec3D& Pt) const
  /*!
    Calculates the distance from the point to the Cone
    does not calculate the point on the cone that is closest
    \param Pt :: Point to calcuate from

    - normalise to a cone vertex at the origin
    - calculate the angle between the axis and the Point
    - Calculate the distance to P
    \return distance to Pt
  */
{
  const Geometry::Vec3D Px=Pt-Centre;
  // test is the centre to point distance is zero
  if(Px.abs()<CTolerance)
    return Px.abs();
  double Pangle=Px.dotProd(Normal)/Px.abs();
  if (Pangle<0.0)
    Pangle=acos(-Pangle);
  else
    Pangle=acos(Pangle);
  
  Pangle-=M_PI*alpha/180.0;
  return Px.abs()*sin(Pangle);
}

int
Cone::side(const Geometry::Vec3D& R) const
{
  /*!
    Calculate if the point R is within
    the cone (return -1) or outside, 
    (return 1)
    \todo NEEDS ON SURFACE CALCULATING::
    waiting for a point to cone distance function

    \param R :: Point to determine if in/out of cone
    \return Side of R
  */
  const Geometry::Vec3D cR = R-Centre;
  double rptAngle=cR.dotProd(Normal);
  rptAngle*=rptAngle/cR.dotProd(cR);
  return (sqrt(rptAngle)>cangle) ? 1 : -1;  
}

int
Cone::onSurface(const Geometry::Vec3D& R) const
{
  /*! 
     Calculate if the point R is on
     the cone (Note: have to be careful here
     since angle calcuation calcuates an angle.
     We need a distance for tolerance!)
     \param R :: Point to check
     \return 1 if on surface and -1 if not no surface
  */
  const Geometry::Vec3D cR = R-Centre;
  double rptAngle=cR.dotProd(Normal);
  rptAngle*=rptAngle/cR.dotProd(cR);
  return (sqrt(rptAngle)>cangle) ? 1 : -1;  
}

void
Cone::write(std::ostream& OX) const
  /*!
    Write out the cone class in an mcnpx
    format.
    \param OX :: Output Stream (required for multiple std::endl)
  */
{
  //               -3 -2 -1 0 1 2 3        
  const char Tailends[]="zyx xyz";
  const int Ndir=Normal.masterDir(CTolerance);
  if (Ndir==0)
    {
      Quadratic::write(OX);
      return;
    }
  std::ostringstream cx;
  Quadratic::writeHeader(cx);
  
  const int Cdir=Centre.masterDir(CTolerance);
  cx.precision(Surface::Nprecision);
  // Name and transform 
   
  if (Cdir || Centre.nullVector(CTolerance))
    {
      cx<<" k";
      cx<< Tailends[Ndir+3]<<" ";          // set x,y,z based on Ndir
      cx<< ((Cdir>0) ? Centre[Cdir-1] : Centre[-Cdir-1]);
      cx<<" ";
    }
  else
    {
      cx<<" k/";
      cx<< Tailends[Ndir+3]<<" ";          // set x,y,z based on Ndir
      for(int i=0;i<3;i++)
	cx << Centre[i] << " ";
    }
  const double TA=tan((M_PI*alpha)/180.0);    // tan^2(angle)
  cx<<TA*TA;
  StrFunc::writeMCNPX(cx.str(),OX);
  return;
}

int
Cone::importXML(IndexIterator<XML::XMLobject,XML::XMLgroup>& SK,
		const int singleFlag)
  /*!
    Given a distribution that has been put into an XML base set.
    The XMLcollection need to have the XMLgroup pointing to
    the section for this DExpt.

    \param SK :: IndexIterator object
    \param singleFlag :: single pass through to determine if has key
    (only for virtual base object)
   */
{
  int errCnt(0);
  int levelExit(SK.getLevel());
  do
    {
      if (*SK)
        {
	  const std::string& KVal=SK->getKey();
	  const XML::XMLread* RPtr=dynamic_cast<const XML::XMLread*>(*SK);
	  int errNum(1);
	  if (RPtr)
	    {
	      if (KVal=="Centre")
		errNum=(StrFunc::convert(RPtr->getFront(),Centre)) ? 0 : 1;
	      else if (KVal=="Normal")
		errNum=(StrFunc::convert(RPtr->getFront(),Normal)) ? 0 : 1;
	      else if (KVal=="Alpha")
	        {
		  errNum=(StrFunc::convert(RPtr->getFront(),alpha)) ? 0 : 1;
		  cangle=cos(M_PI*alpha/180.0);
		}
	      else
		errNum=Quadratic::importXML(SK,1);
	    }
	  if (errNum)
	    {
	      errCnt++;                 // Not good....
	      PLog.warning("importXML :: Key failed "+KVal);
	    }
	  // Post processing
	  if (!singleFlag) 
	    SK++;
	}
    } while (!singleFlag && SK.getLevel()>=levelExit);

  // Reset: stuff.
  if (!singleFlag)
    setBaseEqn();

  return errCnt;
}

void
Cone::procXML(XML::XMLcollect& XOut) const
  /*!
    This writes the XML schema
    \param XOut :: Output parameter
   */
{
  Quadratic::procXML(XOut);
  XOut.addComp("Centre",Centre);
  XOut.addComp("Normal",Normal);
  XOut.addComp("alpha",alpha);
  return;
}

}  // NAMESPACE Geometry

}  // NAMESPACE Mantid
