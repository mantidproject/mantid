#include <iostream>
#include <cmath>
#include <complex>
#include <vector>
#include <algorithm>
#include <iterator>
#include <functional>
#include <gsl/gsl_poly.h>

#include "AuxException.h"
#include "PolyFunction.h"
#include "PolyVar.h"

namespace Mantid
{

namespace mathLevel
{

template<int VCount>
std::ostream& 
operator<<(std::ostream& OX,const PolyVar<VCount>& A)
  /*!
    External Friend :: outputs point to a stream 
    \param OX :: output stream
    \param A :: PolyFunction to write
    \returns The output stream (OX)
  */
{
  A.write(OX);
  return OX;
}

template<int VCount>
PolyVar<VCount>::PolyVar(const int iD) : 
  PolyFunction(),
  iDegree((iD>0) ? iD : 0),
  PCoeff(iDegree+1)
  /*!
    Constructor 
    \param iD :: degree
  */
{
  fill(PCoeff.begin(),PCoeff.end(),PolyVar<VCount-1>(0));
}

template<int VCount>
PolyVar<VCount>::PolyVar(const int iD,const double E) : 
  PolyFunction(E),
  iDegree((iD>0) ? iD : 0),
  PCoeff(iDegree+1)
  /*!
    Constructor 
    \param iD :: degree
    \param E :: Accuracy
  */
{
  fill(PCoeff.begin(),PCoeff.end(),PolyVar<VCount-1>(0,E));
}

template<int VCount>
PolyVar<VCount>::PolyVar(const PolyVar<VCount>& A)  :
  PolyFunction(A),iDegree(A.iDegree),
  PCoeff(A.PCoeff)
  /*! 
    Copy Constructor
    \param A :: PolyVar to copy
   */
{ }

template<int VCount>
template<int ICount>
PolyVar<VCount>::PolyVar(const PolyVar<ICount>& A) :
  iDegree(0),PCoeff(1)
  /*! 
    Copy constructor to a down reference
    \param A :: PolyVar to copy assign as PCoeff[0]
  */
{
  PCoeff[0]=A;
}

template<int VCount>
PolyVar<VCount>& 
PolyVar<VCount>::operator=(const PolyVar<VCount>& A)
  /*! 
    Assignment operator
    \param A :: PolyVar to copy
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

template<int VCount>
template<int ICount>
PolyVar<VCount>& 
PolyVar<VCount>::operator=(const PolyVar<ICount>& A)
  /*! 
    Assignment operator
    \param A :: PolyVar to copy
    \return *this
   */
{
  iDegree = 0;
  PCoeff.resize(1);
  PCoeff[0]=A;
  return *this;
}

template<int VCount>
PolyVar<VCount>& 
PolyVar<VCount>::operator=(const double& V)
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

template<int VCount> 
PolyVar<VCount>::~PolyVar()
  /// Destructor
{
}

template<int VCount>
void 
PolyVar<VCount>::setDegree(const int iD)
  /*!
    Set the degree value
    \param iD :: degree
   */
{
  const int xD = (iD>0) ? iD : 0;
  if (xD>iDegree)
    {
      for(int i=iDegree;i<=xD;i++)
	PCoeff.push_back(PolyVar<VCount-1>(0,this->Eaccuracy));
    }
  else   // reduction in power 
    PCoeff.resize(xD+1);

  iDegree=xD;
  return;
}

template<int VCount>
int 
PolyVar<VCount>::getDegree() const 
  /*!
    Accessor to degree size
    \return size 
  */
{
  return iDegree;
}

template<int VCount>
void
PolyVar<VCount>::setComp(const int Index,const double& V)
  /*!
    Set a component
    \param V :: Value
   */
{
  if (Index>iDegree || Index<0)
    throw ColErr::IndexError(Index,iDegree+1,"PolyVar::setComp(double)");
  PCoeff[Index] = V;
  return;  
}

template<int VCount>
template<int ICount>
void
PolyVar<VCount>::setComp(const int Index,const PolyVar<ICount>& FX)
  /*!
    Set a component
    \param FX :: Base compoenente
   */
{
  if (Index>iDegree || Index<0)
    throw ColErr::IndexError(Index,iDegree+1,"PolyVar::setComp(PolyBase)");
  PCoeff[Index] = FX;
  return;
}

template<int VCount>
double 
PolyVar<VCount>::operator()(const double* DArray) const
  /*!
    Calculate the value of the polynomial at a point
    \param DArray :: Values [x,y,z]
    \return Value of polynomial
  */
{
  double X(1.0);
  double sum(0.0);
  for(int i=0;i<=iDegree;i++)
    {
      sum+=PCoeff[i](DArray)*X;
      X*=DArray[VCount-1];
    }
  return sum;
}

template<int VCount>
double 
PolyVar<VCount>::operator()(const std::vector<double>& DArray) const
  /*!
    Calculate the value of the polynomial at a point
    \param DArray :: Values [x,y,z]
    \return Value of polynomial
  */
{
  if (DArray.size()<VCount)
    throw ColErr::IndexError(DArray.size(),VCount,"PolVar::operator()");
  double X(1.0);
  double sum(0.0);
  for(int i=0;i<=iDegree;i++)
    {
      sum+=PCoeff[i](DArray)*X;
      X*=DArray[VCount-1];
    }
  return sum;
}


template<int VCount>
PolyVar<VCount>&
PolyVar<VCount>::operator+=(const PolyVar<VCount>& A)
  /*!
    PolyVar addition
    \param A :: PolyVar to add
    \return (*this+A);
   */
{
  const int iMax((iDegree>A.iDegree)  ? iDegree : A.iDegree);
  PCoeff.resize(iMax);

  for(int i=0;i<=A.iDegree;i++)
    PCoeff[i]+=A.PCoeff[i];
  iDegree=iMax;
  return *this;
}

template<int VCount>
PolyVar<VCount>&
PolyVar<VCount>::operator-=(const PolyVar<VCount>& A)
  /*!
    PolyVar subtraction
    \param A :: PolyVar multiplication
    \return (*this*A);
   */
{
  const int iMax((iDegree>A.iDegree)  ? iDegree : A.iDegree);
  PCoeff.resize(iMax);

  for(int i=0;i<=A.iDegree;i++)
    PCoeff[i]-=A.PCoeff[i];
  iDegree=iMax;
  return *this;
}

template<int VCount>
PolyVar<VCount>&
PolyVar<VCount>::operator*=(const PolyVar<VCount>& A)
  /*!
    Multiply two Polynomials
    \param A :: PolyVar multiplication
    \return (*this*A);
   */
{
  std::vector<PolyVar<VCount-1> > POut(iDegree+A.iDegree+2); 
  std::vector<int> Zero(A.iDegree+1);
  transform(A.PCoeff.begin(),A.PCoeff.end(),Zero.begin(),
    std::bind2nd(std::mem_fun_ref(&PolyVar<VCount-1>::isZero),this->Eaccuracy));
  
  for(int i=0;i<=iDegree;i++)
    {
      if (!PCoeff[i].isZero(this->Eaccuracy))  // Cheaper than the calls to operator*
        {
	  for(int j=0;j<=A.iDegree;j++)
	    if (!Zero[j])
	      POut[i+j]+= PCoeff[i]*A.PCoeff[j];
	}
    }
  
  PCoeff=POut;
  compress();  
  return *this;
}

template<int VCount>
PolyVar<VCount> 
PolyVar<VCount>::operator+(const PolyVar<VCount>& A) const
  /*!
    PolyVar addition
    \param A :: PolyVar addition
    \return (*this+A);
   */
{
  PolyVar<VCount> kSum(*this);
  return kSum+=A;
}

template<int VCount>
PolyVar<VCount> 
PolyVar<VCount>::operator-(const PolyVar<VCount>& A) const
  /*!
    PolyVar addition
    \param A :: PolyVar addition
    \return (*this+A);
   */
{
  PolyVar<VCount> kSum(*this);
  return kSum-=A;
}

template<int VCount>
PolyVar<VCount> 
PolyVar<VCount>::operator*(const PolyVar<VCount>& A) const
  /*!
    PolyVar addition
    \param A :: PolyVar addition
    \return (*this+A);
   */
{
  PolyVar<VCount> kSum(*this);
  return kSum*=A;
}


// SCALAR BASICS

template<int VCount>
PolyVar<VCount> 
PolyVar<VCount>::operator+(const double V) const
  /*!
    PolyVar multiplication
    \param A :: PolyVar 
    \return (*this+A);
   */
{
  PolyVar<VCount> kSum(*this);
  return kSum+=V;
}

template<int VCount>
PolyVar<VCount> 
PolyVar<VCount>::operator-(const double V) const
  /*!
    PolyVar substractr
    \param A :: PolyVar substract
    \return (*this-A);
   */
{
  PolyVar<VCount> kSum(*this);
  return kSum-=V;
}

template<int VCount>
PolyVar<VCount>
PolyVar<VCount>::operator*(const double V) const
  /*!
    PolyVar multiplication
    \param A :: PolyVar multiplication
    \return (*this*A);
   */
{
  PolyVar<VCount> kSum(*this);
  return kSum*=V;
}

template<int VCount>
PolyVar<VCount>
PolyVar<VCount>::operator/(const double V) const
  /*!
    PolyVar division
    \param A :: PolyVar Division
    \return (*this/A);
   */
{
  PolyVar<VCount> kSum(*this);
  return kSum/=V;
}

//
// ---------------- SCALARS --------------------------
//

template<int VCount>
PolyVar<VCount>&
PolyVar<VCount>::operator+=(const double V) 
  /*!
    PolyVar addition
    \param V :: Value to add
    \return (*this+V);
   */
{
  PCoeff[0]+=V;
  return *this;
}

template<int VCount>
PolyVar<VCount>&
PolyVar<VCount>::operator-=(const double V) 
  /*!
    PolyVar subtraction
    \param V :: Value to subtract
    \return (*this+V);
   */
{
  PCoeff[0]-=V;  // There is always zero component
  return *this;
}

template<int VCount>
PolyVar<VCount>&
PolyVar<VCount>::operator*=(const double V)  
  /*!
    PolyVar multiplication
    \param V :: Value to multipication
    \return (*this*V);
   */
{
  typename std::vector< PolyVar<VCount-1> >::iterator vc;
  for(vc=PCoeff.begin();vc!=PCoeff.end();vc++)
    (*vc)*=V;
  return *this;
}

template<int VCount>
PolyVar<VCount>&
PolyVar<VCount>::operator/=(const double V) 
  /*!
    PolyVar division scalar
    \param V :: Value to divide
    \return (*this/V);
   */
{
  typename std::vector< PolyVar<VCount-1> >::iterator vc;
  for(vc=PCoeff.begin();vc!=PCoeff.end();vc++)
    (*vc)/=V;
  return *this;
}

template<int VCount>
PolyVar<VCount> 
PolyVar<VCount>::operator-() const
  /*!
    PolyVar negation operator
    \return -(*this);
   */
{
  PolyVar<VCount> KOut(*this);
  KOut *= -1.0;
  return KOut;
}
 
template<int VCount>
PolyVar<VCount> 
PolyVar<VCount>::getDerivative() const
  /*!
    Take derivative
    \return dP / dx
  */
{
  PolyVar<VCount> KOut(*this);
  return KOut.derivative();
}

template<int VCount>
PolyVar<VCount>&
PolyVar<VCount>::derivative()
  /*!
    Take derivative of this polynomial
    \return dP / dx
  */
{
  if (iDegree<1)
    {
      PCoeff[0] *= 0.0;
      return *this;
    }

  for(int i=0;i<iDegree;i++)
    PCoeff[i]=PCoeff[i+1] * (i+1.0);
  iDegree--;

  return *this;
}

template<int VCount>
PolyVar<VCount>
PolyVar<VCount>::GetInversion() const
  /*!
    Inversion of the coefficients
    Note that this is 
    \return (Poly)^-1
   */
{
  PolyVar<VCount> InvPoly(iDegree);
  for (int i=0; i<=iDegree; i++)
    InvPoly.PCoeff[i] = PCoeff[iDegree-i];  
  return InvPoly;
}

template<int VCount>
void 
PolyVar<VCount>::compress(const double epsilon)
  /*!
    Two part process remove coefficients of zero or near zero: 
    Reduce degree by eliminating all (nearly) zero leading coefficients
    and by making the leading coefficient one.  
    \param epsilon :: coeficient to use to decide if a values is zero
   */
{
  const double eps((epsilon>0.0) ? epsilon : this->Eaccuracy);
  for (;iDegree>=0 && PCoeff[iDegree].isZero(eps);iDegree--);

  if (iDegree > 0)
    {
      if (PCoeff[iDegree].isDouble())
        {
	  const double Leading = PCoeff[iDegree].getDouble();
	  for (int i=0;i<=iDegree;i++)
	      PCoeff[i] /= Leading;
	}
    }
  else
    iDegree=0;

  PCoeff.resize(iDegree+1);

  return;
}

template<int VCount>
int 
PolyVar<VCount>::getCount(const double eps) const
  /*!
    Determine number of 
    \param eps :: Value to used
  */
{
  int cnt(0);
  for(int i=0;i<=iDegree;i++)
    if(!PCoeff[i].isZero(eps))
      cnt++;
  return cnt;
}

template<int VCount>
int 
PolyVar<VCount>::isZero(const double eps) const
  /*!
    Determine if is zero
    \param eps :: Value to used
  */
{
  int i;
  for(i=0;i<=iDegree && PCoeff[i].isZero(eps);i++);
  return (i<=iDegree) ? 0 : 1;
}

template<int VCount>
void
PolyVar<VCount>::write(std::ostream& OX) const
  /*!
    Basic write command
    \param OX :: output stream
  */
{
  const char Variable("xyzabc"[VCount-1]);
  int first(1);
  for(int i=0;i<=iDegree;i++)
    {
      int zero(0);
      const int cnt=PCoeff[i].getCount(this->Eaccuracy);
      if (cnt)
        {
	  if (!first) OX<<" + ";
	  if (!i || cnt<2)
	    OX<<PCoeff[i];
	  else
	    OX<<"("<<PCoeff[i]<<")";
	}
      else 
	zero=1;

      if (!zero)
        {
	  first=0;
	  if (i)
	    {
	      OX<<Variable;
	      if (i!=1)
		OX<<"^"<<i;
	    }
	}
    }
  return;
}  

/// \cond TEMPLATE

template class PolyVar<1>;   // f(x,y) 
template class PolyVar<2>;   // f(x,y,z)
template class PolyVar<3>;   // f(x,y,z,a)

template std::ostream& operator<<(std::ostream&,const PolyVar<1>&);
template std::ostream& operator<<(std::ostream&,const PolyVar<2>&);
template std::ostream& operator<<(std::ostream&,const PolyVar<3>&);

template void PolyVar<2>::setComp(int,const PolyVar<1>&);

template PolyVar<2>& PolyVar<2>::operator=(const PolyVar<1>&);
template PolyVar<3>& PolyVar<3>::operator=(const PolyVar<1>&);
template PolyVar<3>& PolyVar<3>::operator=(const PolyVar<2>&);


/// \endcond TEMPLATE

}  // NAMESPACE  mathLevel

} // NAMESPACE Mantid
