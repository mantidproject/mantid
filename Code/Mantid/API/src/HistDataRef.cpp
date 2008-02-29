#include <iostream>
#include <string>
#include <stdexcept>
#include "MantidAPI/HistDataRef.h"
#include "MantidAPI/IErrorHelper.h"

namespace Mantid
{

  namespace API
  {

    /*!
    Standard Copy Constructor
    \param A :: HistDataRef Item to copy
    */
    HistDataRef::HistDataRef(const HistDataRef& A) : PointDataRef(A),IHistData(),
      /*xPointer(A.xPointer),yPointer(A.yPointer),ePointer(A.ePointer),e2Pointer(A.e2Pointer),
      errorHelper(A.errorHelper),spectraNo(A.spectraNo),*/ x2Pointer(A.x2Pointer)
    {}

    /// Default constructor
    HistDataRef::HistDataRef(): PointDataRef(),IHistData(),
      x2Pointer(0)
    {}

    /*!
    Standard Assignment Constructor
    \param A :: HistDataRef Item to copy
    \return *this
    */
    HistDataRef& HistDataRef::operator=(const HistDataRef& A)
    {
      if (this!=&A)
      {
        *xPointer= *A.xPointer;
        *x2Pointer= *A.x2Pointer;
        *yPointer= *A.yPointer;
        *ePointer= *A.ePointer;
        if (A.e2Pointer)
        {
          *e2Pointer= *A.e2Pointer;
        }
        errorHelper = A.errorHelper;
        spectraNo = A.spectraNo;
      }
      return *this;
    }

    /*!
    Standard Assignment Constructor
    \param A :: IHistData Item to copy
    \return *this
    */
    HistDataRef& HistDataRef::operator=(const IHistData& A)
    {
      if (this!=&A)
      {
        *xPointer= A.X();
        *x2Pointer= A.X2();
        *yPointer= A.Y();
        *ePointer= A.E();
        *e2Pointer= A.E2();
        errorHelper = A.ErrorHelper();
        spectraNo = A.SpectraNo();
      }
      return *this;
    }

    /*!
    Standard Destructor
    */
    HistDataRef::~HistDataRef()
    {
      //do not delete the contents as they are managed by the collection.
    }


    /*! 
    Operator== all components must be equal
    \param A :: Other object to compare
    */
    int HistDataRef::operator==(const HistDataRef& A) const
    {
      return  (*xPointer!=*A.xPointer || *x2Pointer!=*A.x2Pointer ||
        *yPointer!=*A.yPointer || 
        *ePointer!=*A.ePointer || *e2Pointer!=*A.e2Pointer) ? 0 : 1;
    }

    /*! 
    Operator!= any component is not equal
    \param A :: Other object to compare
    \return this!=A
    */
    int HistDataRef::operator!=(const HistDataRef& A) const
    {
      return  (*xPointer==*A.xPointer && *x2Pointer==*A.x2Pointer &&
        *yPointer == *A.yPointer &&
        *ePointer== *A.ePointer  && *e2Pointer!=*A.e2Pointer) ? 0 : 1;
    }

    /*! 
    Operator< takes xPointer to last precidence.
    \param A :: HistDataRef to compare
    \return this < A
    */
    int HistDataRef::operator<(const HistDataRef& A) const
    {
      if (&A!=this)
      {
        if (*xPointer> *A.xPointer)
          return 0;
        if (*xPointer< *A.xPointer)
          return 1;
        if (*x2Pointer> *A.x2Pointer)
          return 0;
        if (*x2Pointer< *A.x2Pointer)
          return 1;
        if (*yPointer> *A.yPointer)
          return 0;
        if (*yPointer< *A.yPointer)
          return 1;
        if (*ePointer > *A.ePointer)
          return 0;
        if (*yPointer< *A.yPointer)
          return 1;
        if (*e2Pointer < *A.e2Pointer)
          return 0;
      }
      return 0;
    }

    /*! 
    Operator> takes xPointer to last precidence.
    Uses operator<  to obtain value.
    Note it does not uses 1-(A<this)
    \param A :: HistDataRef to compare
    \return this > A
    */
    int HistDataRef::operator>(const HistDataRef& A) const
    {
      return !(this->operator<(A));
    }

    /** Const accessor for X2
    @return The value of X2
    */
    const double& HistDataRef::X2() const
    {
      return *x2Pointer; 
    }

    /** Accessor for X2
    @return The value of X2
    */
    double& HistDataRef::X2()
    {
      return *x2Pointer; 
    }

  } // NAMESPACE API

}  // NAMESPACE Mantid
