#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <complex>
#include <cmath>
#include <list>
#include <vector>
#include <map>
#include <stack>
#include <string>
#include <algorithm>

#include "AuxException.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"
#include "XMLattribute.h"
#include "XMLobject.h"
#include "XMLgroup.h"
#include "XMLread.h" 
#include "XMLcollect.h" 
#include "IndexIterator.h"
#include "MantidKernel/Support.h"
#include "mathSupport.h"
#include "MantidGeometry/Matrix.h"
#include "MantidGeometry/V3D.h"
#include "MantidGeometry/PolyBase.h"
#include "MantidGeometry/BaseVisit.h"
#include "MantidGeometry/Surface.h"

namespace Mantid
{

namespace Geometry
{

Kernel::Logger& Surface::PLog = Kernel::Logger::get("Surface"); 

/// Tolerance for floating point
const double STolerance(1e-6);

Surface::Surface() : 
  Name(-1)
  /*!
    Constructor
  */
{}

Surface::Surface(const Surface& A) : 
  Name(A.Name)
  /*!
    Copy constructor
    \param A :: Surface to copy
  */
{ }


Surface&
Surface::operator=(const Surface& A)
  /*!
    Assignment operator
    \param A :: Surface to copy
    \return *this
  */
{
  if (this!=&A)
    {
      Name=A.Name;
    }
  return *this;
}

Surface::~Surface()
  /*!
    Destructor
  */
{}

int
Surface::side(const Geometry::V3D&) const
  /// Surface side : throw AbsObjMethod
{
  throw Kernel::Exception::AbsObjMethod("Surface::side");
}

void 
Surface::print() const
  /*!
    Simple print out function for surface header 
  */
{
  std::cout<<"Surf == "<<Name<<std::endl;
  return;
}

void
Surface::writeHeader(std::ostream& OX) const
  /*!
    Writes out the start of an MCNPX surface description .
    Does not check the length etc
    \param OX :: Output stream
  */
{
  OX<<Name<<" ";
  return;
}
  
int
Surface::importXML(IndexIterator<XML::XMLobject,XML::XMLgroup>& SK,
		   const int singleFlag)
  /*!
    Given a distribution that has been put into an XML base set.
    The XMLcollection need to have the XMLgroup pointing to
    the section for this Surface. 

    \param SK :: IndexIterator scheme
    \param singleFlag :: Single pass identifer [expected 1 ]
    \return Error count
   */
{
  int errCnt(0);
  int levelExit(SK.getLevel());
  do
    {
      if (*SK)
        {
	  int errNum(1);
	  const std::string& KVal= SK->getKey();
//	  const std::string KBase= SK->getKeyBase();
//	  const int Knum =  SK->getKeyNum();
	  const XML::XMLread* RPtr=dynamic_cast<const XML::XMLread*>(*SK);
	  
	  if (RPtr)
	    {
	      if (KVal=="Name")
		errNum=1-StrFunc::convert(RPtr->getFront(),Name);
	    }
	  // Failure test:
	  if (errNum)
	    {
	      PLog.warning("importXML :: Key failed "+KVal);
	      errCnt++;
	    }
	}
      if (!singleFlag) SK++;
    } while (!singleFlag && SK.getLevel()>=levelExit);
  
  return errCnt;
}

void
Surface::procXML(XML::XMLcollect& XOut) const
  /*!
    This writes the XML schema
    \param XOut :: Output parameter
   */
{
  XOut.addComp("Name",Name);
  return;
}

void
Surface::writeXML(const std::string& Fname) const
  /*!
    The generic XML writing system.
    This should call the virtual function procXML
    to correctly built the XMLcollection.
    \param Fname :: Filename 
  */
{
  XML::XMLcollect XOut;
  XOut.addGrp("Surface");
  XOut.addAttribute("type","Surface");
  this->procXML(XOut);          
  XOut.closeGrp();
  std::ofstream OX;
  OX.open(Fname.c_str());
  XOut.writeXML(OX);
  return;
}

void
Surface::write(std::ostream&) const
{
  throw Kernel::Exception::AbsObjMethod("Surface::write");
}
  
  
}  // NAMESPACE Geometry

} // NAMESPACE Mantid




