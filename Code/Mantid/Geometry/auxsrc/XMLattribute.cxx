#include <iostream>
#include <cmath>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <functional>
#include <algorithm>
#include <iterator>
#include <time.h>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

#include "Exception.h"
#include "RefControl.h"
#include "Matrix.h"
#include "support.h"
#include "XMLnamespace.h"
#include "XMLattribute.h"

std::ostream& 
XML::operator<<(std::ostream& OX,const XMLattribute& A)
  /*!
    Calls XMLattribute method write to output class
    \param of :: Output stream
    \param A :: XMLattribute to write
    \return Current state of stream
  */
{
  A.writeXML(OX);
  return OX;
}

namespace XML
{

// --------------------------------------------------------
//                   XMLattribute
// --------------------------------------------------------

XMLattribute::XMLattribute() :
  empty(1)
  /*!
    Constructor
  */
{}

XMLattribute::XMLattribute(const std::string& A,
			   const std::string& B) :
  empty(0)
  /*!
    Constructor (with single attribute)
    \param A :: Name of unit
    \param B :: Values
  */
{
  AObj.push_back(A);
  Val.push_back(B);
}

XMLattribute::XMLattribute(const XMLattribute& A) :
  empty(A.empty),AObj(A.AObj),Val(A.Val)
  /*!
    Copy Constructor
    \param A :: XMLattribute to copy
  */
{}

XMLattribute&
XMLattribute::operator=(const XMLattribute& A) 
  /*!
    Assignment operator
    \param A :: XMLattribute to copy
    \return *this
  */
{
  if (this!=&A)
    {
      empty=A.empty;
      AObj=A.AObj;
      Val=A.Val;
    }
  return *this;
}

XMLattribute::~XMLattribute()
  /*!
    Destructor
  */
{}

void
XMLattribute::addAttribute(const std::string& Name,
			   const std::string& V)
  /*!
    Adds a value to the attributions. Clears
    the empty flag.
    \param A :: Unit name
    \param V :: Attribute Value
   */

{
  empty=0;
  AObj.push_back(Name);
  Val.push_back(V);
  return;
}

void
XMLattribute::addAttribute(const std::vector<std::string>& Vec)
  /*!
    Adds an attriubte to the main system.
    \param Vec :: list of string compents in the form key="value"
   */
    
{
  std::string Key,Atb;
  std::vector<std::string>::const_iterator vc;
  for(vc=Vec.begin();vc!=Vec.end();vc++)
    {
      std::string Part=*vc;
      if (splitAttribute(Part,Key,Atb)>0)
	addAttribute(Key,Atb);
    }
  return;
}

int
XMLattribute::setAttribute(const std::string& Key,const std::string& Value)
  /*!
    Set a given attribute 
    \param Key :: Key name
    \param Value :: Value of attribute
    \retval 0 :: success
    \retval -1 :: Attribute does not exists
   */
{
  const int item=hasAttribute(Key);
  if (!item)
    return -1;

  Val[item-1]=Value;
  return 0;
}

int
XMLattribute::hasAttribute(const std::string& Name) const
  /*!
    Determines if an attribute exists 
    \param Name :: Attribute to find
    \retval 0 :: No Attribute found
    \retval Index+1 :: Attribute of index
   */
{
  if (empty)
    return 0;

  std::vector<std::string>::const_iterator vc;
  vc=find(AObj.begin(),AObj.end(),Name);
  if (vc!=AObj.end())
    return 1+distance(AObj.begin(),vc);
  return 0;
}

std::string
XMLattribute::getAttribute(const std::string& Name) const
  /*!
    Determines if an attribute exists and return value
    \param Name :: Attribute to find
    \return Value object 
   */

{
  const int Index=hasAttribute(Name);
  if (!Index)
    return Val[Index-1];
  return "";
}

void
XMLattribute::writeXML(std::ostream& OX) const
  /*!
    Write out to the stream
    \param OX :: Output stream
   */
{
  if (!empty)
    {
      for(int i=0;i<static_cast<int>(AObj.size());i++)
	OX<<" "<<AObj[i]<<"=\""<<Val[i]<<"\"";
    }
  return;
}

}  //NAMESPACE XML

