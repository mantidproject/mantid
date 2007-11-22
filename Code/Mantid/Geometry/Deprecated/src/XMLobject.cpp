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

#include "Logger.h"
#include "AuxException.h"
#include "RefControl.h"
#include "Matrix.h"
#include "Vec3D.h"
#include "Support.h"
// #include "SpecDataHold.h"
#include "XMLattribute.h"
#include "XMLobject.h"
#include "XMLgroup.h"
#include "XMLcomp.h"

namespace Mantid
{

std::ostream& 
XML::operator<<(std::ostream& OX,const XMLobject& A)
  /*!
    Calls XMLattribute method write to output class
    \param of :: Output stream
    \param A :: XMLattribute to write
    \return Current state of stream
  */
{
  (&A)->writeXML(OX);
  return OX;
}

namespace XML
{

XMLobject::XMLobject(XMLobject* B) : 
  depth(0),repNumber(0),Base(B)
  /*!
    Default constructor
  */
{}

XMLobject::XMLobject(XMLobject* B,const std::string& K) :
  depth(0),loaded(0),repNumber(0),Key(K),Base(B)
  /*!
    Default constructor (with key)
    \param K :: Key value
  */
{}

XMLobject::XMLobject(XMLobject* B,const std::string& K,const int GN) :
  depth(0),loaded(0),repNumber(GN),Key(K),Base(B)
  /*!
    Default constructor (with key)
    \param K :: Key value
  */

{}

XMLobject::XMLobject(const XMLobject& A) :
  depth(A.depth),loaded(A.loaded),
  repNumber(A.repNumber),Key(A.Key),Attr(A.Attr),Base(A.Base)
  /*!
    Copy constructor for use by derived classes
    \param A :: object to copy
  */
{}


XMLobject&
XMLobject::operator=(const XMLobject& A) 
  /*!
    Standard assignment operator (for use
    by derived classes)
    \param A :: Base object to copy
    \return *this
  */
{
  if (this!=&A)
    {
      depth=A.depth;
      loaded=A.loaded;
      repNumber=A.repNumber;
      Key=A.Key;
      Attr=A.Attr;
      Base=A.Base;
    }
  return *this;
}

XMLobject*
XMLobject::clone() const
  /*!
    Clone function
    \return new XMLobject copy
  */
{
  return new XMLobject(*this);
}

void
XMLobject::addAttribute(const std::string& Name,
			const char* Val)
  /*!
    Adds an attriubte to the main system.
    \param Name :: Key name
    \param Val :: Value
   */
    
{
  Attr.addAttribute(Name,std::string(Val));
  return;
}

template<>
void
XMLobject::addAttribute(const std::string& Name,
			const std::string& Val)
  /*!
    Adds an attriubte to the main system.
    \param Name :: Key name
    \param Val :: Value
   */
    
{
  Attr.addAttribute(Name,Val);
  return;
}

template<typename T>
void
XMLobject::addAttribute(const std::string& Name,
			const T& Val)
  /*!
    Adds an attriubte to the main system.
    \param Name :: Key name
    \param Val :: Object that has a conversion to an outputstream
   */
{
  std::ostringstream cx;
  cx<<Val;
  Attr.addAttribute(Name,cx.str());
  return;
}

void
XMLobject::addAttribute(const std::vector<std::string>& Vec)
  /*!
    Adds an attriubte to the main system.
    \param Vec :: list of string compents in the form key="value"
   */
    
{
  Attr.addAttribute(Vec);
  return;
}

int
XMLobject::setAttribute(const std::string& Key,const std::string& Value)
  /*!
    Set attribute 
    \param Key :: Key Name
    \param Value :: Value size
    \retval 0 :: success
    \retval -1 :: failure 
   */
{
  return Attr.setAttribute(Key,Value);
}

int
XMLobject::hasAttribute(const std::string& Name) const
  /*!
    Given an attribute, find out if we have it
    \retval Index+1 if Name is found
    \retval 0 :: failure
   */
    
{
  return Attr.hasAttribute(Name);
}

std::string
XMLobject::getAttribute(const std::string& Name) const
  /*!
    Given an attribute, find out if we have it
    returns attribute
   */
    
{
  return Attr.getAttribute(Name);
}

std::string
XMLobject::getFullKey() const
  /*!
    Recursive system to build the full key
    \return full deliminated key
  */
{
  return (Base) ? Base->getFullKey()+"/"+Key : Key;
}  


std::string
XMLobject::getKeyBase() const
  /*!
    This is not really correct since assume _ is unique
    \return base string
   */
{
  int pos=Key.size();  
  if (!pos)
    return Key;  // this will be null.
  for(pos--;pos>=0 && isdigit(Key[pos]);pos--);
  return Key.substr(0,pos+1);
}

int
XMLobject::getKeyNum() const
  /*!
    This extacts the last readable integer from the 
    key name.
    \retval Number on success
    \retval -1 :: failure
    
   */
{
  int pos=Key.size();  
  if (!pos) return -1; 
  for(pos--;pos>=0 && isdigit(Key[pos]);pos--);
  int out;
  if (StrFunc::convert(Key.substr(pos+1),out))
    return out;
  return -1; 
}

int 
XMLobject::readObject(std::istream&)
{
  return -1;
}

void
XMLobject::writeDepth(std::ostream& OX) const
  /*!
    Horrible function to write out to the
    stream the correct amount of spacitee
    \param OX :: stream to use
  */
{
  if (depth<1)
    return;
  OX<<std::string(depth,' ');
  return;
}

std::string
XMLobject::getCurrentFile(const int depth) const
  /*!
    Get a string based on 
    -XMLparentKey_XMLkey_rRepeatNumber
    \return string values
  */
{
  const XMLobject* Parent=this;
  std::string Name;
  for(int i=0;i<depth && Parent;i++)
    {
      Name=(i) ? Parent->getKey()+"_"+Name : Parent->getKey();
      Parent=Parent->getParent();
    }

  if (repNumber)
    {
      std::ostringstream cx;
      cx<<Name<<"_r"<<repNumber<<std::endl;
      return cx.str();
    }
  return Name;
}


template<typename T>
const T&
XMLobject::getValue(const T& DefValue) const
  /*!
    Given an object convert into a value based on a default
    value or return the default value
  */
{
  const  XML::XMLcomp<T>* CPobj=
    dynamic_cast<const XML::XMLcomp<T>* >(this);
  if (CPobj && !CPobj->isEmpty())
    return CPobj->getValue();
  // Ok but maybe convert an XMLread to a value object
  

  return DefValue;
}


}   // NAMESPACE XML


/// \cond TEMPLATE
template const double& XML::XMLobject::getValue(const double&) const;
template const int& XML::XMLobject::getValue(const int&) const;
template const Geometry::Vec3D& XML::XMLobject::getValue(const Geometry::Vec3D&) const;

template void XML::XMLobject::addAttribute(const std::string&,const double&);
template void XML::XMLobject::addAttribute(const std::string&,const int&);
template void XML::XMLobject::addAttribute(const std::string&,const Geometry::Vec3D&);

/// \endcond TEMPLATE

} // NAMESPACE Mantid
