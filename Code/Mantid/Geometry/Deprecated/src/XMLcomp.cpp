#include <iostream>
#include <cmath>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <functional>
#include <iterator>
#include <time.h>
#include <boost/shared_ptr.hpp>
// #include <boost/traittypes.hpp>

#include "Logger.h"
#include "AuxException.h"
#include "Matrix.h"
#include "Vec3D.h"
#include "Support.h"

#include "XMLnamespace.h"
#include "XMLattribute.h"
#include "XMLobject.h"
#include "XMLgroup.h"
#include "XMLcomp.h"

namespace Mantid
{

namespace XML
{

// --------------------------------------------------------
//                   XMLcomp
// --------------------------------------------------------

template<typename T>
XMLcomp<T>::XMLcomp(XMLobject* B) :
  XMLobject(B),empty(1)
  /*!
    Constructor with junk key (value is NOT set)
    \param K :: key
  */
{}

template<typename T>
XMLcomp<T>::XMLcomp(XMLobject* B,const std::string& K) :
  XMLobject(B,K),empty(1)
  /*!
    Constructor with junk key (value is NOT set)
    \param K :: key
  */
{}

template<typename T>
XMLcomp<T>::XMLcomp(XMLobject* B,const std::string& K,const T& V) :
  XMLobject(B,K),empty(0),Value(V)
  /*!
    Constructor with Key and Value
    \param K :: key
    \param V :: Value to est
  */
{}

template<typename T>
XMLcomp<T>::XMLcomp(const XMLcomp<T>& A) :
  XMLobject(A),empty(A.empty),Value(A.Value)
  /*!
    Standard copy constructor
    \param A :: XMLcomp to copy
  */
{}

template<typename T>
XMLcomp<T>&
XMLcomp<T>::operator=(const XMLcomp<T>& A) 
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
      Value=A.Value;
    }
  return *this;
}


template<typename T>
XMLcomp<T>*
XMLcomp<T>::clone() const
  /*!
    The clone function
    \return new copy of this
  */
{
  return new XMLcomp<T>(*this);
}

template<typename T>
XMLcomp<T>::~XMLcomp()
  /*!
    Standard destructor
  */
{ }

template<typename T>
void
XMLcomp<T>::setComp(const T& A) 
  /*!
    Set the component value
    \param A :: Component to copy
  */
{
   Value=A;
   return;
}

template<typename T>
int
XMLcomp<T>::readObject(std::istream& FX)
  /*!
    Generic read from a string
    \param FX :: Filestream to read from
    \retval 0 :: success
    \retval 1 :: key
  */
{
  std::string Lines;
  std::string closeKey;
  if (XML::splitComp(FX,closeKey,Lines) ||
      this->Key!=closeKey)
    return -1;

  return  (!StrFunc::convert(Lines,Value)) ? -2 : 0;
}


template<>
int
XMLcomp<Geometry::Vec3D>::readObject(std::istream& FX)
  /*!
    Generic read from a string
    \param FX :: Filestream to read from
    \retval 0 :: success
    \retval -1 :: failure (to close key)
  */
{
  std::string Lines;
  std::string closeKey;
  if (XML::splitComp(FX,closeKey,Lines) ||
      this->Key!=closeKey)
    return -1;
  double a,b,c;
  if (!StrFunc::section(Lines,a) || 
      !StrFunc::section(Lines,b) || 
      !StrFunc::section(Lines,c))
    return -2;
  
  Value=Geometry::Vec3D(a,b,c);

  return 0;
}

#ifdef SpecDataHold_h
template<>
int
XMLcomp<TimeData::SpecDataHold>::readObject(std::istream&)
  /*!
    Generic read from a string
    \param FX :: Filestream to read from
    \retval 0 :: failure
    \retval 1 :: success
  */
{
  std::cerr<<"Calling SpecDataHold::readObject"<<std::endl;
  return 0;
}
#endif


template<>
int
XMLcomp<tm*>::readObject(std::istream&)
  /*!
    Generic read from a string
    \param FX :: Filestream to read from
    \retval 0 :: failure
    \retval 1 :: success
  */
{
  std::cerr<<"Calling time tm*"<<std::endl;
  return 0;
}

template<>
int
XMLcomp<nullObj>::readObject(std::istream&)
  /*!
    Generic read from a string
    \param FX :: Filestream to read from
    \retval 0 :: failure
    \retval 1 :: success
  */
{
  std::cerr<<"Calling nullObj::readObject"<<std::endl;
  return  1;
}

template<typename T>
void
XMLcomp<T>::writeXML(std::ostream& OX) const
  /*!
    Write out the object
    \param OX :: output stream
  */
{
  writeDepth(OX);
  std::string::size_type pos=Key.find('%');
  const std::string KeyOut=Key.substr(0,pos);
  if (empty)
    {
      OX<<"<"<<KeyOut<<Attr<<"/>"<<std::endl;
      return;
    }
  OX<<"<"<<KeyOut<<Attr<<">";
  OX<<Value;
  OX<<"</"<<KeyOut<<">"<<std::endl;
  return;
}

template<>
void
XMLcomp<std::string>::writeXML(std::ostream& OX) const
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
      OX<<"<"<<KeyOut<<"/>"<<std::endl;
      return;
    }      

  OX<<"<"<<KeyOut<<Attr<<">";
  // Remove difficult to parse symbols:
  pos=Value.find_first_of("&<>");
  if (pos!=std::string::npos)
    {
      std::string VCopy=Value;
      std::ostringstream cx;
      do
        {
	  cx<<VCopy.substr(0,pos);
	  switch (VCopy[pos])
	    {
	    case '&':
	      cx<<"&amp";
	      break;
	    case '<':
	      cx<<"&lt";
	      break;
	    case '>':
	      cx<<"&gt";
	      break;
	    default:
	      std::cerr<<"Error with value "<<Value<<std::endl;
	      exit(1);
	    }
	  VCopy.erase(0,pos+1);
	  pos=VCopy.find_first_of("&<>");
	} while (pos!=std::string::npos);
      OX<<cx.str();
    }
  else
    OX<<Value;

  OX<<"</"<<KeyOut<<">"<<std::endl;
  return;
}

#ifdef SpecDataHold_h
template<>
void
XMLcomp<TimeData::SpecDataHold>::writeXML(std::ostream& OX) const
  /*!
    Write out a SpecDataHold XML object 
    - This is specialised since it is necessary
    to write out the depth.
    \param OX :: output stream
  */
{
  writeDepth(OX);
  std::string::size_type pos=Key.find('%');
  const std::string KeyOut=Key.substr(0,pos);
  if (empty)
    {
      OX<<"<"<<KeyOut<<"/>"<<std::endl;
      return;
    } 

  OX<<"<"<<KeyOut<<Attr<<">"<<std::endl;
  Value.write(OX,this->depth+2);
  OX<<"</"<<KeyOut<<">"<<std::endl;
  return;
}
#endif

template<>
void
XMLcomp<nullObj>::writeXML(std::ostream& OX) const
  /*!
    Write out a null XML object (\</Key\>)
    \param OX :: output stream
  */
{
  writeDepth(OX);
  std::string::size_type pos=Key.find('%');
  std::string KeyOut( (pos!=std::string::npos)  ? Key.substr(0,pos) : Key);
  OX<<"<"<<KeyOut<<Attr<<"/>"<<std::endl;
  return;
}

template<>
void
XMLcomp<tm*>::writeXML(std::ostream& OX) const
  /*!
    Write out a time XML object (\</Key\>)
    \param OX :: output stream
    \todo :: SPECIAL FOR DATE
  */
{
  writeDepth(OX);
  std::string::size_type pos=Key.find('%');
  std::string KeyOut( (pos!=std::string::npos)  ? Key.substr(0,pos) : Key);
  OX<<"<"<<KeyOut<<"/>"<<std::endl;
  return;
}
  
}   // NAMESPACE XML

/// \cond TEMPLATE

template class XML::XMLcomp<double>;
template class XML::XMLcomp<std::string>;
template class XML::XMLcomp<int>;
template class XML::XMLcomp<Mantid::Geometry::Vec3D>;

//template class XML::XMLcomp<std::vector<double> >;
// template class XML::XMLcomp<std::vector<int> >;
//template class XML::XMLcomp<std::vector<std::string> >;
template class XML::XMLcomp<XML::nullObj>;

#ifdef SpecDataHold_h
// template class XML::XMLcomp<tm*>;
template class XML::XMLcomp<TimeData::SpecDataHold>;
#endif

/// \endcond TEMPLATE

} // NAMESPACE Mantid
