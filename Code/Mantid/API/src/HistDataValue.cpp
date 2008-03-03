#include "MantidAPI/HistDataValue.h"
#include "MantidAPI/IErrorHelper.h"
#include "MantidKernel/Exception.h"

namespace Mantid
{

  namespace API
  {

    /*!
    Standard Copy Constructor
    \param A :: HistDataValue Item to copy
    */
    HistDataValue::HistDataValue(const HistDataValue& A) : IPointData(),
      xValue(A.xValue),yValue(A.yValue),eValue(A.eValue),e2Value(A.e2Value),
      errorHelper(A.errorHelper),spectraNo(A.spectraNo), x2Value(A.x2Value),_isHistogram(A._isHistogram)
    {}

    /*!
    Standard Copy Constructor
    \param A :: HistDataValue Item to copy
    */
    HistDataValue::HistDataValue(const IPointData& A) : IPointData(),
      xValue(A.X()),yValue(A.Y()),eValue(A.E()),e2Value(A.E2()),
      errorHelper(A.ErrorHelper()),spectraNo(A.SpectraNo()),
      x2Value(0),_isHistogram(A.isHistogram())
    {
      if (isHistogram())
      {
        x2Value = A.X2();
      }
    }

    /// Default constructor
    HistDataValue::HistDataValue(): IPointData(),
      xValue(0),yValue(0),eValue(0),e2Value(0),
      errorHelper(0),spectraNo(0), x2Value(0),_isHistogram(false)
    {}

    /*!
    Standard Assignment Constructor
    \param A :: HistDataValue Item to copy
    \return *this
    */
    HistDataValue& HistDataValue::operator=(const HistDataValue& A)
    {
      if (this!=&A)
      {
        xValue= A.xValue;
        x2Value= A.x2Value;
        yValue= A.yValue;
        eValue= A.eValue;
        e2Value= A.e2Value;
        errorHelper = A.errorHelper;
        spectraNo = A.spectraNo;
        _isHistogram = A._isHistogram;
      }
      return *this;
    }

    /*!
    Standard Assignment Constructor
    \param A :: IPointData Item to copy
    \return *this
    */
    HistDataValue& HistDataValue::operator=(const IPointData& A)
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
        if (A.isE2())
        {
          e2Value= A.E2();
        }
        errorHelper = A.ErrorHelper();
        spectraNo = A.SpectraNo();
      }
      return *this;
    }

    /*!
    Standard Destructor
    */
    HistDataValue::~HistDataValue()
    {
    }


    /*! 
    Operator== all components must be equal
    \param A :: Other object to compare
    */
    int HistDataValue::operator==(const HistDataValue& A) const
    {
      return  (xValue!=A.xValue || x2Value!=A.x2Value || yValue!=A.yValue || 
        eValue!=A.eValue || e2Value!=A.e2Value) ? 0 : 1;
    }

    /*! 
    Operator!= any component is not equal
    \param A :: Other object to compare
    \return this!=A
    */
    int HistDataValue::operator!=(const HistDataValue& A) const
    {
      return  (xValue==A.xValue && x2Value==A.x2Value && yValue == A.yValue &&
        eValue== A.eValue  && e2Value!=A.e2Value) ? 0 : 1;
    }

    /*! 
    Operator< takes xValue to last precidence.
    \param A :: HistDataValue to compare
    \return this < A
    */
    int HistDataValue::operator<(const HistDataValue& A) const
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
        if (e2Value < A.e2Value)
          return 0;
      }
      return 0;
    }

    /*! 
    Operator> takes xValue to last precidence.
    Uses operator<  to obtain value.
    Note it does not uses 1-(A<this)
    \param A :: HistDataValue to compare
    \return this > A
    */
    int HistDataValue::operator>(const HistDataValue& A) const
    {
      return !(this->operator<(A));
    }

    /** Const accessor for X2
    @return The value of X2
    */
    const double& HistDataValue::X2() const
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
    double& HistDataValue::X2()
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

    /** Const Accessor for ErrorHelper class
    @return Pointer to the ErrorHelper class
    */
    const IErrorHelper* HistDataValue::ErrorHelper() const
    {
      return errorHelper; 
    }

    /** Const Accessor for Spectra Number
    @return The Spectra Number
    */
    int HistDataValue::SpectraNo() const
    {
      return spectraNo; 
    } 

    /** Const Accessor for X value
    @return The X value
    */
    const double& HistDataValue::X() const
    {
      return xValue; 
    }

    /** Accessor for X value
    @return The X value
    */
    double& HistDataValue::X()
    {
      return xValue; 
    }

    /** Const Accessor for Y value
    @return The Y value
    */
    const double& HistDataValue::Y() const
    {
      return yValue; 
    }

    /** Accessor for Y value
    @return The Y value
    */
    double& HistDataValue::Y()
    {
      return yValue; 
    }

    /** Const Accessor for E value
    @return The E value
    */
    const double& HistDataValue::E() const
    {
      return eValue; 
    }

    /** Accessor for E value
    @return The E value
    */
    double& HistDataValue::E()
    {
      return eValue; 
    }

    /** Const Accessor for E2 value
    @return The E2 value
    */
    const double& HistDataValue::E2() const
    {
      return e2Value; 
    }

    /** Accessor for E2 value
    @return The E2 value
    */
    double& HistDataValue::E2()
    {
      return e2Value; 
    }

    const bool HistDataValue::isHistogram() const
    {
      return _isHistogram;
    }


  } // NAMESPACE API

}  // NAMESPACE Mantid
