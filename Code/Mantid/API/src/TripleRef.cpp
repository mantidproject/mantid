#include <iostream>
#include <string>
#include <stdexcept>
#include "MantidAPI/TripleRef.h"
#include "MantidAPI/IErrorHelper.h"

namespace Mantid
{

  namespace API
  {

    /*!
    Standard Copy Constructor
    \param A :: TripleRef Item to copy
    */
    template<typename T>
    TripleRef<T>::TripleRef(const TripleRef<T>& A) :
    first(A.first),second(A.second),third(A.third),
      errorHelper(A.errorHelper),detector(A.detector)
    {}

    /// Default constructor
    template<typename T>
    TripleRef<T>::TripleRef():first(0),second(0),third(0),
      errorHelper(0),detector(0)
    {}

    /*!
    Constructor from a 3 value input 
    \param A :: first item
    \param B :: second item
    \param C :: third item
    */
    template<typename T>
    TripleRef<T>::TripleRef(T& A,T& B,T& C):first(&A),second(&B),third(&C),
      errorHelper(0),detector(0)
    {
    }

    /*!
    Standard Assignment Constructor
    \param A :: TripleRef Item to copy
    \return *this
    */
    template<typename T>
    TripleRef<T>& TripleRef<T>::operator=(const TripleRef<T>& A)
    {
      if (this!=&A)
      {
        *first= *A.first;
        *second= *A.second;
        *third= *A.third;
        *errorHelper = *A.errorHelper;
        detector = A.detector;
      }
      return *this;
    }

    /*!
    Standard Destructor
    */
    template<typename T>
    TripleRef<T>::~TripleRef()
    {
      //do not delete the contents of first second and third as they are managed by the collection.
    }


    /*! 
    Operator== all components must be equal
    */
    template<typename T>
    int TripleRef<T>::operator==(const TripleRef<T>& A) const
    {
      return  (*first!=*A.first || *second!=*A.second || 
        *third!=*A.third) ? 0 : 1;
    }

    /*! 
    Operator!= any component is not equal
    \param A :: Other object to compare
    \return this!=A
    */
    template<typename T>
    int TripleRef<T>::operator!=(const TripleRef<T>& A) const
    {
      return  (*first==*A.first && *second == *A.second &&
        *third== *A.third) ? 0 : 1;
    }

    /*! 
    Operator< takes first to last precidence.
    \param A :: TripleRef to compare
    \return this < A
    */
    template<typename T>
    int TripleRef<T>::operator<(const TripleRef<T>& A) const
    {
      if (&A!=this)
      {
        if (*first> *A.first)
          return 0;
        if (*first< *A.first)
          return 1;
        if (*second> *A.second)
          return 0;
        if (*second< *A.second)
          return 1;
        if (*third < *A.third)
          return 0;
      }
      return 0;
    }

    /*! 
    Operator> takes first to last precidence.
    Uses operator<  to obtain value.
    Note it does not uses 1-(A<this)
    \param A :: TripleRef to compare
    \return this > A
    */
    template<typename T>
    int TripleRef<T>::operator>(const TripleRef<T>& A) const
    {
      return !(this->operator<(A));
    }

    /*!
    Accessor Reference Function
    \param A :: Index to item to get 0-2
    \return Reference Item[A]
    */
    template<typename T>
    T& TripleRef<T>::operator[](const int A)
    {
      switch (A)
      {
      case 0:
        return *first;
      case 1:
        return *second;
      case 2:
        return *third;
      default:
        throw std::range_error("TripleRef::operator[]");
      }
      // Never gets here
      //  return *first;
    }


    /*!
    Accessor Value Function
    \param A :: Index to item to get 0-2
    \return Item[A]
    */
    template<typename T>
    const T& TripleRef<T>::operator[](const int A)  const
    {
      switch (A)
      {
      case 0:
        return *first;
      case 1:
        return *second;
      case 2:
        return *third;
      default:
        throw std::range_error("TripleRef::operator[]");
      }
      // Never gets here
      //  return *first;
    }

    template<typename T>
    const IErrorHelper* TripleRef<T>::ErrorHelper() const
    {
      return errorHelper; 
    }

    template<typename T>
    const int& TripleRef<T>::Detector() const
    {
      return detector; 
    }

    /// \cond TEMPLATE

    template class TripleRef<double>;

    /// \endcond TEMPLATE

  } // NAMESPACE API

}  // NAMESPACE Mantid
