#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>
#include <iterator>

#include <functional>

#include "AuxException.h"
#include "PolyBase.h"

namespace Mantid
{

namespace mathLevel
{

PolyBase::PolyBase (const int iD) : 
  iDegree((iD>0) ? iD : 0),
  afCoeff(iDegree+1) 
  /*!
    Constructor 
    \param iD :: degree
  */
{}

PolyBase::PolyBase (const PolyBase& A)  :
  iDegree(A.iDegree),afCoeff(A.afCoeff)
  /*! 
    Copy Constructor
    \param A :: PolyBase to copy
   */
{ }

PolyBase& 
PolyBase::operator=(const PolyBase& A)
  /*! 
    Assignment operator
    \param A :: PolyBase to copy
    \return *this
   */
{
  if (this!=&A)
    {
      iDegree = A.iDegree;
      afCoeff=A.afCoeff;
    }
  return *this;
}

PolyBase::~PolyBase ()
  /// Destructor
{}

void 
PolyBase::SetDegree(const int iD)
  /*!
    Set the degree value
    \param iD :: degree
   */
{
  iDegree = (iD>0) ? iD : 0;
  afCoeff.resize(iDegree+1);
}

int 
PolyBase::GetDegree () const 
  /*!
    Accessor to degree size
    \return size 
  */
{
  return iDegree;
}

PolyBase::operator const std::vector<double>& () const
  /*!
    Accessor to a vector component
    \return vector
  */
{
  return afCoeff;
}

PolyBase::operator std::vector<double>& ()
  /*!
    Accessor to a vector component
    \return vector
  */
{
  return afCoeff;
}


double
PolyBase::operator[](const int i) const
  /*!
    Accessor to component
    \param i :: index 
    \return Coeficient c_i
   */
{
  if (i>iDegree || i<0)
    throw ColErr::IndexError(i,iDegree+1,"PolyBase::operator[] const");
  return afCoeff[i];
}

double& 
PolyBase::operator[](const int i)
  /*!
    Accessor to component
    \param i :: index 
    \return Coeficient c_i
   */
{
  if (i>iDegree || i<0)
    throw ColErr::IndexError(i,iDegree+1,"PolyBase::operator[]");
  return afCoeff[i];
}

double 
PolyBase::operator()(const double X) const
  /*!
    Calculate polynomial at x
    \param x :: Value to calculate poly
    \return polyvalue(x)
  */
{
  double Result = afCoeff[iDegree];
  for (int i=iDegree-1; i >= 0; i--)
    {
      Result *= X;
      Result += afCoeff[i];
    }
  return Result;
}

PolyBase& 
PolyBase::operator+=(const PolyBase& A)
  /*!
    Self addition value
    \param A :: PolyBase to add 
    \return *this+=A;
   */
{
  iDegree=(iDegree>A.iDegree) ? iDegree : A.iDegree;
  afCoeff.resize(iDegree+1);
  for(int i=0;i<=iDegree && i<=A.iDegree;i++)
    afCoeff[i]+=A.afCoeff[i];
  return *this;
}

PolyBase& 
PolyBase::operator-=(const PolyBase& A)
  /*!
    Self addition value
    \param A :: PolyBase to add 
    \return *this+=A;
   */
{
  iDegree=(iDegree>A.iDegree) ? iDegree : A.iDegree;
  afCoeff.resize(iDegree+1);
  for(int i=0;i<=iDegree && i<=A.iDegree;i++)
    afCoeff[i]-=A.afCoeff[i];
  return *this;
}

PolyBase& 
PolyBase::operator*=(const PolyBase& A)
  /*!
    Self multiplication value
    \param A :: PolyBase to add 
    \return *this*=A;
   */
{
  const int iD=iDegree+A.iDegree;
  std::vector<double> CX(iD+1,0.0);   // all set to zero
  for(int i=0;i<=iDegree;i++)
    for(int j=0;j<=A.iDegree;j++)
      {
	const int cIndex=i+j;
	CX[cIndex]+=afCoeff[i]*A.afCoeff[j];
      }
  afCoeff=CX;
  return *this;
}

PolyBase 
PolyBase::operator+(const PolyBase& A) const
  /*!
    PolyBase addition
    \param A :: PolyBase addition
    \return (*this+A);
   */
{
  PolyBase kSum(*this);
  return kSum+=A;
}

PolyBase
PolyBase::operator-(const PolyBase& A) const
  /*!
    PolyBase subtraction
    \param A :: PolyBase addition
    \return (*this-A);
   */
{
  PolyBase kSum(*this);
  return kSum-=A;
}

PolyBase 
PolyBase::operator*(const PolyBase& A) const
  /*!
    PolyBase multiplication
    \param A :: PolyBase multiplication
    \return (*this*A);
   */
{
  PolyBase kSum(*this);
  return kSum*=A;
}

PolyBase 
PolyBase::operator+(const double V) const
  /*!
    PolyBase multiplication
    \param A :: PolyBase 
    \return (*this+A);
   */
{
  PolyBase kSum(*this);
  return kSum+=V;
}

PolyBase 
PolyBase::operator-(const double V) const
  /*!
    PolyBase substractr
    \param A :: PolyBase substract
    \return (*this-A);
   */
{
  PolyBase kSum(*this);
  return kSum-=V;
}

PolyBase 
PolyBase::operator*(const double V) const
  /*!
    PolyBase multiplication
    \param A :: PolyBase multiplication
    \return (*this*A);
   */
{
  PolyBase kSum(*this);
  return kSum*=V;
}

PolyBase 
PolyBase::operator/(const double V) const
  /*!
    PolyBase division
    \param A :: PolyBase Division
    \return (*this/A);
   */
{
  PolyBase kSum(*this);
  return kSum/=V;
}

// ---------------- SCALARS --------------------------

PolyBase&
PolyBase::operator+=(const double V) 
  /*!
    PolyBase addition
    \param V :: Value to add
    \return (*this+V);
   */
{
  afCoeff[0]+=V;  // There is always zero component
  return *this;
}

PolyBase&
PolyBase::operator-=(const double V) 
  /*!
    PolyBase subtraction
    \param V :: Value to subtract
    \return (*this+V);
   */
{
  afCoeff[0]-=V;  // There is always zero component
  return *this;
}

PolyBase&
PolyBase::operator*=(const double V) 
  /*!
    PolyBase multiplication
    \param V :: Value to multipication
    \return (*this*V);
   */
{
  transform(afCoeff.begin(),afCoeff.end(),afCoeff.begin(),
	    std::bind2nd(std::multiplies<double>(),V));
  return *this;
}

PolyBase&
PolyBase::operator/=(const double V) 
  /*!
    PolyBase division scalar
    \param V :: Value to divide
    \return (*this/V);
   */
{
  transform(afCoeff.begin(),afCoeff.end(),afCoeff.begin(),
	    std::bind2nd(std::divides<double>(),V));
  return *this;
}

PolyBase PolyBase::operator-() const
  /*!
    PolyBase negation operator
    \return -(*this);
   */
{
  PolyBase KOut(*this);
  KOut*= -1.0;
  return KOut;
}
 
PolyBase 
PolyBase::GetDerivative() const
  /*!
    Take derivative
    \return dP / dx
  */
{
  PolyBase KOut(*this);
  return KOut.derivative();
}

PolyBase&
PolyBase::derivative()
  /*!
    Take derivative of this polynomial
    \return dP / dx
  */
{
  if (iDegree<1)
    {
      afCoeff[0]=0.0;
      return *this;
    }

  for(int i=0;i<iDegree;i++)
    afCoeff[i]=afCoeff[i+1]*(i+1);
  iDegree--;

  return *this;
}

PolyBase 
PolyBase::GetInversion() const
  /*!
    Inversion of the coefficients
    \return (Poly)^-1
   */
{
  PolyBase InvPoly(iDegree);
  for (int i=0; i<=iDegree; i++)
    InvPoly.afCoeff[i] = afCoeff[iDegree-i];
  return InvPoly;
}

void 
PolyBase::Compress(const double Epsilon)
  /*!
    Two part process remove coefficients of zero or near zero: 
    Reduce degree by eliminating all (nearly) zero leading coefficients
    and by making the leading coefficient one.  The input parameter is
    the threshold for specifying that a coefficient is effectively zero.
    \param Epsilon (fraction to use
   */
{
  for (int i=iDegree; i>=0 && fabs(afCoeff[i])<=Epsilon;i--)
    iDegree--;
  
  if (iDegree >= 0)
    {
      const double InvLeading = 1.0/afCoeff[iDegree];
      afCoeff[iDegree] = 1.0;
      for (int i=0; i<iDegree;i++)
	afCoeff[i] *= InvLeading;
      afCoeff.resize(iDegree+1);
    }
  return;
}

void 
PolyBase::Divide(const PolyBase& pD,PolyBase& pQ, 
		 PolyBase& pR,const double Epsilon) const
  /*!
    Carry out polynomial division of this / pD  (Euclidean algorithm)
    If 'this' is P(t) and the divisor is D(t) with degree(P) >= degree(D),
    then P(t) = Q(t)*D(t)+R(t) where Q(t) is the quotient with
    degree(Q) = degree(P) - degree(D) and R(t) is the remainder with
    degree(R) < degree(D).  If this routine is called with
    degree(P) < degree(D), then Q = 0 and R = P are returned.  The value
    of epsilon is used as a threshold on the coefficients of the remainder
    polynomial.  If smaller, the coefficient is assumed to be zero.

    \param pD :: Polynominal to divide
    \param pQ :: Quotant
    \param pR :: Remainder
    \param Epsilon :: Toleracne
  */
{
  const int iQuotDegree = iDegree - pD.iDegree;
  if (iQuotDegree >= 0)
    {
      pQ.SetDegree(iQuotDegree);
      // temporary storage for the remainder
      pR = *this;
      
      // do the division 
      double fInv = 1.0/pD[pD.iDegree];
      for (int iQ=iQuotDegree;iQ>=0;iQ--)
        {
	  int iR = pD.iDegree + iQ;
	  pQ[iQ] = fInv*pR[iR];
	  for (iR--;iR>=iQ;iR--)
	    pR.afCoeff[iR] -= pQ[iQ]* pD[iR-iQ];
	}
      
      // calculate the correct degree for the remainder
      pR.Compress(Epsilon);
      return;
    }

  pQ.SetDegree(0);
  pQ[0] = 0.0;
  pR = *this;
  return;
}

}  // NAMESPACE  mathLevel

} // NAMESPACE Mantid
