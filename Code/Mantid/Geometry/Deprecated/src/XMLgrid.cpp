#include <iostream>
#include <cmath>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <functional>
#include <iterator>
#include <boost/shared_ptr.hpp>
// #include <boost/traittypes.hpp>

#include "Logger.h"
#include "AuxException.h"
#include "RefControl.h"
#include "Matrix.h"
#include "Vec3D.h"
#include "Support.h"
#include "XMLnamespace.h"
#include "XMLattribute.h"
#include "XMLobject.h"
#include "XMLgroup.h"
#include "XMLgrid.h"

namespace Mantid
{

namespace XML
{

// --------------------------------------------------------
//                   XMLgrid
// --------------------------------------------------------

template<template<typename T,typename A> class V,typename T,typename A> 
XMLgrid<V,T,A>::XMLgrid(XMLobject* B,const std::string& K) :
  XMLobject(B,K),size(0),empty(1),contLine(10)
  /*!
    Constructor with junk key (value is NOT set)
    \param B :: XMLobject to used as parent
    \param K :: key
  */
{}

template<template<typename T,typename Alloc> class V,typename T,typename Alloc> 
XMLgrid<V,T,Alloc>::XMLgrid(const XMLgrid<V,T,Alloc>& A) :
  XMLobject(A),size(A.size),empty(A.empty),Grid(A.Grid),
  contLine(A.contLine)
  /*!
    Standard copy constructor
    \param A :: XMLgrid to copy
  */
{}

template<template<typename T,typename A> class V,typename T,typename Alloc> 
XMLgrid<V,T,Alloc>&
XMLgrid<V,T,Alloc>::operator=(const XMLgrid<V,T,Alloc>& A) 
  /*!
    Standard assignment operator
    \param A :: Object to copy
    \return *this
  */
{
  if (this!=&A)
    {
      XMLobject::operator=(A);
      size=A.size;
      empty=A.empty;
      Grid=A.Grid;
      contLine=A.contLine;
    }
  return *this;
}


template<template<typename T,typename A> class V,typename T,typename A> 
XMLgrid<V,T,A>*
XMLgrid<V,T,A>::clone() const
  /*!
    The clone function
    \return new copy of this
  */
{
  return new XMLgrid<V,T,A>(*this);
}

template<template<typename T,typename A> class V,typename T,typename A> 
XMLgrid<V,T,A>::~XMLgrid()
  /*!
    Standard destructor
  */
{}

template<template<typename T,typename A> class V,typename T,typename A> 
void
XMLgrid<V,T,A>::setComp(const int Index,const V<T,A>& VO)
  /*!
    Set Component in Grid.
    \param Index :: Index number to add
    \param VO :: container object to add
   */
{
  if (Index>=size)
    {
      size=Index+1;
      Grid.resize(size);
    }
  if (!VO.empty())
    {
      Grid[Index]=VO;
      empty=0;
    }
  return;
}

template<template<typename T,typename A> class V,typename T,typename A> 
V<T,A>&
XMLgrid<V,T,A>::getGVec(const int Index)
  /*!
    Set Component in Grid.
    \param Index :: Index number to add
    \param VO :: container object to add
   */
{
  if (Index<0 || Index>=static_cast<int>(Grid.size()))
    throw ColErr::IndexError(Index,Grid.size(),"XMLgrid::getGVec");
  return Grid[Index];
}

template<template<typename T,typename A> class V,typename T,typename A> 
const V<T,A>&
XMLgrid<V,T,A>::getGVec(const int Index) const
  /*!
    Set Component in Grid.
    \param Index :: Index number to add
    \param VO :: container object to add
   */
{
  if (Index<0 || Index>=static_cast<int>(Grid.size()))
    throw ColErr::IndexError(Index,Grid.size(),"XMLgrid::getGVec");
  return Grid[Index];
}

template<template<typename T,typename A> class V,typename T,typename A> 
int
XMLgrid<V,T,A>::readObject(std::istream& FX)
  /*!
    Generic read from a string. Assumes that size has
    been set.
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
  T value;
  int side(0);
  Grid.clear();
  Grid.resize(side);
  do
    {
      pX=XML::splitLine(FX,closeKey,Lines);
      while(StrFunc::section(Lines,value))
        {
	  Grid[side].push_back(value);
	  side++;
	  side %= size;
	}
    }
  while(!pX);
 
  // even up sizes of vectors
  for(;side;side=(side+1) % size)
    Grid[side].push_back(T());
  
  if (XML::splitLine(FX,closeKey,Lines) ||
      this->Key!=closeKey)
    return -1;
	
  return 0;
}

template<template<typename T,typename A> class V,typename T,typename A> 
void
XMLgrid<V,T,A>::writeXML(std::ostream& OX) const
  /*!
    Write out the object
    \param OX :: output stream
  */
{
  writeDepth(OX);
  std::string::size_type pos=Key.find('%');
  const std::string KeyOut=Key.substr(0,pos);
  if (empty || size==0 || Grid[0].empty())
    {
      OX<<"<"<<KeyOut<<Attr<<"/>"<<std::endl;
      return;
    }
  
  typename std::vector< V<T,A> >::const_iterator gc;
  std::vector< typename V<T,A>::const_iterator > IterVec;  // Vector of iterators
  std::vector< typename  V<T,A>::const_iterator > IterEnd;  // Vector of iterators
  typename std::vector< typename V<T,A>::const_iterator >::iterator ivc;  // Vector of iterators
  typename std::vector< typename V<T,A>::const_iterator >::iterator evc;  // Vector of iterators
  int finished=Grid.size();       // Number of components that must be changed
  for(gc=Grid.begin();gc!=Grid.end();gc++)
    {
      IterVec.push_back(gc->begin());
      IterEnd.push_back(gc->end());
      if (gc->begin()==gc->end())
	finished--;
    }

  // EMPTY WRITE::
  if (!finished)
    {
      writeDepth(OX);
      OX<<"<"<<KeyOut<<Attr<<"/>"<<std::endl;
      return; 
    }

  // Valued write
  OX<<"<"<<KeyOut<<Attr<<">"<<std::endl;

  const int factor(IterVec.size());
  int cnt(0);
  while(finished)
    {
      if (cnt==0)
	writeDepth(OX);

      for(ivc=IterVec.begin(),evc=IterEnd.begin();
	  ivc!=IterVec.end();ivc++,evc++)
        {
	  if (*ivc != *evc)
	    {
	      OX<<*(*ivc)<<" ";
	      (*ivc)++;
	      if (*ivc == *evc)          // Note the extra test after ivc++
		finished--;
	      cnt++;
	    }
	  else
	    {
	      OX<<"    ";
	    }
	}
      if (cnt+factor>contLine)
        {
	  OX<<std::endl;
	  cnt=0;
	}
    }
  writeDepth(OX);
  OX<<"</"<<KeyOut<<">"<<std::endl;
  return;
}

  
} // NAMESPACE

/// \cond TEMPLATE

template class XML::XMLgrid<std::vector,std::string,std::allocator<std::string> >;
template class XML::XMLgrid<std::vector,double,std::allocator<double> >;
template class XML::XMLgrid<std::vector,int,std::allocator<int> >;

/// \endcond TEMPLATE

}  // NAMESPACE Mantid
