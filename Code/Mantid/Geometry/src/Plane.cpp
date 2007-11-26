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
#include "XMLattribute.h"
#include "XMLobject.h"
#include "XMLgroup.h"
#include "XMLread.h"
#include "XMLcollect.h"
#include "IndexIterator.h"
#include "MantidKernel/Support.h"
#include "regexSupport.h"
#include "Matrix.h"
#include "Vec3D.h"
#include "BaseVisit.h"
#include "Surface.h"
#include "Plane.h"

namespace Mantid
{

namespace Geometry
{

Kernel::Logger& Plane::PLog(Kernel::Logger::get("Plane"));

int
Plane::possibleLine(const std::string& Line)
  /*!
     Checks to see if the string is sufficient
     and appropiate to be a plane 
     \retval 1 :: all parts for a plane
     \retval -1 :: extension possible 
  */
{
  // Split line
  std::vector<std::string> Items=StrFunc::StrParts(Line);
  if (Items.size()<2)           //Indecyferable lineSu
    return 0;

  unsigned int ix(1);

  if (tolower(Items[ix++][0])!='p' &&         //Not a plane
      tolower(Items[ix++][0])!='p')
    return 0;                    
  ix--;
  if (Items[ix][1]>='x' && Items[ix][1]<='z')       //Simple plane ?
    {
      return (Items.size()>ix+1) ? 1 : -1;
    }
  if (Items.size()==ix+5 || Items.size()==ix+10)
    return 1;
  if (Items.size()<10)
    return -1;
  return 0;
}
 

Plane::Plane() : Surface(),
  PTolerance(1e-6),NormV(1.0,0.0,0.0),Dist(0)
  /*!
    Constructor: sets plane in y-z plane and throught origin
  */
{
  setBaseEqn();
}

Plane::Plane(const Plane& A) : Surface(A),
  PTolerance(A.PTolerance),NormV(A.NormV),Dist(A.Dist)
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
      this->Surface::operator=(A);
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
     - (C) p Vec3D Vec3D Vec3D
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
      if (cnt!=4 || cnt!=9)
	return -3;
      if (cnt==9)          // Vec3d type
        {
	  Geometry::Vec3D A=Geometry::Vec3D(surf[0],surf[1],surf[2]);
	  Geometry::Vec3D B=Geometry::Vec3D(surf[3],surf[4],surf[5]);
	  Geometry::Vec3D C=Geometry::Vec3D(surf[6],surf[7],surf[8]);
	  B-=A;
	  C-=A;
	  NormV = B*C;
	  NormV.makeUnit();
	  Dist=A.dotProd(NormV);
	}
      else        // Norm Equation:
        { 
	  NormV=Geometry::Vec3D(surf[0],surf[1],surf[2]);
	  const double ll=NormV.makeUnit();
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
      NormV=Geometry::Vec3D(surf[0],surf[1],surf[2]);
    }
  else
    return -3;       // WRONG NAME

  setBaseEqn();
  return 0;
}

int
Plane::setPlane(const Geometry::Vec3D& P,const Geometry::Vec3D& N) 
  /*!
    Given a point and a normal direction set the plane
    \param P :: Point for plane to pass thought
    \param N :: Normal for the plane
    \retval 0 :: success
  */
{
  NormV=N;
  NormV.makeUnit();
  Dist=P.dotProd(NormV);
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
  NormV.makeUnit();
  Surface::rotate(MA);
  return;
}

void
Plane::displace(const Geometry::Vec3D& Sp) 
  /*!
    Displace the plane by Point Sp.  
    i.e. r+sp now on the plane 
    \param Sp :: point value of displacement
  */
{
  Dist+=NormV.dotProd(Sp);
  Surface::displace(Sp);
  return;
}

double
Plane::distance(const Geometry::Vec3D& A) const
  /*!
    Determine the distance of point A from the plane 
    returns a value relative to the normal
    \param A :: point to get distance from 
    \returns singed distance from point
  */
{
  return A.dotProd(NormV)-Dist;
}

double
Plane::dotProd(const Plane& A) const
  /*!
    \param A :: plane to calculate the normal distance from x
    \returns the Normal.A.Normal dot product
  */
{
  return NormV.dotProd(A.NormV);
}

Geometry::Vec3D
Plane::crossProd(const Plane& A) const
  /*!
    Take the cross produce of the normals
    \param A :: plane to calculate the cross product from 
    \returns the Normal x A.Normal cross product 
  */
{
  return NormV*A.NormV;
}



int
Plane::side(const Geometry::Vec3D& A) const
  /*!
    Calcualates the side that the point is on
    \param A :: test point
    \retval +ve :: on the same side as the normal
    \retval -ve :: the  opposite side 
    \retval 0 :: A is on the plane itself (within tolerence) 
  */
{
  double Dp=NormV.dotProd(A);
  Dp-=Dist;
  if (PTolerance<fabs(Dp))
    return (Dp>0) ? 1 : -1;
  return 0;
}

int
Plane::onSurface(const Geometry::Vec3D& A) const
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
  Surface::print();
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

int
Plane::importXML(IndexIterator<XML::XMLobject,XML::XMLgroup>& SK,
		    const int singleFlag)
  /*!
    Given a distribution that has been put into an XML base set.
    The XMLcollection need to have the XMLgroup pointing to
    the section for this Plane.

    \param SK :: IndexIterator object
    \param singleFlag :: single pass through to determine if has key
    (only for virtual base object)
    \return Error count (+ve)
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
	      if (KVal=="NormV")
		errNum=(StrFunc::convert(RPtr->getFront(),NormV)) ? 0 : 1;
	      else if (KVal=="Dist")
		errNum=(StrFunc::convert(RPtr->getFront(),Dist)) ? 0 : 1;
	      else
		errNum=Surface::importXML(SK,1);
	    }
	  if (errNum)
	    {
	      PLog.warning("importXML :: Key failed "+KVal);
	      errCnt++;
	    }
	  // Post processing
	  if (!singleFlag) 
	    SK++;
	}
    } while (!singleFlag && SK.getLevel()>=levelExit);

  return errCnt;
}

void
Plane::procXML(XML::XMLcollect& XOut) const
  /*!
    This writes the XML schema
    \param XOut :: Output parameter
   */
{
  XOut.getCurrent()->addAttribute("type","Plane");
  Surface::procXML(XOut);
  XOut.addComp("NormV",NormV);
  XOut.addComp("Dist",Dist);
  return;
}



} // NAMESPACE MonteCarlo

}  // NAMESPACE Mantid
