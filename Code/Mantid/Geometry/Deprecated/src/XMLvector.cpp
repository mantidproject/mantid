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

#include "MantidKernel/Logger.h"
#include "AuxException.h"
#include "MantidGeometry/Matrix.h"
#include "MantidGeometry/V3D.h"
#include "MantidKernel/Support.h"
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

template<template<typename T,typename A> class V, typename T,typename A>
XMLvector<V,T,A>::XMLvector(XMLobject* B) :
  XMLobject(B),empty(1)
  /*!
    Constructor with junk key (value is NOT set)
    \param K :: key
  */
{}

template<template<typename T,typename A> class V, typename T,typename A>
XMLvector<V,T,A>::XMLvector(XMLobject* B,const std::string& K) :
  XMLobject(B,K),empty(1)
  /*!
    Constructor with junk key (value is NOT set)
    \param K :: key
  */
{}

template<template<typename T,typename A> class V, typename T,typename A>
XMLvector<V,T,A>::XMLvector(XMLobject* B,const std::string& K,
			  const V<T,A>& Xvec,const V<T,A>& Yvec) :
  XMLobject(B,K),empty(0),X(Xvec),Y(Yvec)
  /*!
    Constructor with Key and Value
    \param K :: key
    \param V :: Value to est
  */
{}

template<template<typename T,typename Alloc> 
         class V, typename T,typename Alloc>
XMLvector<V,T,Alloc>::XMLvector(const XMLvector<V,T,Alloc>& A) :
  XMLobject(A),empty(A.empty),X(A.X),Y(A.Y)
  /*!
    Standard copy constructor
    \param A :: XMLvector to copy
  */
{}

template<template<typename T,typename Alloc> 
         class V, typename T,typename Alloc>
XMLvector<V,T,Alloc>&
XMLvector<V,T,Alloc>::operator=(const XMLvector<V,T,Alloc>& A) 
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

template<template<typename T,typename A> class V, typename T,typename A>
XMLvector<V,T,A>*
XMLvector<V,T,A>::clone() const
  /*!
    The clone function
    \return new copy of this
  */
{
  return new XMLvector<V,T,A>(*this);
}

template<template<typename T,typename A> class V, typename T,typename A>
XMLvector<V,T,A>::~XMLvector()
  /*!
    Standard destructor
  */
{ }

template<template<typename T,typename A> class V, typename T,typename A>
int
XMLvector<V,T,A>::readObject(std::istream& FX)
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

template<template<typename T,typename A> class V, typename T,typename A>
void
XMLvector<V,T,A>::writeXML(std::ostream& OX) const
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
  typename V<T,A>::const_iterator xc=X.begin();
  typename V<T,A>::const_iterator yc;
  for(yc=Y.begin();yc!=Y.end();xc++,yc++)
    OX<<(*xc)<<" "<<(*yc)<<std::endl;
  OX<<"</"<<KeyOut<<">"<<std::endl;
  return;
}

  
}    // NAMESPACE XML

/// \cond TEMPLATE

template class XML::XMLvector<std::vector,double,std::allocator<double> >;

/// \endcond TEMPLATE

}    // NAMESPACE Mantid
