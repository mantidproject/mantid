#include <iostream>
#include <cmath>
#include <complex>
#include <vector>
#include <algorithm>
#include <iterator>
#include <functional>
#include <gsl/gsl_poly.h>

#include "MantidKernel/Exception.h"
#include "MantidKernel/Support.h"
#include "MantidGeometry/Math/PolyVar.h"

#ifndef M_PI 
#define M_PI 3.1415926535897932384626433832795 
#endif

namespace Mantid
{
  namespace mathLevel
  {

    std::ostream& 
      operator<<(std::ostream& OX,const PolyVar<1>& A)
      /*!
      External Friend :: outputs point to a stream 
      \param OX :: output stream
      \param A :: PolyFunction to write
      \returns The output stream (OX)
      */
    {
      if (!A.write(OX))
        OX<<"0";
      return OX;
    }

    PolyVar<1>::PolyVar(const int iD) : 
    PolyFunction(),
      iDegree((iD>0) ? iD : 0),
      PCoeff(iDegree+1)
      /*!
      Constructor 
      \param iD :: degree
      */
    {
      fill(PCoeff.begin(),PCoeff.end(),0.0);
    }

    PolyVar<1>::PolyVar(const int iD,const double E) : 
    PolyFunction(E),
      iDegree((iD>0) ? iD : 0),
      PCoeff(iDegree+1)
      /*!
      Constructor 
      \param iD :: degree
      \param E :: Accuracy
      */
    {
      fill(PCoeff.begin(),PCoeff.end(),0.0);
    }

    PolyVar<1>::PolyVar(const PolyVar<1>& A)  :
    PolyFunction(A),iDegree(A.iDegree),PCoeff(A.PCoeff)
      /*! 
      Copy Constructor
      \param A :: PolyBase to copy
      */
    { }

    PolyVar<1>& 
      PolyVar<1>::operator=(const PolyVar<1>& A)
      /*! 
      Assignment operator
      \param A :: PolyBase to copy
      \return *this
      */
    {
      if (this!=&A)
      {
        PolyFunction::operator=(A);
        iDegree = A.iDegree;
        PCoeff=A.PCoeff;
      }
      return *this;
    }

    PolyVar<1>& 
      PolyVar<1>::operator=(const double& V)
      /*! 
      Assignment operator
      \param V :: Double value to set
      \return *this
      */
    {
      iDegree = 0;
      PCoeff.resize(1);
      PCoeff[0]=V;
      return *this;
    }

    PolyVar<1>::~PolyVar()
      /// Destructor
    {}

    void 
      PolyVar<1>::setDegree(const int iD)
      /*!
      Set the degree value
      \param iD :: degree
      */
    {
      iDegree = (iD>0) ? iD : 0;
      PCoeff.resize(iDegree+1);
    }

    void
      PolyVar<1>::zeroPoly()
      /*!
      Zeros each polynominal coefficient
      */
    {
      fill(PCoeff.begin(),PCoeff.end(),0.0);
      return;
    }


    int 
      PolyVar<1>::getDegree() const 
      /*!
      Accessor to degree size
      \return size 
      */
    {
      return iDegree;
    }

    PolyVar<1>::operator const std::vector<double>& () const
      /*!
      Accessor to a vector component
      \return vector
      */
    {
      return PCoeff;
    }

    PolyVar<1>::operator std::vector<double>& ()
      /*!
      Accessor to a vector component
      \return vector
      */
    {
      return PCoeff;
    }


    double
      PolyVar<1>::operator[](const int i) const
      /*!
      Accessor to component
      \param i :: index 
      \return Coeficient c_i
      */
    {
      if (i>iDegree || i<0)
        throw Kernel::Exception::IndexError(i,iDegree+1,"PolyBase::operator[] const");
      return PCoeff[i];
    }

    double& 
      PolyVar<1>::operator[](const int i)
      /*!
      Accessor to component
      \param i :: index 
      \return Coeficient c_i
      */
    {
      if (i>iDegree || i<0)
        throw Kernel::Exception::IndexError(i,iDegree+1,"PolyBase::operator[]");
      return PCoeff[i];
    }

    double 
      PolyVar<1>::operator()(const double X) const
      /*!
      Calculate polynomial at x
      \param X :: Value to calculate poly
      \return polyvalue(x)
      */
    {
      double Result = PCoeff[iDegree];              
      for (int i=iDegree-1; i >= 0; i--)
      {
        Result *= X;
        Result += PCoeff[i];
      }
      return Result;
    }

      /*!
      Calculate the value of the polynomial at a point
      \param DArray :: Values [x,y,z]
      \return the value at that point
      */
    double PolyVar<1>::operator()(const double* DArray) const
    {
      double X(1.0);
      double sum(0.0);
      const double xVal(DArray[0]);
      for(int i=0;i<=iDegree;i++)
      {
        sum+=PCoeff[i]*X;
        X*=xVal;
      }
      return sum;
    }

    double 
      PolyVar<1>::operator()(const std::vector<double>& DArray) const
      /*!
      Calculate the value of the polynomial at a point
      \param DArray :: Values [x,y,z]
      \return results
      */
    {
      if (DArray.empty())
        throw Kernel::Exception::IndexError(0,1,"PolVar<1>::operator()");
      double X(1.0);
      double sum(0.0);
      const double xVal(DArray[0]);
      for(int i=0;i<=iDegree;i++)
      {
        sum+=PCoeff[i]*X;
        X*=xVal;
      }
      return sum;
    }

    PolyVar<1>& 
      PolyVar<1>::operator+=(const PolyVar<1>& A)
      /*!
      Self addition value
      \param A :: PolyBase to add 
      \return *this+=A;
      */
    {
      iDegree=(iDegree>A.iDegree) ? iDegree : A.iDegree;
      PCoeff.resize(iDegree+1);
      for(int i=0;i<=iDegree && i<=A.iDegree;i++)
        PCoeff[i]+=A.PCoeff[i];
      return *this;
    }

    PolyVar<1>& 
      PolyVar<1>::operator-=(const PolyVar<1>& A)
      /*!
      Self addition value
      \param A :: PolyBase to add 
      \return *this+=A;
      */
    {
      iDegree=(iDegree>A.iDegree) ? iDegree : A.iDegree;
      PCoeff.resize(iDegree+1);
      for(int i=0;i<=iDegree && i<=A.iDegree;i++)
        PCoeff[i]-=A.PCoeff[i];
      return *this;
    }

    PolyVar<1>& 
      PolyVar<1>::operator*=(const PolyVar<1>& A)
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
          CX[cIndex]+=PCoeff[i]*A.PCoeff[j];
        }
        PCoeff=CX;
        return *this;
    }

    PolyVar<1> 
      PolyVar<1>::operator+(const PolyVar<1>& A) const
      /*!
      PolyBase addition
      \param A :: PolyBase addition
      \return (*this+A);
      */
    {
      PolyVar<1> kSum(*this);
      return kSum+=A;
    }

    PolyVar<1>
      PolyVar<1>::operator-(const PolyVar<1>& A) const
      /*!
      PolyBase subtraction
      \param A :: PolyBase addition
      \return (*this-A);
      */
    {
      PolyVar<1> kSum(*this);
      return kSum-=A;
    }

    PolyVar<1> 
      PolyVar<1>::operator*(const PolyVar<1>& A) const
      /*!
      PolyBase multiplication
      \param A :: PolyBase multiplication
      \return (*this*A);
      */
    {
      PolyVar<1> kSum(*this);
      return kSum*=A;
    }

    PolyVar<1> 
      PolyVar<1>::operator+(const double V) const
      /*!
      PolyBase multiplication
      \param V :: PolyBase 
      \return (*this+V);
      */
    {
      PolyVar<1> kSum(*this);
      return kSum+=V;
    }

    PolyVar<1> 
      PolyVar<1>::operator-(const double V) const
      /*!
      PolyVar<1> substractr
      \param V :: PolyBase substract
      \return (*this-V);
      */
    {
      PolyVar<1> kSum(*this);
      return kSum-=V;
    }

    PolyVar<1> 
      PolyVar<1>::operator*(const double V) const
      /*!
      PolyBase multiplication
      \param V :: PolyBase multiplication
      \return (*this*V);
      */
    {
      PolyVar<1> kSum(*this);
      return kSum*=V;
    }

    PolyVar<1> 
      PolyVar<1>::operator/(const double V) const
      /*!
      PolyBase division
      \param V :: PolyBase Division
      \return (*this/V);
      */
    {
      PolyVar<1> kSum(*this);
      return kSum/=V;
    }

    // ---------------- SCALARS --------------------------

    PolyVar<1>&
      PolyVar<1>::operator+=(const double V) 
      /*!
      PolyBase addition
      \param V :: Value to add
      \return (*this+V);
      */
    {
      PCoeff[0]+=V;  // There is always zero component
      return *this;
    }

    PolyVar<1>&
      PolyVar<1>::operator-=(const double V) 
      /*!
      PolyBase subtraction
      \param V :: Value to subtract
      \return (*this+V);
      */
    {
      PCoeff[0]-=V;  // There is always zero component
      return *this;
    }

    PolyVar<1>&
      PolyVar<1>::operator*=(const double V) 
      /*!
      PolyBase multiplication
      \param V :: Value to multipication
      \return (*this*V);
      */
    {
      transform(PCoeff.begin(),PCoeff.end(),PCoeff.begin(),
        std::bind2nd(std::multiplies<double>(),V));
      return *this;
    }

    PolyVar<1>&
      PolyVar<1>::operator/=(const double V) 
      /*!
      PolyBase division scalar
      \param V :: Value to divide
      \return (*this/V);
      */
    {
      transform(PCoeff.begin(),PCoeff.end(),PCoeff.begin(),
        std::bind2nd(std::divides<double>(),V));
      return *this;
    }

    PolyVar<1> 
      PolyVar<1>::operator-() const
      /*!
      PolyBase negation operator
      \return -(*this);
      */
    {
      PolyVar<1> KOut(*this);
      KOut*= -1.0;
      return KOut;
    }

    int
      PolyVar<1>::operator==(const PolyVar<1>& A) const
      /*!
      Determine if two polynomials are equal
      \param A :: Other polynomial to use
      \return 1 :: true 0 on false
      */
    {
      int i;
      for(i=0;i<=A.iDegree && i<=iDegree;i++)
        if (fabs(PCoeff[i]-A.PCoeff[i])>this->Eaccuracy)
          return 0;
      if (A.iDegree>iDegree)
      {
        for(;i<=A.iDegree;i++)
          if (fabs(A.PCoeff[i])>this->Eaccuracy)
            return 0;
      }
      else if (A.iDegree<iDegree)
      {
        for(;i<=iDegree;i++)
          if (fabs(PCoeff[i])>this->Eaccuracy)
            return 0;
      }
      return 1;
    }

    int
      PolyVar<1>::operator!=(const PolyVar<1>& A) const
      /*!
      Determine if two polynomials are different
      \param A :: Other polynomial to use
      \return 1 is polynomial differ:  0 on false
      */
    {
      return (this->operator==(A)) ? 0 : 1;
    }

    int
      PolyVar<1>::operator==(const double& V) const
      /*!
      Determine if a polynomial and a value are equal
      \param V :: Single value to use
      \return 1 equal:  0 on false
      */
    {
      if (fabs(PCoeff[0]-V)>this->Eaccuracy)
        return 0;
      for(int i=1;i<=iDegree;i++)
        if (fabs(PCoeff[i])>this->Eaccuracy)
          return 0;

      return 1;
    }

    int
      PolyVar<1>::operator!=(const double& V) const
      /*!
      Determine if a value and polynomial are different
      \param V :: Single value to use
      \return 1 is polynomial differs:  0 on false
      */
    {
      return (this->operator==(V)) ? 0 : 1;
    }


    PolyVar<1> 
      PolyVar<1>::getDerivative() const
      /*!
      Take derivative
      \return dP / dx
      */
    {
      PolyVar<1> KOut(*this);
      return KOut.derivative();
    }

    PolyVar<1>&
      PolyVar<1>::derivative()
      /*!
      Take derivative of this polynomial
      \return dP / dx
      */
    {
      if (iDegree<1)
      {
        PCoeff[0]=0.0;
        return *this;
      }

      for(int i=0;i<iDegree;i++)
        PCoeff[i]=PCoeff[i+1]*(i+1);
      iDegree--;

      return *this;
    }

    PolyVar<1> 
      PolyVar<1>::GetInversion() const
      /*!
      Inversion of the coefficients
      \return (Poly)^-1
      */
    {
      PolyVar<1> InvPoly(iDegree);
      for (int i=0; i<=iDegree; i++)
        InvPoly.PCoeff[i] = PCoeff[iDegree-i];
      return InvPoly;
    }

    void 
      PolyVar<1>::compress(const double epsilon)
      /*!
      Two part process remove coefficients of zero or near zero: 
      Reduce degree by eliminating all (nearly) zero leading coefficients
      and by making the leading coefficient one.  
      \param epsilon :: coeficient to use to decide if a values is zero
      */
    {
      const double eps((epsilon>0.0) ? epsilon : this->Eaccuracy);
      for (;iDegree>=0 && fabs(PCoeff[iDegree])<=eps;iDegree--);

      if (iDegree >= 0)
      {
        const double Leading = PCoeff[iDegree];
        PCoeff[iDegree] = 1.0;   // avoid rounding issues
        for (int i=0;i<iDegree;i++)
          PCoeff[i] /= Leading;
      }
      else
        iDegree=0;

      PCoeff.resize(iDegree+1);
      return;
    }

    void 
      PolyVar<1>::divide(const PolyVar<1>& pD,PolyVar<1>& pQ, 
      PolyVar<1>& pR,const double epsilon) const
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
      \param epsilon :: Tolerance  [-ve to use master tolerance]
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
            pR.PCoeff[iR] -= pQ[iQ]* pD[iR-iQ];
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
      PolyVar<1>::realRoots(const double epsilon)
      /*!
      Get just the real roots
      \param epsilon :: tolerance factor (-ve to use default)
      \return vector of the real roots (if any)
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
      PolyVar<1>::calcRoots(const double epsilon)
      /*!
      Calculate all the roots of the polynominal.
      Uses the GSL which uses a Hessian reduction of the 
      characteristic compainion matrix.
      \f[ A= \left( -a_{m-1}/a_m -a_{m-2}/a_m ... -a_0/a_m \right) \f]
      where the matrix component below A is the Indenty.
      However, GSL requires that the input coefficient is a_m == 1,
      hence the call to this->compress().
      \param epsilon :: tolerance factor (-ve to use default)
      \return roots (not sorted/uniqued)
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
        Out[0]=std::complex<double>(-PCoeff[0]);
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
      gsl_poly_complex_solve(&PCoeff.front(),iDegree+1, WS, RZ);
      for(int i=0;i<iDegree;i++)
        Out[i]=std::complex<double>(RZ[2*i],RZ[2*i+1]);
      gsl_poly_complex_workspace_free (WS);
      delete [] RZ;
      return Out;
    }

    int
      PolyVar<1>::solveQuadratic(std::complex<double>& AnsA,
      std::complex<double>& AnsB) const
      /*!
      Solves Complex Quadratic component.
      compress MUST have been called.
      \param AnsA :: complex roots of the equation 
      \param AnsB :: complex roots of the equation 
      \return number of unique solutions 
      */
    {
      const double b=PCoeff[1];
      const double c=PCoeff[0];

      const double cf=b*b-4.0*c;
      if (cf>=0)          /* Real Roots */
      {
        const double q=(b>=0) ? -0.5*(b+sqrt(cf)) : -0.5*(b-sqrt(cf));
        AnsA=std::complex<double>(q,0.0);
        AnsB=std::complex<double>(c/q,0.0);
        return (cf==0) ? 1 : 2;
      }

      std::complex<double> CQ(-0.5*b,0);
#ifdef _WIN32
      CQ.imag((b>=0) ?
        -0.5*sqrt(-cf) : 0.5*sqrt(-cf));
#else
      CQ.imag()= (b>=0) ?
        -0.5*sqrt(-cf) : 0.5*sqrt(-cf);
#endif
      AnsA = CQ;
      AnsB = c/CQ;
      return 2;
    }

    int
      PolyVar<1>::solveCubic(std::complex<double>& AnsA,std::complex<double>& AnsB,
      std::complex<double>& AnsC) const
      /*!
      Solves Cubic equation
      \param AnsA :: complex roots of the equation 
      \param AnsB :: complex roots of the equation 
      \param AnsC :: complex roots of the equation 
      \return number of unique solutions 
      */

    {
      double q,r;        /* solution parameters */
      double s,t,termR,termI,discrim;
      double q3,r13;
      std::pair<std::complex<double>,std::complex<double> > SQ;

      const double b = PCoeff[2];
      const double c = PCoeff[1];
      const double d = PCoeff[0];

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

          /*!
      Get the count
      \param eps :: Value to used
      \return The count
      */
    int PolyVar<1>::getCount(const double eps) const
    {
      int cnt(0);
      for(int i=0;i<=iDegree;i++)
        if (fabs(PCoeff[i])>eps)
          cnt++;
      return cnt++;
    }

          /*!
      Determine if is zero
      \param eps :: Value to used
      \return 0 if the value is 0, otherwise 1
      */
    int PolyVar<1>::isZero(const double eps) const
    {
      int i;
      for(i=0;i<=iDegree && fabs(PCoeff[i])<eps;i++);
      return (i<=iDegree) ? 0 : 1;
    }


    int 
      PolyVar<1>::isUnit(const double eps) const
      /*!
      Determine if is zero
      \param eps :: Value to used
      \retval 2 :: x unit set 
      \retval 1 :: +ve unit 
      \retval 0 :: Not unit
      \retval -1 :: -ve unit 
      */
    {
      int i;
      for(i=iDegree;i>0 && fabs(PCoeff[i])<eps;i--);
      if (i)
        return 0;
      if (fabs(fabs(PCoeff[0])-1.0)>eps)
        return 0;
      return (PCoeff[0]>0.0) ? 1 : -1;
    }


    int 
      PolyVar<1>::isUnitary(const double eps) const
      /*!
      Determine if is zero
      \param eps :: Value to used
      \retval 2 :: x unit set 
      \retval 1 :: +ve unit 
      \retval 0 :: Not unit
      \retval -1 :: -ve unit 
      */
    {
      int item(0);
      int flag(0);
      for(int i=iDegree;i>=0 && flag<2;i--)
      {
        if (fabs(PCoeff[i])>=eps)
        {
          item=i;
          flag++;
        }
      }
      if (flag==2 || flag==0) // all zeros are also NOT unit
        return 0;
      if (fabs(fabs(PCoeff[item])-1.0)>eps)
        return 0;
      const int sign( (PCoeff[item]>0.0) ? 1 : -1);
      return ((item==0) ? 1 : 2) * sign;
    }


    int
      PolyVar<1>::read(const std::string& Line)
      /*!
      Given a line of type 
      x^2+4.0x+3.0x 
      convert into a function:
      \return 0 :: success
      \return -1 :: failure
      */
    {
      const char Variable('x');
      std::string CLine=StrFunc::removeSpace(Line);
      setDegree(PolyFunction::getMaxSize(CLine,Variable));

      std::string::size_type pos=CLine.find(Variable);
      double cV;
      while (pos!=std::string::npos)
      {
        int compStart(pos);
        int sign(0);     // default is positive but not found
        // get preFunction 
        while(compStart>0)
        {
          compStart--;
          if (CLine[compStart]=='+' || CLine[compStart]=='-')
          {
            sign=(CLine[compStart]=='+') ? 1 : -1;
            break;
          } 
        }
        std::string Comp;      
        if (sign)
          Comp=CLine.substr(compStart+1,pos-1);
        else
        {
          Comp=CLine.substr(compStart,pos-compStart);
          sign=1;
        }

        // Find power
        int pV(1);
        CLine.erase(0,pos+1);
        if (!CLine.empty() && CLine[0]=='^')
        {
          CLine.erase(0,1);
          StrFunc::sectPartNum(CLine,pV);
        }

        if (pV<0 || (!Comp.empty() && !StrFunc::convert(Comp,cV)))
          throw std::invalid_argument("Invalid line in PolVarOne::read : " + Line);

        if (Comp.empty())
          cV=1.0;
        PCoeff[pV]=sign*cV;
        // Find next value
        pos=CLine.find(Variable);
      }

      // Process variable at end:
      if (!CLine.empty() && StrFunc::section(CLine,cV))
        PCoeff[0]=cV;

      return 0;
    }

    int
      PolyVar<1>::write(std::ostream& OX,const int prePlus) const
      /*!
      Basic write command
      \param OX :: output stream
      \param prePlus :: prePlus
      \return 0 :: value ==0.0
      */
    {
      int nowrite(0);
      for(int i=iDegree;i>=0;i--)
      {
        if (fabs(PCoeff[i])>this->Eaccuracy)
        {
          if ((nowrite || prePlus) && PCoeff[i]>0.0)
            OX<<"+";
          else if (PCoeff[i]<0.0)
            OX<<"-";
          // Now write value:
          if (!i || fabs(fabs(PCoeff[i])-1.0)>this->Eaccuracy)
            OX<<fabs(PCoeff[i]);
          if (i)
          {
            OX<<"x";
            if (i!=1)	OX<<"^"<<i;
          }
          nowrite=1;
        }
      }
      return nowrite;
    }  

  }  // NAMESPACE  mathLevel

} // NAMESPACE Mantid
