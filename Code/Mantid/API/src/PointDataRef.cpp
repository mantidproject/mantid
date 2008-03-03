#include "MantidAPI/PointDataRef.h"
#include "MantidAPI/IErrorHelper.h"
#include "MantidKernel/Exception.h"
namespace Mantid
{

  namespace API
  {

    /*!
    Standard Copy Constructor
    \param A :: PointDataRef Item to copy
    */
    PointDataRef::PointDataRef(const PointDataRef& A) : IPointData(),
      xPointer(A.xPointer),x2Pointer(A.x2Pointer),yPointer(A.yPointer),ePointer(A.ePointer),e2Pointer(A.e2Pointer),
      errorHelper(A.errorHelper),spectraNo(A.spectraNo)
    {}

    /// Default constructor
    PointDataRef::PointDataRef(): IPointData(),
      xPointer(0),x2Pointer(0),yPointer(0),ePointer(0),e2Pointer(0),
      errorHelper(0),spectraNo(0)
    {}

    /*!
    Standard Assignment Constructor
    \param A :: PointDataRef Item to copy
    \return *this
    */
    PointDataRef& PointDataRef::operator=(const PointDataRef& A)
    {
      if (this!=&A)
      {
        *xPointer= *A.xPointer;
        *yPointer= *A.yPointer;
        *ePointer= *A.ePointer;
        if (A.e2Pointer)
        {
          *e2Pointer= *A.e2Pointer;
        }        
        if (A.x2Pointer)
        {
          *x2Pointer= *A.x2Pointer;
        }
        errorHelper = A.errorHelper;
        spectraNo = A.spectraNo;
      }
      return *this;
    }

    /*!
    Standard Assignment Constructor
    \param A :: IPointData Item to copy
    \return *this
    */
    PointDataRef& PointDataRef::operator=(const IPointData& A)
    {
      if (this!=&A)
      {
        *xPointer= A.X();
        *yPointer= A.Y();
        *ePointer= A.E();
        if(e2Pointer)
        {
          *e2Pointer= A.E2();
        }        
        if(x2Pointer)
        {
          *x2Pointer= A.X2();
        }
        errorHelper = A.ErrorHelper();
        spectraNo = A.SpectraNo();
      }
      return *this;
    }

    /*!
    Standard Destructor
    */
    PointDataRef::~PointDataRef()
    {
      //do not delete the contents as they are managed by the collection.
    }


    /*! 
    Operator== all components must be equal
    \param A :: Other object to compare
    */
    int PointDataRef::operator==(const PointDataRef& A) const
    {
      return  (*xPointer!=*A.xPointer || *yPointer!=*A.yPointer || 
        *ePointer!=*A.ePointer || *e2Pointer!=*A.e2Pointer) ? 0 : 1;
    }

    /*! 
    Operator!= any component is not equal
    \param A :: Other object to compare
    \return this!=A
    */
    int PointDataRef::operator!=(const PointDataRef& A) const
    {
      return  (*xPointer==*A.xPointer && *yPointer == *A.yPointer &&
        *ePointer== *A.ePointer  && *e2Pointer!=*A.e2Pointer) ? 0 : 1;
    }

    /*! 
    Operator< takes xPointer to last precidence.
    \param A :: PointDataRef to compare
    \return this < A
    */
    int PointDataRef::operator<(const PointDataRef& A) const
    {
      if (&A!=this)
      {
        if (*xPointer> *A.xPointer)
          return 0;
        if (*xPointer< *A.xPointer)
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
    \param A :: PointDataRef to compare
    \return this > A
    */
    int PointDataRef::operator>(const PointDataRef& A) const
    {
      return !(this->operator<(A));
    }

    /** Const Accessor for ErrorHelper class
    @return Pointer to the ErrorHelper class
    */
    const IErrorHelper* PointDataRef::ErrorHelper() const
    {
      return errorHelper; 
    }

    /** Const Accessor for Spectra Number
    @return The Spectra Number
    */
    int PointDataRef::SpectraNo() const
    {
      return spectraNo; 
    } 

    /** Const Accessor for X value
    @return The X value
    */
    const double& PointDataRef::X() const
    {
      return *xPointer; 
    }

    /** Accessor for X value
    @return The X value
    */
    double& PointDataRef::X()
    {
      return *xPointer; 
    }

    /** Const Accessor for Y value
    @return The Y value
    */
    const double& PointDataRef::Y() const
    {
      return *yPointer; 
    }

    /** Accessor for Y value
    @return The Y value
    */
    double& PointDataRef::Y()
    {
      return *yPointer; 
    }

    /** Const Accessor for E value
    @return The E value
    */
    const double& PointDataRef::E() const
    {
      return *ePointer; 
    }

    /** Accessor for E value
    @return The E value
    */
    double& PointDataRef::E()
    {
      return *ePointer; 
    }

    /** Const Accessor for E2 value
    @return The E2 value
    */
    const double& PointDataRef::E2() const
    {
      return *e2Pointer; 
    }

    /** Accessor for E2 value
    @return The E2 value
    */
    double& PointDataRef::E2()
    {
      return *e2Pointer; 
    }

    /** Const Accessor for X2 value, this should only be used if isHistogram() == true
    @return The X2 value
    */
    const double& PointDataRef::X2() const 
    {
      if (isHistogram())
      {
        return *x2Pointer; 
      }
      else
      {
        throw Kernel::Exception::NotFoundError("X2 value is not set, check isHistogram() before accessing X2","X2");
      }
    }

    /** Accessor for X2 value, this should only be used if isHistogram() == true
    @return The X2 value
    */
    double& PointDataRef::X2() 
    {
      if (isHistogram())
      {
        return *x2Pointer; 
      }
      else
      {
        throw Kernel::Exception::NotFoundError("X2 value is not set, check isHistogram() before accessing X2","X2");
      }
    }
   
    /** Returns true if the data point is hastogram data and therefore has an X2.
    @returns true if the X2 value is present
    */
    const bool PointDataRef::isHistogram() const
    {
      return (x2Pointer!=0);
    }


    /*! Clone method
    *  Make a copy of the PointDataRef
    *  @return new(*this)
    */
    PointDataRef* PointDataRef::clone() const
    {
      return new PointDataRef(*this);
    }
  } // NAMESPACE API

}  // NAMESPACE Mantid
