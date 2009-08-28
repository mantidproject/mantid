#include <string>
#include "MantidGeometry/Math/Triple.h"

namespace Mantid
{

template<typename T>
Triple<T>::Triple() : first(),second(),third()
  /*!
    Standard Constructor
  */
{}
  
template<typename T>
Triple<T>::Triple(const Triple<T>& A) :
  first(A.first),second(A.second),third(A.third)
  /*!
    Standard Copy Constructor
    \param A :: Triple Item to copy
  */
{}

template<typename T>
Triple<T>::Triple(const T& A,const T& B,const T& C) :
  first(A),second(B),third(C)
  /*!
    Constructor from a 3 value input 
    \param A :: first item
    \param B :: second item
    \param C :: third item
  */
{}

template<typename T>
Triple<T>&
Triple<T>::operator=(const Triple<T>& A)
  /*!
    Standard Assignment Constructor
    \param A :: Triple Item to copy
    \return *this
  */
{
  if (this!=&A)
    {
      first=A.first;
      second=A.second;
      third=A.third;
    }
  return *this;
}

template<typename T>
Triple<T>::~Triple()
  /*!
    Standard Destructor
  */
{}


template<typename T>
int
Triple<T>::operator==(const Triple<T>& A) const
  /*! 
    Operator== all components must be equal
  */
{
  return  (first!=A.first || second!=A.second || 
	   third!=A.third) ? 0 : 1;
}

template<typename T>
int
Triple<T>::operator!=(const Triple<T>& A) const
  /*! 
    Operator!= any component is not equal
    \param A :: Other object to compare
    \return this!=A
  */
{
  return  (first==A.first && second==A.second &&
	   third==A.third) ? 0 : 1;
}

template<typename T>
int
Triple<T>::operator<(const Triple<T>& A) const
  /*! 
    Operator< takes first to last precidence.
    \param A :: Triple to compare
    \return this < A
  */
{
  if (first>A.first)
    return 0;
  if (first<A.first)
    return 1;
  if (second>A.second)
    return 0;
  if (second<A.second)
    return 1;
  if (third>=A.third)
    return 0;
  return 1;
}

template<typename T>
int
Triple<T>::operator>(const Triple<T>& A) const
  /*! 
    Operator> takes first to last precidence.
    Uses operator<  to obtain value.
    Note it does not uses 1-(A<this)
    \param A :: Triple to compare
    \return this > A
  */
{
  return A.operator<(*this);
}

template<typename T>
T&
Triple<T>::operator[](const int A)
  /*!
    Accessor Reference Function
    \param A :: Index to item to get 0-2
    \return Reference Item[A]
  */
{
  switch (A)
    {
    case 0:
      return first;
    case 1:
      return second;
    case 2:
      return third;
    default:
      throw "Range Error";
    }
  // Never gets here
  return first;
}

template<typename T>
T
Triple<T>::operator[](const int A) const
  /*!
    Accessor Value Function
    \param A :: Index to item to get 0-2
    \return Item[A]
  */
{
  switch (A)
    {
    case 0:
      return first;
    case 1:
      return second;
    case 2:
      return third;
    default:
      throw "Range Error";
    }
  // Never gets here
  return first;
}


/* 
------------------------------------------------------------------
    Different Triple 
------------------------------------------------------------------   
*/

template<typename F,typename S,typename T>
DTriple<F,S,T>::DTriple() : first(),second(),third()
  /*!
    Standard Constructor
  */
{}
  
template<typename F,typename S,typename T>
DTriple<F,S,T>::DTriple(const DTriple<F,S,T>& A) :
  first(A.first),second(A.second),third(A.third)
  /*!
    Standard Copy Constructor
    \param A :: DTriple Item to copy
  */
{}

template<typename F,typename S,typename T>
DTriple<F,S,T>::DTriple(const F& A,const S& B,const T& C) :
  first(A),second(B),third(C)
  /*!
    Constructor from a 3 value input 
    \param A :: first item
    \param B :: second item
    \param C :: third item
  */
{}

template<typename F,typename S,typename T>
DTriple<F,S,T>&
DTriple<F,S,T>::operator=(const DTriple<F,S,T>& A)
  /*!
    Assignment from a 3 value input 
    \param A :: DTriple to copy from 
    \returns *this
  */
{
  if (this!=&A)
    {
      first=A.first;
      second=A.second;
      third=A.third;
    }
  return *this;
}

template<typename F,typename S,typename T>
DTriple<F,S,T>::~DTriple()
  /*!
    Standard Destructor
  */
{}


template<typename F,typename S,typename T>
int
DTriple<F,S,T>::operator==(const DTriple<F,S,T>& A) const
  /*! 
    Operator== all components must be equal
    \param A :: Object to compare
    \returns A==*this
  */
{
  return  (first!=A.first || second!=A.second || 
	   third!=A.third) ? 0 : 1;
}

template<typename F,typename S,typename T>
int
DTriple<F,S,T>::operator!=(const DTriple<F,S,T>& A) const
  /*! 
    Operator!= any component is not equal
  */
{
  return  (first==A.first && second==A.second &&
	   third==A.third) ? 0 : 1;
}

template<typename F,typename S,typename T>
int
DTriple<F,S,T>::operator<(const DTriple<F,S,T>& A) const
  /*! 
    Operator< takes first to last precidence.
    \param A :: Triple to compare
    \return this < A
  */
{
  if (first>A.first)
    return 0;
  if (first<A.first)
    return 1;
  if (second>A.second)
    return 0;
  if (second<A.second)
    return 1;
  if (third>=A.third)
    return 0;
  return 1;
}

template<typename F,typename S,typename T>
int
DTriple<F,S,T>::operator>(const DTriple<F,S,T>& A) const
  /*! 
    Operator> takes first to last precidence.
    \param A :: Triple to compare
    \return this < A
  */
{
  return A.operator<(*this);
}

/// \cond TEMPLATE

namespace Geometry
{
class Rule;
}

template class DTriple<Geometry::Rule*,int,Geometry::Rule*>;
template class DTriple<std::string,int,int>;
template class DTriple<int,int,double>;
template class DTriple<int,int,std::string>;
template class DTriple<int,double,std::string>;
template class Triple<int>;
template class Triple<double>;

/// \endcond TEMPLATE

} // Namespace Mantid
