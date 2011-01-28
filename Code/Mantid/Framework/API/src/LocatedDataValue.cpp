#include "MantidAPI/LocatedDataValue.h"
#include "MantidKernel/Exception.h"

namespace Mantid
{

  namespace API
  {

    /**
    Standard Copy Constructor
    @param A :: LocatedDataValue Item to copy
    */
    LocatedDataValue::LocatedDataValue(const LocatedDataValue& A) : ILocatedData(),
      xValue(A.xValue),yValue(A.yValue),eValue(A.eValue),
      x2Value(A.x2Value),_isHistogram(A._isHistogram)
    {}

    /**
    Standard Copy Constructor
    @param A :: LocatedDataValue Item to copy
    */
    LocatedDataValue::LocatedDataValue(const ILocatedData& A) : ILocatedData(),
      xValue(A.X()),yValue(A.Y()),eValue(A.E()),
      x2Value(0),_isHistogram(A.isHistogram())
    {
      if (isHistogram())
      {
        x2Value = A.X2();
      }
    }

    /// Default constructor
    LocatedDataValue::LocatedDataValue(): ILocatedData(),
      xValue(0),yValue(0),eValue(0)
    {}

    /**
    Standard Assignment Constructor
    @param A :: LocatedDataValue Item to copy
    @return *this
    */
    LocatedDataValue& LocatedDataValue::operator=(const LocatedDataValue& A)
    {
      if (this!=&A)
      {
        xValue= A.xValue;
        x2Value= A.x2Value;
        yValue= A.yValue;
        eValue= A.eValue;
        _isHistogram = A._isHistogram;
      }
      return *this;
    }

    /**
    Standard Assignment Constructor
    @param A :: ILocatedData Item to copy
    @return *this
    */
    LocatedDataValue& LocatedDataValue::operator=(const ILocatedData& A)
    {
      if (this!=&A)
      {
        xValue= A.X();
        _isHistogram = A.isHistogram();
        if (_isHistogram)
        {
          x2Value= A.X2();
        }

        yValue= A.Y();
        eValue= A.E();
      }
      return *this;
    }

    /**
    Standard Destructor
    */
    LocatedDataValue::~LocatedDataValue()
    {
    }


    /** 
    Operator== all components must be equal
    @param A :: Other object to compare
    */
    int LocatedDataValue::operator==(const LocatedDataValue& A) const
    {
      return  (xValue!=A.xValue || x2Value!=A.x2Value || yValue!=A.yValue || 
        eValue!=A.eValue) ? 0 : 1;
    }

    /** 
    Operator!= any component is not equal
    @param A :: Other object to compare
    @return this!=A
    */
    int LocatedDataValue::operator!=(const LocatedDataValue& A) const
    {
      return  (xValue==A.xValue && x2Value==A.x2Value && yValue == A.yValue &&
        eValue== A.eValue) ? 0 : 1;
    }

    /** 
    Operator< takes xValue to last precidence.
    @param A :: LocatedDataValue to compare
    @return this < A
    */
    int LocatedDataValue::operator<(const LocatedDataValue& A) const
    {
      if (&A!=this)
      {
        if (xValue> A.xValue)
          return 0;
        if (xValue< A.xValue)
          return 1;
        if (x2Value> A.x2Value)
          return 0;
        if (x2Value< A.x2Value)
          return 1;
        if (yValue> A.yValue)
          return 0;
        if (yValue< A.yValue)
          return 1;
        if (eValue > A.eValue)
          return 0;
        if (yValue< A.yValue)
          return 1;
      }
      return 0;
    }

    /** 
    Operator> takes xValue to last precidence.
    Uses operator<  to obtain value.
    Note it does not uses 1-(A<this)
    @param A :: LocatedDataValue to compare
    @return this > A
    */
    int LocatedDataValue::operator>(const LocatedDataValue& A) const
    {
      return !(this->operator<(A));
    }

    /** Const accessor for X2
    @return The value of X2
    */
    const double& LocatedDataValue::X2() const
    {
      if (isHistogram())
      {
        return x2Value; 
      }
      else
      {
        throw Kernel::Exception::NotFoundError("X2 value is not set, check isHistogram() before accessing X2","X2");
      }
    }

    /** Accessor for X2
    @return The value of X2
    */
    double& LocatedDataValue::X2()
    {
      if (isHistogram())
      {
        return x2Value; 
      }
      else
      {
        throw Kernel::Exception::NotFoundError("X2 value is not set, check isHistogram() before accessing X2","X2");
      }
    }

    /** Const Accessor for X value
    @return The X value
    */
    const double& LocatedDataValue::X() const
    {
      return xValue; 
    }

    /** Accessor for X value
    @return The X value
    */
    double& LocatedDataValue::X()
    {
      return xValue; 
    }

    /** Const Accessor for Y value
    @return The Y value
    */
    const double& LocatedDataValue::Y() const
    {
      return yValue; 
    }

    /** Accessor for Y value
    @return The Y value
    */
    double& LocatedDataValue::Y()
    {
      return yValue; 
    }

    /** Const Accessor for E value
    @return The E value
    */
    const double& LocatedDataValue::E() const
    {
      return eValue; 
    }

    /** Accessor for E value
    @return The E value
    */
    double& LocatedDataValue::E()
    {
      return eValue; 
    }

    bool LocatedDataValue::isHistogram() const
    {
      return _isHistogram;
    }


  } // NAMESPACE API

}  // NAMESPACE Mantid
