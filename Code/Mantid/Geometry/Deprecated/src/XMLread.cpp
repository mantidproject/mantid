#include <iostream>
#include <time.h>
#include <cmath>
#include <vector>
#include <list>
#include <map>
#include <string>
#include <sstream>
#include <functional>
#include <iterator>
#include <boost/shared_ptr.hpp>

#include "MantidKernel/Logger.h"
#include "AuxException.h"
#include "MantidGeometry/Matrix.h"
#include "Vec3D.h"
#include "MantidKernel/Support.h"
#include "XMLnamespace.h"
#include "XMLattribute.h"
#include "XMLobject.h"
#include "XMLgroup.h"
#include "XMLread.h"

namespace Mantid
{

namespace XML
{

// --------------------------------------------------------
//                   XMLread
// --------------------------------------------------------


XMLread::XMLread(XMLobject* B,const std::string& K) :
  XMLobject(B,K),empty(1)
  /*!
    Constructor with junk key (value is NOT set)
    \param K :: key
  */
{}

XMLread::XMLread(XMLobject* B,const std::string& K,
		 const std::vector<std::string>& V) :
  XMLobject(B,K),empty(0)
  /*!
    Constructor with Key and Value
    \param K :: key
    \param V :: Value to set
  */
{
  Comp.resize(V.size());
  copy(V.begin(),V.end(),Comp.begin());
    
}

XMLread::XMLread(const XMLread& A) :
  XMLobject(A),empty(A.empty),Comp(A.Comp)
  /*!
    Standard copy constructor
    \param A :: XMLread to copy
  */
{}

XMLread&
XMLread::operator=(const XMLread& A) 
  /*!
    Standard assignment operator
    \param A :: Object to copy
    \return *this
  */
{
  if (this!=&A)
    {
      XMLobject::operator=(A);
      empty=A.empty;
      Comp=A.Comp;
    }
  return *this;
}

XMLread*
XMLread::clone() const
  /*!
    The clone function
    \return new copy of this
  */
{
  return new XMLread(*this);
}

XMLread::~XMLread()
  /*!
    Standard destructor
  */
{ }

std::string&
XMLread::getFront()
  /*!
    Gets the first item in the stack
    \retval Front object 
    \throw ExBase if stack empty
  */
{
  if (Comp.empty())
    throw ColErr::ExBase("XMLread::getFront : Stack empty");
  return Comp.front();
}

const std::string&
XMLread::getFront() const
  /*!
    Gets the first item in the stack
    \retval Front object 
    \throw ExBase if stack empty
  */
{
  if (Comp.empty())
    throw ColErr::ExBase("XMLread::getFront : Stack empty");
  
  return Comp.front();
}

std::string
XMLread::getFullString() const
  /*!
    Get the whole string
    \retval Full line from the list
  */
{
  std::string Out;
  if (empty || Comp.empty() )
    return Out;
  std::list<std::string>::const_iterator vc;
  for(vc=Comp.begin();vc!=Comp.end();vc++)
    Out+=(*vc);
  return Out;
}

void 
XMLread::addLine(const std::string& Line)
  /*!
    Adds a line to the system
    \param Line :: Line to add
  */
{
  Comp.push_back(Line);
  return;
}


template<template<typename T,typename A> class V,typename T,typename A> 
int
XMLread::convertToContainer(V<T,A>& CT) const
  /*!
    Given a container of type V<T>, convert all of the
    lines to single objects in the system
    \param CT :: Container to fill.
    \return Number of items passed to the string
  */
{
  CT.erase(CT.begin(),CT.end());
  CStore::const_iterator vc;
  for(vc=Comp.begin();vc!=Comp.end();vc++)
    {
      std::string Line = *vc;
      T tmpObj;
      while(StrFunc::section(Line,tmpObj))
	CT.push_back(tmpObj);
    }
  // Clear points to object.
  return CT.size();
}

template<template<typename T,typename A> class V,typename T,typename A> 
int
XMLread::convertToContainer(const int dmp,V<T,A>& CA,V<T,A>& CB) const
  /*!
    Given a container of type V<T>, convert all of the
    lines to single objects in the system
    \param dmp :: Placeholder for dumped item
    \param CA :: Container to fill.
    \param CB :: Container to fill.
    \return Number of items passed to the string
  */
{
  CA.erase(CA.begin(),CA.end());
  CB.erase(CB.begin(),CB.end());
  CStore::const_iterator vc;
  int dflag(0);
  int item(0);   
  for(vc=Comp.begin();vc!=Comp.end();vc++)
    {
      std::string Line = *vc;
      T tmpObj;
      while(StrFunc::section(Line,tmpObj))
        {
	  if (dflag!=dmp) // not dump point
	    {
	      if (!item)
		CA.push_back(tmpObj);
	      else
		CB.push_back(tmpObj);
	      item=1-item;
	    }
	  dflag++;
	  dflag %= 3;
	}
    }
  if (CA.size()!=CB.size())
    CB.push_back(CA.back());
  
  // Clear points to object.
  return CB.size();
}

template<typename T> 
int
XMLread::convertToObject(T& OT) const
  /*!
    Convert the XMLread string into an object
    \param OT :: Object to fill.
    \retval 1 :: success
    \retval 0 :: failure
  */
{
  std::ostringstream cx;
  CStore::const_iterator vc;
  for(vc=Comp.begin();vc!=Comp.end();vc++)
    cx<<*vc<<" ";
  return StrFunc::convert(cx.str(),OT);
}

void
XMLread::setObject(const std::vector<std::string>& V) 
  /*!
    Sets all the objects based on a vector
    \param V :: Lines of string to add
   */
{
  Comp.clear();

  if (!V.empty())
    {
      Comp.resize(V.size());
      copy(V.begin(),V.end(),Comp.begin());
      empty=0;
    }
  else
    empty=1;
  return;
}

int
XMLread::pop()
  /*!
    Takes the front item of the list
    \retval 0 :: Items still left
    \retval 1 :: Last item
  */
{
  Comp.pop_front();
  return (Comp.empty()) ? 1 : 0;
}

void
XMLread::writeXML(std::ostream& OX) const
  /*!
    Write out the object. This code is specialised
    to allow multiple "identical" group component names.
    The % tag field is used to keep "real" items apart.
    \param OX :: output stream
  */
{
  writeDepth(OX);
  std::string::size_type pos=Key.find('%');
  std::string KeyOut( (pos!=std::string::npos)  ? Key.substr(0,pos) : Key);
  if (empty)
    {
      OX<<"<"<<KeyOut<<Attr<<"/>"<<std::endl;
      return;
    }      
  OX<<"<"<<KeyOut<<Attr<<">";
  if (Comp.size()==1)          // special since will only put on one line
    {
      OX<<Comp.front();
    }
  else               
    {
      OX<<std::endl;
      // Remove difficult to parse symbols:
      CStore::const_iterator vc;
      for(vc=Comp.begin();vc!=Comp.end();vc++)
        {
	 writeDepth(OX);
	 OX<<"  ";         // +2 dept
	 OX<<XML::procString(*vc)<<std::endl;
	}
      writeDepth(OX);
    }
  // Out Key
  OX<<"</"<<KeyOut<<">"<<std::endl;
  return;
}

/*!
\cond TEMPLATE
*/

template int XMLread::convertToObject(Geometry::Vec3D&) const;
template int XMLread::convertToContainer(std::vector<int>&) const;
template int XMLread::convertToContainer(std::vector<double>&) const;
template int XMLread::convertToContainer(const int,std::vector<double>&,
					 std::vector<double>&) const;

/*!
\endcond TEMPLATE
*/
  
}  // NAMESPACE XML

}  // NAMESPACE Mantid

