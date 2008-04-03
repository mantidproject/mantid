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
#include "AuxException.h"
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
#include "Sphere.h"

namespace Mantid
{

namespace Geometry
{
 
Kernel::Logger& Sphere::PLog(Kernel::Logger::get("Sphere"));

/// @cond
const double STolerance(1e-6);
/// @endcond

Sphere::Sphere() : Surface(),
  Centre(0,0,0),Radius(0.0)
  /*!
    Default constructor 
    make sphere at the origin radius zero 
  */
{
  setBaseEqn();
}
  
Sphere::Sphere(const Sphere &A) : 
  Surface(A),Centre(A.Centre),Radius(A.Radius)
  /*!
    Default Copy constructor 
    \param A :: Sphere to copy
  */
{ }

Sphere*
Sphere::clone() const
  /*!
    Makes a clone (implicit virtual copy constructor) 
    \return new (*this)
  */
{
  return new Sphere(*this);
}

Sphere&
Sphere::operator=(const Sphere &A) 
  /*!
    Default Assignment operator
    \param A :: Sphere to copy
    \return *this
  */
{
  if (this!=&A)
    {
      Surface::operator=(A);
      Centre=A.Centre;
      Radius=A.Radius;
    }
  return *this;
}

Sphere::~Sphere()
  /*!
    Destructor
  */
{}

int
Sphere::setSurface(const std::string& Pstr)
  /*! 
    Processes a standard MCNPX cone string    
    Recall that cones can only be specified on an axis
     Valid input is: 
     - so radius 
     - s cen_x cen_y cen_z radius
     - sx - cen_x radius
    \return : 0 on success, neg of failure 
  */
{
  std::string Line=Pstr;
  std::string item;
  if (!StrFunc::section(Line,item) || 
      tolower(item[0])!='s' || item.length()>2)
    return -1;

  double cent[3]={0,0,0};
  double R;
  if (item.length()==2)       // sx/sy/sz
    {
      if (tolower(item[1])!='o')
        {
	  const int pType=static_cast<int>(tolower(item[1])-'x');
	  if (pType<0 || pType>2)
	    return -3;
	  if (!StrFunc::section(Line,cent[pType]))
	    return -4;
	}
    }
  else if (item.length()==1)
    {
      int index;
      for(index=0;index<3 && StrFunc::section(Line,cent[index]);
	  index++);
      if (index!=3)
	return -5;
    }
  else
    return -6;
  if (!StrFunc::section(Line,R))
    return -7;

  Centre=Geometry::Vec3D(cent);
  Radius=R;
  setBaseEqn();
  return 0;
} 


int
Sphere::side(const Geometry::Vec3D& Pt) const
  /*!
     Calculate where the point Pt is relative to the 
     sphere.
     \param Pt :: Point to test
     \retval -1 :: Pt within sphere
     \retval 0 :: point on the surface (within CTolerance)
     \retval 1 :: Pt outside the sphere 
  */
{
  const Geometry::Vec3D Xv=Pt-Centre;
  if (onSurface(Pt)==1)
  {
    return 0;
  }
  return (Xv.dotProd(Xv)>Radius*Radius) ? 1 : -1;
}

int
Sphere::onSurface(const Geometry::Vec3D& Pt) const
  /*!
    Calculate if the point Pt on the surface of the sphere
    (within tolerance CTolerance)
    \param Pt :: Point to check
    \return 1 :: on the surfacae or 0 if not.
  */
{
  const Geometry::Vec3D Xv=Pt-Centre;
  return (fabs(Xv.dotProd(Xv)-Radius*Radius)>STolerance*STolerance) ? 0 : 1;
}

double
Sphere::distance(const Geometry::Vec3D& Pt) const
  /*! 
    Determine the shortest distance from the Surface 
    to the Point. 
    \param Pt :: Point to calculate distance from
    \return distance (Positive only)
  */
{
  const Geometry::Vec3D Amov=Pt-Centre;
  return fabs(Amov.abs()-Radius);
}


void
Sphere::displace(const Geometry::Vec3D& Pt) 
  /*!
    Apply a shift of the centre
    \param Pt :: distance to add to the centre
  */
{
  Centre+=Pt;
  Surface::displace(Pt);
  return;
}

void
Sphere::rotate(const Geometry::Matrix<double>& MA) 
  /*!
    Apply a Rotation matrix
    \param MA :: matrix to rotate by
  */
{
  Centre.rotate(MA);
  Surface::rotate(MA);
  return;
}

void 
Sphere::setCentre(const Geometry::Vec3D& A)
  /*!
    Set the centre point
    \param A :: New Centre Point
  */
{
  Centre=A;
  setBaseEqn();
  return;
}

void 
Sphere::setBaseEqn()
  /*!
    Sets an equation of type (general sphere)
    \f[ x^2+y^2+z^2+Gx+Hy+Jz+K=0 \f]
  */
{
  BaseEqn[0]=1.0;     // A x^2
  BaseEqn[1]=1.0;     // B y^2
  BaseEqn[2]=1.0;     // C z^2 
  BaseEqn[3]=0.0;     // D xy
  BaseEqn[4]=0.0;     // E xz
  BaseEqn[5]=0.0;     // F yz
  BaseEqn[6]= -2.0*Centre[0];     // G x
  BaseEqn[7]= -2.0*Centre[1];     // H y
  BaseEqn[8]= -2.0*Centre[2];     // J z
  BaseEqn[9]= Centre.dotProd(Centre)-Radius*Radius;        // K const
  return;
}


void
Sphere::procXML(XML::XMLcollect& XOut) const
  /*!
    This writes the XML schema
    \param XOut :: Output parameter
   */
{
  XOut.getCurrent()->addAttribute("type","Sphere");
  Surface::procXML(XOut);
  XOut.addComp("Centre",Centre);
  XOut.addComp("Radius",Radius);
  return;
}

int
Sphere::importXML(IndexIterator<XML::XMLobject,XML::XMLgroup>& SK,
		  const int singleFlag)
  /*!
    Given a distribution that has been put into an XML base set.
    The XMLcollection need to have the XMLgroup pointing to
    the section for this Sphere.

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
	      else if (KVal=="Radius")
		errNum=(StrFunc::convert(RPtr->getFront(),Radius)) ? 0 : 1;
	      else
		errNum=Surface::importXML(SK,1);
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
Sphere::write(std::ostream& OX) const
  /*! 
    Object of write is to output a MCNPX plane info 
    \param OX :: Output stream (required for multiple std::endl)  
    \todo (Needs precision) 
  */
{
  std::ostringstream cx;
  Surface::writeHeader(cx);
  cx.precision(Surface::Nprecision);
  if (Centre.Distance(Geometry::Vec3D(0,0,0))<STolerance)
    {
      cx<<"so "<<Radius;
    }
  else
    {
      cx<<"s "<<Centre<<" "<<Radius;
    }
  StrFunc::writeMCNPX(cx.str(),OX);
  return;
}

}

}  // NAMESPACE Mantid
