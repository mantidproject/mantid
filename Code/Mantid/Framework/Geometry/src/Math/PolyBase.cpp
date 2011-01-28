#include <iostream>
#include <cmath>
#include <complex>
#include <vector>
#include <algorithm>
#include <iterator>
#include <iterator>
#include <functional>
#include <gsl/gsl_poly.h>

#include "MantidKernel/Exception.h"
#include "MantidGeometry/Math/PolyBase.h"

namespace Mantid
{

namespace mathLevel
{

std::ostream& 
operator<<(std::ostream& OX,const PolyBase& A)
  /**
    External Friend :: outputs point to a stream 
    @param OX :: output stream
    @param A :: PolyBase to write
    @return The output stream (OX)
  */
{
  A.write(OX);
  return OX;
}

PolyBase::PolyBase (const int iD) : 
  iDegree((iD>0) ? iD : 0),
  afCoeff(iDegree+1),Eaccuracy(1e-6)
  /**
    Constructor 
    @param iD :: degree
  */
{}

PolyBase::PolyBase (const int iD,const double E) : 
  iDegree((iD>0) ? iD : 0),
  afCoeff(iDegree+1),Eaccuracy(fabs(E))
  /**
    Constructor 
    @param iD :: degree
    @param E :: Accuracy
  */
{}

PolyBase::PolyBase (const PolyBase& A)  :
  iDegree(A.iDegree),afCoeff(A.afCoeff),
  Eaccuracy(A.Eaccuracy)
  /** 
    Copy Constructor
    @param A :: PolyBase to copy
   */
{ }

PolyBase& 
PolyBase::operator=(const PolyBase& A)
  /** 
    Assignment operator
    @param A :: PolyBase to copy
    @return *this
   */
{
  if (this!=&A)
    {
      iDegree = A.iDegree;
      afCoeff=A.afCoeff;
      Eaccuracy=A.Eaccuracy;
    }
  return *this;
}

PolyBase::~PolyBase()
  /// Destructor
{}

void 
PolyBase::setDegree(const int iD)
  /**
    Set the degree value
    @param iD :: degree
   */
{
  iDegree = (iD>0) ? iD : 0;
  afCoeff.resize(iDegree+1);
}

int 
PolyBase::getDegree() const 
  /**
    Accessor to degree size
    @return size 
  */
{
  return iDegree;
}

PolyBase::operator const std::vector<double>& () const
  /**
    Accessor to a vector component
    @return vector
  */
{
  return afCoeff;
}

PolyBase::operator std::vector<double>& ()
  /**
    Accessor to a vector component
    @return vector
  */
{
  return afCoeff;
}


double
PolyBase::operator[](const int i) const
  /**
    Accessor to component
    @param i :: index 
    @return Coeficient c_i
   */
{
  if (i>iDegree || i<0)
    throw Kernel::Exception::IndexError(i,iDegree+1,"PolyBase::operator[] const");
  return afCoeff[i];
}

double& 
PolyBase::operator[](const int i)
  /**
    Accessor to component
    @param i :: index 
    @return Coeficient c_i
   */
{
  if (i>iDegree || i<0)
    throw Kernel::Exception::IndexError(i,iDegree+1,"PolyBase::operator[]");
  return afCoeff[i];
}

double 
PolyBase::operator()(const double X) const
  /**
    Calculate polynomial at x
    @param X :: Value to calculate poly
    @return polyvalue(x)
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
  /**
    Self addition value
    @param A :: PolyBase to add 
    @return *this+=A;
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
  /**
    Self addition value
    @param A :: PolyBase to add 
    @return *this+=A;
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
  /**
    Self multiplication value
    @param A :: PolyBase to add 
    @return *this*=A;
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
  /**
    PolyBase addition
    @param A :: PolyBase addition
    @return (*this+A);
   */
{
  PolyBase kSum(*this);
  return kSum+=A;
}

PolyBase
PolyBase::operator-(const PolyBase& A) const
  /**
    PolyBase subtraction
    @param A :: PolyBase addition
    @return (*this-A);
   */
{
  PolyBase kSum(*this);
  return kSum-=A;
}

PolyBase 
PolyBase::operator*(const PolyBase& A) const
  /**
    PolyBase multiplication
    @param A :: PolyBase multiplication
    @return (*this*A);
   */
{
  PolyBase kSum(*this);
  return kSum*=A;
}

PolyBase 
PolyBase::operator+(const double V) const
  /**
    PolyBase addition
    @param V :: Value Addtion
    @return (*this+V);
   */
{
  PolyBase kSum(*this);
  return kSum+=V;
}

PolyBase 
PolyBase::operator-(const double V) const
  /**
    PolyBase substractr
    @param V :: Value substract
    @return (*this-V);
   */
{
  PolyBase kSum(*this);
  return kSum-=V;
}

PolyBase 
PolyBase::operator*(const double V) const
  /**
    PolyBase multiplication
    @param V :: Value multiplication
    @return (*this*V);
   */
{
  PolyBase kSum(*this);
  return kSum*=V;
}

PolyBase 
PolyBase::operator/(const double V) const
  /**
    PolyBase division
    @param V :: Value division
    @return (*this/V);
   */
{
  PolyBase kSum(*this);
  return kSum/=V;
}

// ---------------- SCALARS --------------------------

PolyBase&
PolyBase::operator+=(const double V) 
  /**
    PolyBase addition
    @param V :: Value to add
    @return (*this+V);
   */
{
  afCoeff[0]+=V;  // There is always zero component
  return *this;
}

PolyBase&
PolyBase::operator-=(const double V) 
  /**
    PolyBase subtraction
    @param V :: Value to subtract
    @return (*this+V);
   */
{
  afCoeff[0]-=V;  // There is always zero component
  return *this;
}

PolyBase&
PolyBase::operator*=(const double V) 
  /**
    PolyBase multiplication
    @param V :: Value to multipication
    @return (*this*V);
   */
{
  transform(afCoeff.begin(),afCoeff.end(),afCoeff.begin(),
	    std::bind2nd(std::multiplies<double>(),V));
  return *this;
}

PolyBase&
PolyBase::operator/=(const double V) 
  /**
    PolyBase division scalar
    @param V :: Value to divide
    @return (*this/V);
   */
{
  transform(afCoeff.begin(),afCoeff.end(),afCoeff.begin(),
	    std::bind2nd(std::divides<double>(),V));
  return *this;
}

PolyBase PolyBase::operator-() const
  /**
    PolyBase negation operator
    @return -(*this);
   */
{
  PolyBase KOut(*this);
  KOut*= -1.0;
  return KOut;
}
 
PolyBase 
PolyBase::getDerivative() const
  /**
    Take derivative
    @return dP / dx
  */
{
  PolyBase KOut(*this);
  return KOut.derivative();
}

PolyBase&
PolyBase::derivative()
  /**
    Take derivative of this polynomial
    @return dP / dx
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
  /**
    Inversion of the coefficients
    @return (Poly)^-1
   */
{
  PolyBase InvPoly(iDegree);
  for (int i=0; i<=iDegree; i++)
    InvPoly.afCoeff[i] = afCoeff[iDegree-i];
  return InvPoly;
}

void 
PolyBase::compress(const double epsilon)
  /**
    Two part process remove coefficients of zero or near zero: 
    Reduce degree by eliminating all (nearly) zero leading coefficients
    and by making the leading coefficient one.  
    @param epsilon :: coeficient to use to decide if a values is zero
   */
{
  const double eps((epsilon>0.0) ? epsilon : Eaccuracy);
  for (;iDegree>=0 && fabs(afCoeff[iDegree])<=eps;iDegree--);

  if (iDegree >= 0)
    {
      const double Leading = afCoeff[iDegree];
      afCoeff[iDegree] = 1.0;   // avoid rounding issues
      for (int i=0;i<iDegree;i++)
	afCoeff[i] /= Leading;
    }
  else
    iDegree=0;

  afCoeff.resize(iDegree+1);
  return;
}

void 
PolyBase::divide(const PolyBase& pD,PolyBase& pQ, 
		 PolyBase& pR,const double epsilon) const
  /**
    Carry out polynomial division of this / pD  (Euclidean algorithm)
    If 'this' is P(t) and the divisor is D(t) with degree(P) >= degree(D),
    then P(t) = Q(t)*D(t)+R(t) where Q(t) is the quotient with
    degree(Q) = degree(P) - degree(D) and R(t) is the remainder with
    degree(R) < degree(D).  If this routine is called with
    degree(P) < degree(D), then Q = 0 and R = P are returned.  The value
    of epsilon is used as a threshold on the coefficients of the remainder
    polynomial.  If smaller, the coefficient is assumed to be zero.

    @param pD :: Polynominal to divide
    @param pQ :: Quotant
    @param pR :: Remainder
    @param epsilon :: Tolerance  [-ve to use master tolerance]
  */
{
  const int iQuotDegree = iDegree - pD.iDegree;
  if (iQuotDegree >= 0)
    {
      pQ.setDegree(iQuotDegree);
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
      pR.compress(epsilon);
      return;
    }

  pQ.setDegree(0);
  pQ[0] = 0.0;
  pR = *this;
  return;
}

std::vector<double> 
PolyBase::realRoots(const double epsilon)
  /**
    Get just the real roots
    @param epsilon :: tolerance factor (-ve to use default)
    @return vector of the real roots (if any)
   */
{
  const double eps((epsilon>0.0) ? epsilon : Eaccuracy);
  std::vector<std::complex<double> > Croots=calcRoots(epsilon);
  std::vector<double> Out;
  std::vector<std::complex<double> >::const_iterator vc;
  for(vc=Croots.begin();vc!=Croots.end();vc++)
    {
      if (fabs(vc->imag())<eps)
	Out.push_back(vc->real());
    }
  return Out;
}

std::vector<std::complex<double> >
PolyBase::calcRoots(const double epsilon)
  /**
    Calculate all the roots of the polynominal.
    Uses the GSL which uses a Hessian reduction of the 
    characteristic compainion matrix.
    \f[ A= \left( -a_{m-1}/a_m -a_{m-2}/a_m ... -a_0/a_m \right) \f]
    where the matrix component below A is the Indenty.
    However, GSL requires that the input coefficient is a_m == 1,
    hence the call to this->compress().
    @param epsilon :: tolerance factor (-ve to use default) 
    @return roots (not sorted/uniqued)
  */
{
  compress(epsilon);
  std::vector<std::complex<double> > Out(iDegree);
  // Zero State:
  if (iDegree==0)
    return Out;
  
  // x+a_0 =0 
  if (iDegree==1)
    {
      Out[0]=std::complex<double>(-afCoeff[0]);
      return Out;
    }
  // x^2+a_1 x+c = 0
  if (iDegree==2)
    {
      solveQuadratic(Out[0],Out[1]);
      return Out;
    }

  // x^3+a_2 x^2+ a_1 x+c=0
  if (iDegree==2)
    {
      solveCubic(Out[0],Out[1],Out[2]);
      return Out;
    }
  // THERE IS A QUARTIC / QUINTIC Solution availiable but...
  // WS contains the the hessian matrix if required (eigenvalues/vectors)
  //
  gsl_poly_complex_workspace* WS
    (gsl_poly_complex_workspace_alloc(iDegree+1));
  double* RZ=new double[2*(iDegree+1)];
  gsl_poly_complex_solve(&afCoeff.front(),iDegree+1, WS, RZ);
  for(int i=0;i<iDegree;i++)
    Out[i]=std::complex<double>(RZ[2*i],RZ[2*i+1]);
  gsl_poly_complex_workspace_free (WS);
  delete [] RZ;
  return Out;
}

int
PolyBase::solveQuadratic(std::complex<double>& AnsA,
			 std::complex<double>& AnsB) const
  /**
    Solves Complex Quadratic component.
    compress MUST have been called.
    @param AnsA :: complex roots of the equation 
    @param AnsB :: complex roots of the equation 
    @return Number of unique solutions 
  */
{
  const double b=afCoeff[1];
  const double c=afCoeff[0];
  
  const double cf=b*b-4.0*c;
  if (cf>=0)          /* Real Roots */
    {
      const double q=(b>=0) ? -0.5*(b+sqrt(cf)) : -0.5*(b-sqrt(cf));
      AnsA=std::complex<double>(q,0.0);
      AnsB=std::complex<double>(c/q,0.0);
      return (cf==0) ? 1 : 2;
    }

  std::complex<double> CQ(-0.5*b,0);
#ifndef MS_VISUAL_STUDIO
  CQ.imag() = (b>=0) ?
    -0.5*sqrt(-cf) : 0.5*sqrt(-cf);
#else
  CQ.imag((b>=0) ? -0.5*sqrt(-cf) : 0.5*sqrt(-cf));
#endif
  AnsA = CQ;
  AnsB = c/CQ;
  return 2;
}

int
PolyBase::solveCubic(std::complex<double>& AnsA,std::complex<double>& AnsB,
		     std::complex<double>& AnsC) const
  /**
    Solves Cubic equation
    Compress MUST have been called.
    @param AnsA :: complex roots of the equation 
    @param AnsB :: complex roots of the equation 
    @param AnsC :: complex roots of the equation 
    @return Number of unique solutions 
  */

{
  double q,r;        /* solution parameters */
  double s,t,termR,termI,discrim;
  double q3,r13;
  std::pair<std::complex<double>,std::complex<double> > SQ;

  const double b = afCoeff[2];
  const double c = afCoeff[1];
  const double d = afCoeff[0];
  
  q = (3.0*c - (b*b))/9.0;                   // -q
  r = -27.0*d + b*(9.0*c - 2.0*b*b);       // -r 
  r /= 54.0;
 
  discrim = q*q*q + r*r;           // r^2-qq^3 
  /* The first root is always real. */
  termR = (b/3.0);

  if (discrim > 1e-13)  /* one root real, two are complex */
    { 
      s = r + sqrt(discrim);
      s = ((s < 0) ? -pow(-s, (1.0/3.0)) : pow(s, (1.0/3.0)));
      t = r - sqrt(discrim);
      t = ((t < 0) ? -pow(-t, (1.0/3.0)) : pow(t, (1.0/3.0)));
      AnsA=std::complex<double>(-termR+s+t,0.0);
      // second real point.
      termR += (s + t)/2.0;
      termI = sqrt(3.0)*(-t + s)/2;
      AnsB=std::complex<double>(-termR,termI);
      AnsC=std::complex<double>(-termR,-termI);
      return 3;
    }

  /* The remaining options are all real */

  if (discrim<1e-13) // All roots real 
    {
      q = -q;
      q3 = q*q*q;
      q3 = acos(-r/sqrt(q3));
      r13 = -2.0*sqrt(q);
      AnsA=std::complex<double>(-termR + r13*cos(q3/3.0),0.0);
      AnsB=std::complex<double>(-termR + r13*cos((q3 + 2.0*M_PI)/3.0),0.0);
      AnsC=std::complex<double>(-termR + r13*cos((q3 - 2.0*M_PI)/3.0),0.0);
      return 3;
    }

// Only option left is that all roots are real and unequal 
// (to get here, q*q*q=r*r)

  r13 = ((r < 0) ? -pow(-r,(1.0/3.0)) : pow(r,(1.0/3.0)));
  AnsA=std::complex<double>(-termR+2.0*r13,0.0);
  AnsB=std::complex<double>(-(r13+termR),0.0);
  AnsC=std::complex<double>(-(r13+termR),0.0);
  return 2;
}

void
PolyBase::write(std::ostream& OX) const
  /**
    Basic write command
    @param OX :: output stream
  */
{
  copy(afCoeff.begin(),afCoeff.end(),
       std::ostream_iterator<double>(OX," "));
  return;
}  


}  // NAMESPACE  mathLevel

} // NAMESPACE Mantid
