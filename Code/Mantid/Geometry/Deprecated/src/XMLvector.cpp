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
#include "RefControl.h"
#include "Matrix.h"
#include "Vec3D.h"
#include "Support.h"
// #include "SpecDataHold.h"
#include "XMLnamespace.h"
#include "XMLattribute.h"
#include "XMLobject.h"
#include "XMLgroup.h"
#include "XMLvector.h"

namespace Mantid
{

namespace XML
{

// --------------------------------------------------------
//                   XMLvector
// --------------------------------------------------------


template<template<typename T> class V, typename T>
XMLvector<V,T>::XMLvector(XMLobject* B,const std::string& K) :
  XMLobject(B,K),empty(1)
  /*!
    Constructor with junk key (value is NOT set)
    \param K :: key
  */
{}

template<template<typename T> class V, typename T>
XMLvector<V,T>::XMLvector(XMLobject* B,const std::string& K,
			  const V<T>& Xvec,const V<T>& Yvec) :
  XMLobject(B,K),empty(0),X(Xvec),Y(Yvec)
  /*!
    Constructor with Key and Value
    \param K :: key
    \param V :: Value to est
  */
{}

template<template<typename T> class V, typename T>
XMLvector<V,T>::XMLvector(const XMLvector<V,T>& A) :
  XMLobject(A),empty(A.empty),X(A.X),Y(A.Y)
  /*!
    Standard copy constructor
    \param A :: XMLvectory to copy
  */
{}

template<template<typename T> class V, typename T>
XMLvector<V,T>&
XMLvector<V,T>::operator=(const XMLvector<V,T>& A) 
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
      X=A.X;
      Y=A.Y;
    }
  return *this;
}


template<template<typename T> class V, typename T>
XMLvector<V,T>*
XMLvector<V,T>::clone() const
  /*!
    The clone function
    \return new copy of this
  */
{
  return new XMLvector<V,T>(*this);
}

template<template<typename T> class V, typename T>
XMLvector<V,T>::~XMLvector()
  /*!
    Standard destructor
  */
{ }

template<template<typename T> class V, typename T>
int
XMLvector<V,T>::readObject(std::istream& FX)
  /*!
    Generic read from a string
    \param FX :: Filestream to read from
    \retval 0 :: success
    \retval 1 :: key
  */
{
  std::string Lines;
  std::string closeKey;
  int pX=XML::splitLine(FX,closeKey,Lines);
  if (pX<0)
    return -1;
  double value;
  int side(0);
  do
    {
      pX=XML::splitLine(FX,closeKey,Lines);
      while(StrFunc::section(Lines,value))
        {
	  switch(side)
	    {
	    case 0: 
	      X.push_back(value);
	      break;
	    case 1:
	      Y.push_back(value);
	      break;
	    }
	  side=1-side;
	}
    }
  while(!pX);
  
  empty = (X.empty()) ? 1 : 0;
  if (pX<0 || this->Key!=closeKey)
    return -1;
  return 0;
}

template<template<typename T> class V, typename T>
void
XMLvector<V,T>::writeXML(std::ostream& OX) const
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
  OX<<"<"<<KeyOut<<Attr<<">"<<std::endl;
  typename V<T>::const_iterator xc=X.begin();
  typename V<T>::const_iterator yc;
  for(yc=Y.begin();yc!=Y.end();xc++,yc++)
    OX<<(*xc)<<" "<<(*yc)<<std::endl;
  OX<<"</"<<KeyOut<<">"<<std::endl;
  return;
}
  
}    // NAMESPACE XML

/// \cond TEMPLATE

template class XML::XMLvector<std::vector,double>;

/// \endcond TEMPLATE

}    // NAMESPACE Mantid
