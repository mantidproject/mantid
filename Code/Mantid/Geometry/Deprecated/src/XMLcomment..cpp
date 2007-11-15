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

#include "AuxException.h"
#include "RefControl.h"
#include "Matrix.h"
#include "Vec3D.h"
#include "support.h"
// #include "SpecDataHold.h"
#include "XMLnamespace.h"
#include "XMLattribute.h"
#include "XMLobject.h"
#include "XMLgroup.h"
#include "XMLcomment.h"

namespace XML
{

// --------------------------------------------------------
//                   XMLcomment
// --------------------------------------------------------


XMLcomment::XMLcomment(XMLobject* B,const std::string& K,
		       const std::string& Line) :
  XMLobject(B,K),empty(1)
  /*!
    Constructor with junk key (value is NOT set)
    \param K :: key
  */
{
  addLine(Line);
}

XMLcomment::XMLcomment(XMLobject* B,const std::string& Line) :
  XMLobject(B,"comment"),empty(1)
  /*!
    Constructor with junk key (value is NOT set)
    \param K :: key
  */
{
  addLine(Line);
}

XMLcomment::XMLcomment(XMLobject* B,const std::string& K,
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

XMLcomment::XMLcomment(XMLobject* B,const std::vector<std::string>& V) :
  XMLobject(B,"comment"),empty(0)
  /*!
    Constructor with Key and Value
    \param K :: key
    \param V :: Value to set
  */
{
  Comp.resize(V.size());
  copy(V.begin(),V.end(),Comp.begin());
}

XMLcomment::XMLcomment(const XMLcomment& A) :
  XMLobject(A),empty(A.empty),Comp(A.Comp)
  /*!
    Standard copy constructor
    \param A :: XMLcomment to copy
  */
{}

XMLcomment&
XMLcomment::operator=(const XMLcomment& A) 
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

XMLcomment*
XMLcomment::clone() const
  /*!
    The clone function
    \return new copy of this
  */
{
  return new XMLcomment(*this);
}

XMLcomment::~XMLcomment()
  /*!
    Standard destructor
  */
{ }

std::string&
XMLcomment::getFront()
  /*!
    Gets the first item in the stack
    \retval Front object 
    \throw ExBasy if stack empty
  */
{
  if (Comp.empty())
    throw ColErr::ExBase("XMLcomment::getFront : Stack empty");
  return Comp.front();
}

const std::string&
XMLcomment::getFront() const
  /*!
    Gets the first item in the stack
    \retval Front object 
    \throw ExBase if stack empty
  */
{
  if (Comp.empty())
    throw ColErr::ExBase("XMLcomment::getFront : Stack empty");
  
  return Comp.front();
}

void 
XMLcomment::addLine(const std::string& Line)
  /*!
    Adds a line to the system
    \param Line :: Line to add
  */
{
  Comp.push_back(Line);
  empty=0;
  return;
}

void
XMLcomment::setObject(const std::vector<std::string>& V) 
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
XMLcomment::pop()
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
XMLcomment::writeXML(std::ostream& OX) const
  /*!
    Write out the object. This code is specialised
    to allow multiple "identical" group component names.
    \param OX :: output stream
  */
{
  if (empty || Comp.empty())
    return;
  writeDepth(OX);
  OX<<"<!--";
  // Remove difficult to parse symbols:
  CStore::const_iterator vc=Comp.begin();
  OX<<XML::procString(*vc);

  for(vc++;vc!=Comp.end();vc++)
    {
      OX<<std::endl;
      writeDepth(OX);
      OX<<"   ";
      OX<<XML::procString(*vc);
    }

  OX<<"--!>"<<std::endl;
  return;
}
  
};


