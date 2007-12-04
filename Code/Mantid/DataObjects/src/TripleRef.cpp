#include <string>
#include "TripleRef.h"

namespace Mantid
{

/*!
  \namespace Iterator
  \author S. Ansell
  \version 0.1
  \date December 2007
  \brief Holds items to allow iteration on data types
*/

namespace Iterator

template<typename T>
TripleRef<T>::TripleRef(const TripleRef<T>& A) :
  first(A.first),second(A.second),third(A.third)
  /*!
    Standard Copy Constructor
    \param A :: TripleRef Item to copy
  */
{}

template<typename T>
TripleRef<T>::TripleRef(const T A,const T B,const T C) :
  first(A),second(B),third(C)
  /*!
    Constructor from a 3 value input 
    \param A :: first item
    \param B :: second item
    \param C :: third item
  */
{}

template<typename T>
TripleRef<T>&
TripleRef<T>::operator=(const TripleRef<T>& A)
  /*!
    Standard Assignment Constructor
    \param A :: TripleRef Item to copy
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
TripleRef<T>::~TripleRef()
  /*!
    Standard Destructor
  */
{}


template<typename T>
int
TripleRef<T>::operator==(const TripleRef<T>& A) const
  /*! 
    Operator== all components must be equal
  */
{
  return  (first!=A.first || second!=A.second || 
	   third!=A.third) ? 0 : 1;
}

template<typename T>
int
TripleRef<T>::operator!=(const TripleRef<T>& A) const
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
TripleRef<T>::operator<(const TripleRef<T>& A) const
  /*! 
    Operator< takes first to last precidence.
    \param A :: TripleRef to compare
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
TripleRef<T>::operator>(const TripleRef<T>& A) const
  /*! 
    Operator> takes first to last precidence.
    Uses operator<  to obtain value.
    Note it does not uses 1-(A<this)
    \param A :: TripleRef to compare
    \return this > A
  */
{
  return A.operator<(*this);
}

template<typename T>
T
TripleRef<T>::operator[](const int A)
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
const T
TripleRef<T>::operator[](const int A) const
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

template<typename T>
T
TripleRef<T>::operator[](const int A) 
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


/// \cond TEMPLATE

template class Triple<double&>;

/// \endcond TEMPLATE

}  // NAMESPACE Mantid
