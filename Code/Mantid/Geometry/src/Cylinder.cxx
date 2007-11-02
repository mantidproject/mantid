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
#include <boost/regex.hpp>

#include "Logger.h"
#include "Exception.h"
#include "XMLattribute.h"
#include "XMLobject.h"
#include "XMLgroup.h"
#include "XMLread.h"
#include "XMLcollect.h"
#include "IndexIterator.h"
#include "FileReport.h"
#include "GTKreport.h"
#include "OutputLog.h"
#include "support.h"
#include "regexSupport.h"
#include "Matrix.h"
#include "Vec3D.h"
#include "Line.h"
#include "BaseVisit.h"
#include "Surface.h"
#include "Cylinder.h"

namespace Mantid
{

namespace Geometry
{

Logger& Cylinder::PLog = Logger::get("Cylinder");
const double CTolerance(1e-6);

Cylinder::Cylinder() : Surface(),
   Centre(),Normal(1,0,0),Nvec(0),Radius(0.0)
  /*!
    Standard Constructor creats a cylinder (radius 0)
    along the x axis
  */
{
  // Called after it has been sized by Surface
  Cylinder::setBaseEqn();
}

Cylinder::Cylinder(const Cylinder& A) :
  Surface(A),Centre(A.Centre),Normal(A.Normal),
  Nvec(A.Nvec),Radius(A.Radius)
  /*!
    Standard Copy Constructor
    \param A :: Cyclinder to copy
  */
{}

Cylinder*
Cylinder::clone() const
  /*!
    makes a clone (implicit virtual copy constructor) 
    \return Copy(*this)
  */
{
  return new Cylinder(*this);
}

Cylinder&
Cylinder::operator=(const Cylinder& A) 
  /*!
    Standard Assignment operator
    \param A :: Cylinder object to copy
    \return *this
  */
{
  if (this!=&A)
    {
      Surface::operator=(A);
      Centre=A.Centre;
      Normal=A.Normal;
      Nvec=A.Nvec;
      Radius=A.Radius;
    }
  return *this;
}

Cylinder::~Cylinder()
  /*!
    Standard Destructor
  */
{}

int
Cylinder::setSurface(const std::string& Pstr)
  /*! 
    Processes a standard MCNPX cone string    
    Recall that cones can only be specified on an axis
     Valid input is: 
     - c/x cen_y cen_z radius 
     - cx radius 
    \return : 0 on success, neg of failure 
  */
{
  enum { errDesc=-1, errAxis=-2,
	 errCent=-3, errRadius=-4};

  std::string Line=Pstr;
  std::string item;
  if (!StrFunc::section(Line,item) || 
      tolower(item[0])!='c' || item.length()<2 || 
      item.length()>3)
    return errDesc;

  // Cylinders on X/Y/Z axis
  const  int itemPt((item[1]=='/' && item.length()==3) ? 2 : 1);
  const int ptype=static_cast<int>(tolower(item[itemPt])-'x');
  if (ptype<0 || ptype>=3)
    return errAxis;
  double norm[3]={0.0,0.0,0.0};
  double cent[3]={0.0,0.0,0.0};
  norm[ptype]=1.0;

  if (itemPt!=1)
    {
      // get the other two coordinates
      int index((!ptype) ? 1 : 0);
      while(index<3 &&  StrFunc::section(Line,cent[index]))
	{
	  index++;
	  if (index==ptype)
	    index++;
	}
      if (index!=3)
	return errCent;
    }
  // Now get radius
  double R;
  if (!StrFunc::section(Line,R) || R<=0.0)
    return errRadius;

  Centre=Geometry::Vec3D(cent);
  Normal=Geometry::Vec3D(norm);
  Radius=R;
  setBaseEqn();
  return 0;
} 

int 
Cylinder::side(const Geometry::Vec3D& Pt) const 
  /*!
    Calculate if the point PT within the middle
    of the cylinder 
    \retval -1 :: within cylinder 
    \retval 1 :: outside the cylinder
    \retval 0 :: on the surface 
  */
{
  if (Nvec)      // Nvec =1-3 (point to exclude == Nvec-1)
    {
      double x=Pt[Nvec % 3]-Centre[Nvec % 3];
      x*=x;
      double y=Pt[(Nvec+1) % 3]-Centre[(Nvec+1) % 3];;
      y*=y;
      const double displace=x+y-Radius*Radius;
      if (fabs(displace)<CTolerance)
	return 0;
      return (displace>0.0) ? 1 : -1;
    }
  return Surface::side(Pt);
}

int 
Cylinder::onSurface(const Geometry::Vec3D& Pt) const 
  /*!
    Calculate if the point PT on the cylinder 
    \param Pt :: Geometry::Vec3D to test

    \retval 1 :: on the surface 
    \retval 0 :: not on the surface

  */
{
  if (Nvec)      // Nvec =1-3 (point to exclude == Nvec-1)
    {
      double x=Pt[Nvec % 3]-Centre[Nvec % 3];
      x*=x;
      double y=Pt[(Nvec+1) % 3]-Centre[(Nvec+1) % 3];;
      y*=y;
      return (fabs((x+y)-Radius*Radius)>CTolerance*CTolerance) ? 0 : 1;
    }
  return Surface::onSurface(Pt);
}

void
Cylinder::setNvec()
  /*! 
     Find if the normal vector allows it to be a special
     type of cylinder on the x,y or z axis
     \return 1,2,3 :: corresponding to a x,y,z alignment
  */
{
  Nvec=0;
  for(int i=0;i<3;i++)
    {
      if (fabs(Normal[i])>(1.0-CTolerance))
	{
	  Nvec=i+1;
	  return;
	}
    }
  return;
}

void
Cylinder::rotate(const Geometry::Matrix<double>& MA)
/*!
  Apply a rotation to the cylinder and re-check the
  status of the main axis.
  \param MA :: Rotation Matrix (not inverted)
*/
{
  Centre.rotate(MA);
  Normal.rotate(MA);
  Normal.makeUnit();
  setNvec();
  Surface::rotate(MA);
  return;
}

void 
Cylinder::displace(const Geometry::Vec3D& Pt)
  /*!
    Apply a displacement Pt 
    \param Pt :: Displacement to add to the centre
  */ 
{
  if (Nvec)
    {
      Centre[Nvec % 3]+=Pt[Nvec % 3];
      Centre[(Nvec+1) % 3]+=Pt[(Nvec+1) % 3];
    }
  else
    Centre+=Pt;
  Surface::displace(Pt);
  return;
}

void
Cylinder::setCentre(const Geometry::Vec3D& A)
  /*!
    Sets the centre Geometry::Vec3D
    \param A :: centre point 
  */
{
  Centre=A;
  setBaseEqn();
  return;
}

void
Cylinder::setNorm(const Geometry::Vec3D& A)
  /*! 
    Sets the centre line unit vector 
    \param A :: Vector along the centre line 
    A does not need to be a unit vector
  */
{
  Normal=A;
  Normal.makeUnit();
  setBaseEqn();
  return;
}

void
Cylinder::setBaseEqn()
  /*!
    Sets an equation of type (cylinder)
    \f[ Ax^2+By^2+Cz^2+Dxy+Exz+Fyz+Gx+Hy+Jz+K=0 \f]
  */
{
  const double CdotN(Centre.dotProd(Normal));
  BaseEqn[0]=1.0-Normal[0]*Normal[0];     // A x^2
  BaseEqn[1]=1.0-Normal[1]*Normal[1];     // B y^2
  BaseEqn[2]=1.0-Normal[2]*Normal[2];     // C z^2 
  BaseEqn[3]=-2*Normal[0]*Normal[1];     // D xy
  BaseEqn[4]=-2*Normal[0]*Normal[2];     // E xz
  BaseEqn[5]=-2*Normal[1]*Normal[2];     // F yz
  BaseEqn[6]= 2.0*(Normal[0]*CdotN-Centre[0]);  // G x
  BaseEqn[7]= 2.0*(Normal[1]*CdotN-Centre[1]);  // H y
  BaseEqn[8]= 2.0*(Normal[2]*CdotN-Centre[2]);  // J z
  BaseEqn[9]= Centre.dotProd(Centre)-CdotN*CdotN -Radius*Radius;  // K const
  return;
}

double
Cylinder::distance(const Geometry::Vec3D& A) const
  /*!
    Calculates the distance of point A from 
    the surface of the  cylinder.
    \param A :: Point to calculate distance from
    \return :: +ve distance to the surface.

    \todo INCOMPLETE AS Does not deal with the case of 
    non axis centre line  (has been updated?? )
  */
{
  // First find the normal going to the point
  const Geometry::Vec3D Amov=A-Centre;
  double lambda=Amov.dotProd(Normal);
  const Geometry::Vec3D Ccut= Normal*lambda;
  // The distance is from the centre line to the 
  return  fabs(Ccut.Distance(Amov)-Radius);
}

void
Cylinder::write(std::ostream& OX) const
  /*! 
    Write out the cylinder for MCNPX
    \param OX :: output stream
  */
{
  //               -3 -2 -1 0 1 2 3        
  const char Tailends[]="zyx xyz";
  const int Ndir=Normal.masterDir(CTolerance);
  if (Ndir==0)
    {
      // general surface
      Surface::write(OX);
      return;
    }
  
  const int Cdir=Centre.masterDir(CTolerance);
  std::ostringstream cx;

  writeHeader(cx);
  cx.precision(Surface::Nprecision);
  // Name and transform 
   
  if (Cdir*Cdir==Ndir*Ndir || Centre.nullVector(CTolerance))
    {
      cx<<"c";
      cx<< Tailends[Ndir+3]<<" ";          // set x,y,z based on Ndir
      cx<< Radius;
    }
  else
    {
      cx<<" c/";
      cx<< Tailends[Ndir+3]<<" ";          // set x,y,z based on Ndir

      if(Ndir==1 || Ndir==-1)
	cx<< Centre[1] << " " <<Centre[2] << " ";
      else if(Ndir==2 || Ndir==-2)
	cx<< Centre[0] << " " <<Centre[2] << " ";
      else
	cx<< Centre[0] << " " <<Centre[1] << " ";
      
      cx<< Radius;
    }

  StrFunc::writeMCNPX(cx.str(),OX);
  return;
}

double 
Cylinder::lineIntersect(const Geometry::Vec3D& Pt,
			const Geometry::Vec3D& uVec) const
  /*!
    Given a track starting from Pt and traveling along
    uVec determine the intersection point (distance)
    \param Pt :: Point of track start
    \param uVec Unit vector of length
    \retval Distance to intersect
    \retval -1 Failed to intersect
  */
{
  return -1;
}


void
Cylinder::print() const
 /*!
   Debug routine to print out basic information 
 */
{
  Surface::print();
  std::cout<<"Axis =="<<Normal<<" ";
  std::cout<<"Centre == "<<Centre<<" ";
  std::cout<<"Radius == "<<Radius<<std::endl;
  return;
}

int
Cylinder::importXML(IndexIterator<XML::XMLobject,XML::XMLgroup>& SK,
		    const int singleFlag)
  /*!
    Given a distribution that has been put into an XML base set.
    The XMLcollection need to have the XMLgroup pointing to
    the section for this Cylinder.

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
	      else if (KVal=="Radius")
		errNum=(StrFunc::convert(RPtr->getFront(),Radius)) ? 0 : 1;
	      else if (KVal=="Nvec")
		errNum=(StrFunc::convert(RPtr->getFront(),Nvec)) ? 0 : 1;
	      else
		errNum=Surface::importXML(SK,1);
	    }
	  if (errNum)
	    {
	      errCnt++;                 // Not good....
	      ELog::EMessages.Estream()
		<<"Cylinder::importXML :: Failed on key: "<<KVal;
	      ELog::EMessages.report(2);
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
Cylinder::procXML(XML::XMLcollect& XOut) const
  /*!
    This writes the XML schema
    \param XOut :: Output parameter
   */
{
  XOut.getCurrent()->addAttribute("type",std::string("Cylinder"));
  Surface::procXML(XOut);
  XOut.addComp("Centre",Centre);
  XOut.addComp("Normal",Normal);
  XOut.addComp("Nvec",Nvec);
  XOut.addComp("Radius",Radius);
  return;
}

}   // NAMESPACE MonteCarlo


}  // NAMESPACE Mantid
