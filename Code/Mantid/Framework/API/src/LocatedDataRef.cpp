#include "MantidAPI/LocatedDataRef.h"
#include "MantidKernel/Exception.h"
namespace Mantid
{

  namespace API
  {

    /**
    Standard Copy Constructor
    @param A :: LocatedDataRef Item to copy
    */
    LocatedDataRef::LocatedDataRef(const LocatedDataRef& A) : ILocatedData(),
      xPointer(A.xPointer),x2Pointer(A.x2Pointer),yPointer(A.yPointer),ePointer(A.ePointer)
    {}

    /// Default constructor
    LocatedDataRef::LocatedDataRef(): ILocatedData(),
      xPointer(0),x2Pointer(0),yPointer(0),ePointer(0)
    {}

    /**
    Standard Assignment Constructor
    @param A :: LocatedDataRef Item to copy
    @return *this
    */
    LocatedDataRef& LocatedDataRef::operator=(const LocatedDataRef& A)
    {
      if (this!=&A)
      {
        *xPointer= *A.xPointer;
        *yPointer= *A.yPointer;
        *ePointer= *A.ePointer;
        if (A.x2Pointer)
        {
          *x2Pointer= *A.x2Pointer;
        }
      }
      return *this;
    }

    /**
    Standard Assignment Constructor
    @param A :: ILocatedData Item to copy
    @return *this
    */
    LocatedDataRef& LocatedDataRef::operator=(const ILocatedData& A)
    {
      if (this!=&A)
      {
        *xPointer= A.X();
        *yPointer= A.Y();
        *ePointer= A.E();
        if(x2Pointer)
        {
          *x2Pointer= A.X2();
        }
      }
      return *this;
    }

    /**
    Standard Destructor
    */
    LocatedDataRef::~LocatedDataRef()
    {
      //do not delete the contents as they are managed by the collection.
    }


    /** 
    Operator== all components must be equal
    @param A :: Other object to compare
    */
    int LocatedDataRef::operator==(const LocatedDataRef& A) const
    {
      return  (*xPointer!=*A.xPointer || *yPointer!=*A.yPointer || 
        *ePointer!=*A.ePointer) ? 0 : 1;
    }

    /** 
    Operator!= any component is not equal
    @param A :: Other object to compare
    @return this!=A
    */
    int LocatedDataRef::operator!=(const LocatedDataRef& A) const
    {
      return  (*xPointer==*A.xPointer && *yPointer == *A.yPointer &&
        *ePointer== *A.ePointer) ? 0 : 1;
    }

    /** 
    Operator< takes xPointer to last precidence.
    @param A :: LocatedDataRef to compare
    @return this < A
    */
    int LocatedDataRef::operator<(const LocatedDataRef& A) const
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
      }
      return 0;
    }

    /** 
    Operator> takes xPointer to last precidence.
    Uses operator<  to obtain value.
    Note it does not uses 1-(A<this)
    @param A :: LocatedDataRef to compare
    @return this > A
    */
    int LocatedDataRef::operator>(const LocatedDataRef& A) const
    {
      return !(this->operator<(A));
    }

    /** Const Accessor for X value
    @return The X value
    */
    const double& LocatedDataRef::X() const
    {
      return *xPointer; 
    }

    /** Accessor for X value
    @return The X value
    */
    double& LocatedDataRef::X()
    {
      return *xPointer; 
    }

    /** Const Accessor for Y value
    @return The Y value
    */
    const double& LocatedDataRef::Y() const
    {
      return *yPointer; 
    }

    /** Accessor for Y value
    @return The Y value
    */
    double& LocatedDataRef::Y()
    {
      return *yPointer; 
    }

    /** Const Accessor for E value
    @return The E value
    */
    const double& LocatedDataRef::E() const
    {
      return *ePointer; 
    }

    /** Accessor for E value
    @return The E value
    */
    double& LocatedDataRef::E()
    {
      return *ePointer; 
    }

    /** Const Accessor for X2 value, this should only be used if isHistogram() == true
    @return The X2 value
    */
    const double& LocatedDataRef::X2() const 
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
    double& LocatedDataRef::X2() 
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
    bool LocatedDataRef::isHistogram() const
    {
      return (x2Pointer!=0);
    }


    /** Clone method
    *  Make a copy of the LocatedDataRef
    *  @return new(*this)
    */
    LocatedDataRef* LocatedDataRef::clone() const
    {
      return new LocatedDataRef(*this);
    }
  } // NAMESPACE API

}  // NAMESPACE Mantid
