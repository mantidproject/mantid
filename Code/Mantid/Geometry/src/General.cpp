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
#include "Matrix.h"
#include "Vec3D.h"
#include "BaseVisit.h"
#include "Surface.h"
#include "General.h"

namespace Mantid
{

namespace Geometry
{

Kernel::Logger& General::PLog(Kernel::Logger::get("General"));

const double GTolerance(1e-6);

General::General() : Surface()
  /*!
    Standard Constructor
  */
{
  setBaseEqn();
}

General::General(const General& A) : 
  Surface(A)
  /*!
    Standard Copy Constructor
    \param A :: General Object to copy
  */
{}

General*
General::clone() const
  /*!
    Makes a clone (implicit virtual copy constructor) 
    \return General(this)
  */
{
  return new General(*this);
}

General&
General::operator=(const General& A)
  /*!
    Standard assignment operator
    \param A :: General Object to copy
    \return *this
  */
{
  if (this!=&A)
    {
      Surface::operator=(A);
    }
  return *this;
}

General::~General()
  /*!
    Destructor
  */
{}


int 
General::setSurface(const std::string& Pstr)
  /*! 
    Processes a standard MCNPX general string (GQ/SQ types)
    Despite type, moves both to the general equation.
    \param Pstr :: String to process (with name and transform)
    \return 0 on success, neg of failure
  */
{
  std::string Line=Pstr;
  std::string item;
  if (!StrFunc::section(Line,item) || item.length()!=2 ||
      (tolower(item[0])!='g' &&  tolower(item[0]!='s')) ||
      tolower(item[1])!='q')
    return -1;
      
  double num[10];
  int index;
  for(index=0;index<10 && 
	StrFunc::section(Line,num[index]);index++);
  if (index!=10)
    return -2;

  if (tolower(item[0])=='g')
    {
      for(int i=0;i<10;i++)
	Surface::BaseEqn[i]=num[i];
    }
  else
    {
      Surface::BaseEqn[0]=num[0];
      Surface::BaseEqn[1]=num[1];
      Surface::BaseEqn[2]=num[2];
      Surface::BaseEqn[3]=0.0;;
      Surface::BaseEqn[4]=0.0;;
      Surface::BaseEqn[5]=0.0;;
      Surface::BaseEqn[6]=2*(num[3]-num[7]*num[0]);
      Surface::BaseEqn[7]=2*(num[4]-num[8]*num[1]);
      Surface::BaseEqn[8]=2*(num[5]-num[9]*num[2]);
      Surface::BaseEqn[9]=num[0]*num[7]*num[7]+
	num[1]*num[8]*num[8]+num[2]*num[9]*num[9]-
	2.0*(num[3]*num[7]+num[4]*num[8]+num[5]*num[9])+
	num[6];
    }
  return 0;
  
}

void 
General::setBaseEqn()
  /*!
    Set baseEqn (nothing to do) as it is 
    already a baseEqn driven system
  */
{
  return;
}

int
General::importXML(IndexIterator<XML::XMLobject,XML::XMLgroup>& SK,
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
	      errNum=Surface::importXML(SK,1);
	    }
	  if (errNum)
	    {
	      errCnt++;                 // Not good....
	      PLog.warning("importXML :: Key failed "+KVal);
	    }
	  // Post processing
	  if (!singleFlag) SK++;
	}
    } while (!singleFlag && SK.getLevel()>=levelExit);

  return errCnt;
}

void
General::procXML(XML::XMLcollect& XOut) const
  /*!
    This writes the XML schema
    \param XOut :: Output parameter
   */
{
  XOut.getCurrent()->addAttribute("type","General");
  Surface::procXML(XOut);
  return;
}

}  // NAMESPACE MonteCarlo

}  // NAMESPACE Mantid
