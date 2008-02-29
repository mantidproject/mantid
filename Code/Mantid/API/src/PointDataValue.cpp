#include <iostream>
#include <string>
#include <stdexcept>
#include "MantidAPI/PointDataValue.h"
#include "MantidAPI/IErrorHelper.h"

namespace Mantid
{

  namespace API
  {

    /*!
    Standard Copy Constructor
    \param A :: PointDataValue Item to copy
    */
    PointDataValue::PointDataValue(const PointDataValue& A) : IPointData(),
    xValue(A.xValue),yValue(A.yValue),eValue(A.eValue),e2Value(A.e2Value),
      errorHelper(A.errorHelper),spectraNo(A.spectraNo)
    {}

    /*!
    Standard Copy Constructor
    \param A :: PointDataValue Item to copy
    */
    PointDataValue::PointDataValue(const IPointData& A) : IPointData(),
    xValue(A.X()),yValue(A.Y()),eValue(A.E()),e2Value(A.E2()),
      errorHelper(A.ErrorHelper()),spectraNo(A.SpectraNo())
    {}

    /// Default constructor
    PointDataValue::PointDataValue(): IPointData(),
      xValue(0),yValue(0),eValue(0),e2Value(0),
      errorHelper(0),spectraNo(0)
    {}

    /*!
    Standard Assignment Constructor
    \param A :: PointDataValue Item to copy
    \return *this
    */
    PointDataValue& PointDataValue::operator=(const PointDataValue& A)
    {
      if (this!=&A)
      {
        xValue= A.xValue;
        yValue= A.yValue;
        eValue= A.eValue;
        e2Value= A.e2Value;
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
    PointDataValue& PointDataValue::operator=(const IPointData& A)
    {
      if (this!=&A)
      {
        xValue= A.X();
        yValue= A.Y();
        eValue= A.E();
        e2Value= A.E2();
        errorHelper = A.ErrorHelper();
        spectraNo = A.SpectraNo();
      }
      return *this;
    }

    /*!
    Standard Destructor
    */
    PointDataValue::~PointDataValue()
    {
    }


    /*! 
    Operator== all components must be equal
    \param A :: Other object to compare
    */
    int PointDataValue::operator==(const PointDataValue& A) const
    {
      return  (xValue!=A.xValue || yValue!=A.yValue || 
        eValue!=A.eValue || e2Value!=A.e2Value) ? 0 : 1;
    }

    /*! 
    Operator!= any component is not equal
    \param A :: Other object to compare
    \return this!=A
    */
    int PointDataValue::operator!=(const PointDataValue& A) const
    {
      return  (xValue==A.xValue && yValue == A.yValue &&
        eValue== A.eValue  && e2Value!=A.e2Value) ? 0 : 1;
    }

    /*! 
    Operator< takes xValue to last precidence.
    \param A :: PointDataValue to compare
    \return this < A
    */
    int PointDataValue::operator<(const PointDataValue& A) const
    {
      if (&A!=this)
      {
        if (xValue> A.xValue)
          return 0;
        if (xValue< A.xValue)
          return 1;
        if (yValue> A.yValue)
          return 0;
        if (yValue< A.yValue)
          return 1;
        if (eValue > A.eValue)
          return 0;
        if (yValue< A.yValue)
          return 1;
        if (e2Value < A.e2Value)
          return 0;
      }
      return 0;
    }

    /*! 
    Operator> takes xValue to last precidence.
    Uses operator<  to obtain value.
    Note it does not uses 1-(A<this)
    \param A :: PointDataValue to compare
    \return this > A
    */
    int PointDataValue::operator>(const PointDataValue& A) const
    {
      return !(this->operator<(A));
    }

    /** Const Accessor for ErrorHelper class
    @return Pointer to the ErrorHelper class
    */
    const IErrorHelper* PointDataValue::ErrorHelper() const
    {
      return errorHelper; 
    }

    /** Const Accessor for Spectra Number
    @return The Spectra Number
    */
    int PointDataValue::SpectraNo() const
    {
      return spectraNo; 
    } 
    
    /** Const Accessor for X value
    @return The X value
    */
    const double& PointDataValue::X() const
    {
      return xValue; 
    }

    /** Accessor for X value
    @return The X value
    */
    double& PointDataValue::X()
    {
      return xValue; 
    }

    /** Const Accessor for Y value
    @return The Y value
    */
    const double& PointDataValue::Y() const
    {
      return yValue; 
    }

    /** Accessor for Y value
    @return The Y value
    */
    double& PointDataValue::Y()
    {
      return yValue; 
    }

    /** Const Accessor for E value
    @return The E value
    */
    const double& PointDataValue::E() const
    {
      return eValue; 
    }

    /** Accessor for E value
    @return The E value
    */
    double& PointDataValue::E()
    {
      return eValue; 
    }

    /** Const Accessor for E2 value
    @return The E2 value
    */
    const double& PointDataValue::E2() const
    {
      return e2Value; 
    }

    /** Accessor for E2 value
    @return The E2 value
    */
    double& PointDataValue::E2()
    {
      return e2Value; 
    }

    /*! Clone method
    *  Make a copy of the PointDataValue
    *  @return new(*this)
    */
    PointDataValue* PointDataValue::clone() const
    {
      return new PointDataValue(*this);
    }

  } // NAMESPACE API

}  // NAMESPACE Mantid
