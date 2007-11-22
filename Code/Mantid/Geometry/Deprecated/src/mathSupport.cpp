#include <iostream>
#include <cmath>
#include <complex>
#include <vector>
#include <iterator>
#include <algorithm>
#include <numeric>

#include "AuxException.h"
#include "mathSupport.h"

namespace Mantid
{

int
factorial(const int N)
  /*!
    Ugly function to get fractorial of N
    \param N :: Number to calculate N! 
    \return N!
  */
{
  int out(1);
  for(int i=2;i<N+1;i++)
    out*=i;
  return out;
}

double 
invErf(const double P)
  /*!
    Approximation to the inverse error function
    \param P :: point to calculate ierf(P) from
    \returns ierf(P)
  */
{
  const double a[]={ -0.5751703,-1.896513,-.5496261e-1 };
  const double b[]={ -0.1137730,-3.293474,-2.374996,-1.187515 }; 
  const double c[]={ -0.1146666,-0.1314774,-0.2368201,0.5073975e-1 };
  const double d[]={ -44.27977,21.98546,-7.586103 };
  const double e[]={ -0.5668422e-1,0.3937021,-0.3166501,0.6208963e-1 };
  const double f[]={ -6.266786,4.666263,-2.962883 };
  const double g[]={ 0.1851159E-3,-0.2028152E-2,-0.1498384,0.1078639e-1 };
  const double h[]={ 0.9952975E-1,0.5211733,-0.6888301e-1 };
  //  double  RINFM=1.7014E+38;
  if (P<-1 || P>1)
    return 0;

  const double Sigma=(P>0 ? 1 : -1);
  const double Z=fabs(P);
  double F;
  if (Z>0.85) 
    {
      const double A=1-Z;
      const double W=sqrt(-log(A+A*Z));
      if (W>=2.5) 
        {
	  if (W>=4.) 
	    {
	      const double wInv=1.0/W;
	      const double sn=((g[3]*wInv+g[2])*wInv+g[1])*wInv;
	      const double sd=((wInv+h[2])*wInv+h[1])*wInv+g[0];
	      F=W+W*(g[0]+sn/sd);
	    } 
	  else 
	    {
	      const double sn=((e[3]*W+e[2])*W+e[1])*W;
	      const double sd=((W+f[2])*W+f[1])*W+f[0];
	      F=W+W*(e[0]+sn/sd);
	    }
	} 
      else 
        {
	  const double sn=((c[3]*W+c[2])*W+c[1])*W;
	  const double sd=((W+d[2])*W+d[1])*W+d[0];
	  F=W+W*(c[0]+sn/sd);
	}
    } 
  else 
    {
      const double Z2=Z*Z;
      F=Z+Z*(b[0]+a[0]*Z2/(b[1]+Z2+a[1]/(b[2]+Z2+a[2]/(b[3]+Z2))));
    }
  return Sigma*F;
}

//double 
//normalDista(const double x)
//  /*!
//    Calcuates the inverse normal distribution usign Halleys
//    method.
//    \param x :: value to calulate from (0>x>1)
//    \returns 0 if x>1.0 or  x<0.0
//    \retval normal distribute to machine pecision.
//  */
//{
//  const double a[]={ -3.969683028665376e+01, 2.209460984245205e+02,
//                    -2.759285104469687e+02, 1.383577518672690e+02,
//		   -3.066479806614716e+01, 2.506628277459239e+00 };
//  
//  const double b[]= { -5.447609879822406e+01, 1.615858368580409e+02,
//                  -1.556989798598866e+02, 6.680131188771972e+01,
//                  -1.328068155288572e+01 };
//
//  const double c[]= { -7.784894002430293e-03, -3.223964580411365e-01,
//		      -2.400758277161838e+00, -2.549732539343734e+00,
//		      4.374664141464968e+00, 2.938163982698783e+00 };
//
//  const double d[] = { 7.784695709041462e-03, 3.224671290700398e-01,
//		       2.445134137142996e+00, 3.754408661907416e+00 };
//
//  const double pLow(0.02425);
//  const double pHigh=(1.0-pLow);
//
//  double xOut;
//  
//  if (x>=1.0 || x<=0.0)
//    return 0.0;
//
//  if (x<pLow)
//    {
//      const double q = sqrt(-2.0*log(x));
//      xOut= (((((c[0]*q+c[1])*q+c[2])*q+c[3])*q+c[4])*q+c[5]) / ((((d[0]*q+d[1])*q+d[2])*q+d[3])*q+1);
//    }
//  else if (x>pHigh)
//    {
//      const double q = sqrt(-2.0*log(1-x));
//      xOut= -(((((c[0]*q+c[1])*q+c[2])*q+c[3])*q+c[4])*q+c[5]) / ((((d[0]*q+d[1])*q+d[2])*q+d[3])*q+1);
//    }
//  else
//    {
//      const double q = x - 0.5;
//      const double r = q*q;
//      xOut= (((((a[0]*r+a[1])*r+a[2])*r+a[3])*r+a[4])*r+a[5])*q /
//	(((((b[0]*r+b[1])*r+b[2])*r+b[3])*r+b[4])*r+1);
//    }
//
////This will NOT WORK DO NOT USE IT
//  const double e = 0.5; // * erfc(-xOut/sqrt(2.0)) - x;
//  const double u = e * sqrt(2*M_PI) * exp(xOut*xOut/2);
//  return xOut - u/(1 + xOut*u/2);
//}

double 
randomNormal() 
  /*!
   A normally distributed random number generator.  We avoid
   the uniform rv's being 0.0 since this will result in infinte
   values, and double count the 0 == 2pi.
  */
{ 
  static int i = 1;
  static double u[2] = {0.0, 0.0};
  double r[2];
  
  if ( i==1 ) 
    {
      r[0] = sqrt(-2.0*log(ran()));
      r[1] = 2*M_PI*(ran());
      u[0] = r[0]*sin(r[1]);
      u[1] = r[0]*cos(r[1]);
      i = 0;
    } 
  else 
    {
      i = 1;
    }
 return u[i];
};

template<typename T>
double
norm(const std::vector<T>& Vec)
  /*!
    Function to calculate the mean of a vector.
    \param Vec :: vector to use
    \returns \f$ \sqrt{V.V} \f$
  */
{
  return sqrt(inner_product(Vec.begin(),Vec.end(),Vec.begin(),
			    0.0,
			    std::plus<double>(), 
			    std::multiplies<double>()
			    )
	      );
}


float 
ran()
{
  static int ids = 77564453;
  return ran1(ids);
}

float 
ran1(int &idum,const int start)
  /*!
    Random number generator (obsolete)
    \param idum :: seed (updated)
    \param start :: reinitialise flag
    \returns random number 0->1 
  */
{   
  const int IA(16807);
  const int IM(2147483647);
  const float AM(1.0/IM);
  const int IQ(127773);
  const int IR(2836);
  const int NTAB(32);
  const int NDIV(1+(IM-1)/NTAB);
  const float EPS(1.2e-7);
  const float RNMX(1.0-EPS);

  int j,k;
  static int iy=0;
  static int iv[NTAB];
  float temp;
    
  if ( idum<=0 || !iy || start)
    {
      if (-idum < 1) idum=1;
      else idum = -idum;
      for (j=NTAB+7;j>=0;j--)
	{
	  k=idum/IQ;
	  idum=IA*(idum-k*IQ)-IR*k;
	  if (idum<0) idum += IM;
	  if (j<NTAB) iv[j]=idum;
	}
      iy=iv[0];
    }
  k=idum/IQ;
  idum=IA * (idum-k * IQ)-IR * k;
  if (idum<0) idum += IM;
  j=iy/NDIV;
  iy=iv[j];
  iv[j]= idum;
  return ((temp=AM*iy)>RNMX) ? RNMX : temp;
}

template<typename T>
void
indexSort(const std::vector<T>& pVec,std::vector<int>& Index)
  /*!
    Function to take a vector and sort the vector 
    so as to produce an index.
    Leaves the vector unchanged.
  */
{
  Index.resize(pVec.size());
  std::vector<typename std::pair<T,int> > PartList;
  PartList.resize(pVec.size());

  transform(pVec.begin(),pVec.end(),PartList.begin(),mathSupport::PIndex<T>());
  sort(PartList.begin(),PartList.end());
  transform(PartList.begin(),PartList.end(),Index.begin(),mathSupport::PSep<T>());
  
  /*
    typename std::vector<typename std::pair<T,int> >::const_iterator vc;
    
    for(vc=PartList.begin();vc!=PartList.end();vc++)
      {
        std::cout<<"Part == "<<vc->first<<" "<<vc->second<<std::endl;
      }
  */
  return;
}

double
quad(const double aa,const double bb,const double cc,const double x)
  /*!
    \return value of the quadratic give by
    \f[ ax^2+bx+c \f]
  */
{
  return aa*x*x+bb*x+cc;
}

template<typename InputIter>
int
solveQuadratic(const InputIter Coef,std::pair<std::complex<double>,
	       std::complex<double> >& OutAns)
  /*!
    Solves Complex Quadratic 
    \param Coef :: iterator over all the coefients in the order
    \f[ Ax^2+Bx+C \f].
    \param OutAns :: complex roots of the equation 
    \return number of unique solutions 
  */
{
  double a,b,c,cf;
  a=(*Coef); 
  b=*(Coef+1);
  c=*(Coef+2);

  if (a==0.0)
    {
      if (b==0.0)
	{
	  OutAns.first=std::complex<double>(0.0,0.0);
	  OutAns.second=std::complex<double>(0.0,0.0);
	  return 0;
	}
      else
	{
	  OutAns.first=std::complex<double>(-c/b,0.0);
	  OutAns.second=OutAns.first;
	  return 1;
	}
    }
  cf=b*b-4*a*c;
  if (cf>=0)          /* Real Roots */
    {
      const double q=(b>=0) ? -0.5*(b+sqrt(cf)) : -0.5*(b-sqrt(cf));
      OutAns.first=std::complex<double>(q/a,0.0);
      OutAns.second=std::complex<double>(c/q,0.0);
      return (cf==0) ? 1 : 2;
    }

  std::complex<double> CQ(-0.5*b,0);
#ifndef _WIN32
  CQ.imag() = (b>=0) ?
    -0.5*sqrt(-cf) : 0.5*sqrt(-cf);
#else
  CQ.imag((b>=0) ? -0.5*sqrt(-cf) : 0.5*sqrt(-cf));
#endif
  OutAns.first= CQ/a;
  OutAns.second=c/CQ;
  return 2;
}

template<typename InputIter>
int
solveCubic(const InputIter Coef,std::complex<double>& AnsA,
	   std::complex<double>& AnsB,std::complex<double>& AnsC)
  /*!
    Solves Cubic equation
    \param Coef :: iterator over all the coefients in the order
    \f[ Ax^3+Bx^2+Cx+D \f].
    \param AnsA,AnsB,AnsC :: complex roots of the equation 
    \return number of unique solutions 
  */

{
  typedef std::complex<double> Cpair;
  double q,r;        /* solution parameters */
  double s,t,termR,termI,discrim;
  double q3,r13;
  std::pair<std::complex<double>,std::complex<double> > SQ;

  if ((*Coef)==0)
    {
      const int xi=solveQuadratic(Coef+1,SQ);
      AnsA=SQ.first;
      AnsB=SQ.second;
      AnsC=SQ.second;
      return xi;
    }
  if (*(Coef+3)==0)
    { 
     const int xi=solveQuadratic(Coef,SQ);
     std::cerr<<"Xi == "<<xi<<std::endl;
      AnsA=SQ.first;
      AnsB=(xi==1) ? SQ.first : SQ.second;
      AnsC=std::complex<double>(0.0,0.0);
      return (AnsC!=AnsA) ? xi+1 : xi;
    }
  const double a= (*Coef);
  const double b = *(Coef+1)/ a;
  const double c = *(Coef+2)/ a;
  const double d = *(Coef+3)/ a;
  
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
      AnsA=Cpair(-termR+s+t,0.0);
      // second real point.
      termR += (s + t)/2.0;
      termI = sqrt(3.0)*(-t + s)/2;
      AnsB=Cpair(-termR,termI);
      AnsC=Cpair(-termR,-termI);
      return 3;
    }

  /* The remaining options are all real */

  if (discrim<1e-13) // All roots real 
    {
      q = -q;
      q3 = q*q*q;
      q3 = acos(-r/sqrt(q3));
      r13 = -2.0*sqrt(q);
      AnsA=Cpair(-termR + r13*cos(q3/3.0),0.0);
      AnsB=Cpair(-termR + r13*cos((q3 + 2.0*M_PI)/3.0),0.0);
      AnsC=Cpair(-termR + r13*cos((q3 - 2.0*M_PI)/3.0),0.0);
      return 3;
    }

// Only option left is that all roots are real and unequal 
// (to get here, q*q*q=r*r)

  r13 = ((r < 0) ? -pow(-r,(1.0/3.0)) : pow(r,(1.0/3.0)));
  AnsA=Cpair(-termR+2.0*r13,0.0);
  AnsB=Cpair(-(r13+termR),0.0);
  AnsC=Cpair(-(r13+termR),0.0);
  return 2;
}

template<typename T> 
typename std::vector<T>::const_iterator
iteratorPos(const std::vector<T>& xArray,const T& Aim)
  /*!
    Function finds the iterator position in xArray that 
    which correspond to the next value after aim
    \param xArray :: vector of points in array
    \param Aim :: Point to aim for
    \retval iterator in xArray if Aim>= first or Aim<=last
    \retval xArray.end() if Aim outside of xArray
  */
{
  if (Aim>xArray.back() || Aim<xArray[0])
    xArray.end();

  typename std::vector<T>::const_iterator xV=lower_bound(xArray.begin(),xArray.end(),Aim);
  return xV;
}

template<typename Xtype,typename Ytype>
Ytype
polInterp(const Xtype& Aim,const int Order,
	  const std::vector<Xtype>& Xpts,const std::vector<Ytype>& Ypts) 
  /*!
    Impliments a polynominal fit on data
    from around Order values of Aim. Note that Xpts and ypts
    does not need to be the same type.
    \param Aim :: X value to interpolate to
    \param Order :: Polynominal order 
    \param Xpts :: X-array 
    \param Ypts :: Y-array 
    \return Interpolated point [ y(X) ]
  */
{

  typename std::vector<Xtype>::const_iterator xP=iteratorPos<Xtype>(Xpts,Aim);

  if (xP==Xpts.end())  // indicates out of range
    {
      return (Aim>Xpts.back()) ?
	Ypts[Xpts.size()-1] : Ypts[0];
    }
  typename std::vector<Ytype>::const_iterator yP=Ypts.begin()+distance(Xpts.begin(),xP);
  // Calculate the size of the fit and points 
  const int Orange=(Order+1)/2;   
  const int pt=static_cast<int>(distance(Xpts.begin(),xP));
  int Osize(0);             // number of point in fit
  int stx(-Orange);         //start point
  for(int i=-Orange;i<Orange;i++)
    {
      if ((i+pt>=0) && (i+pt<static_cast<int>(Xpts.size())))
	Osize++;
      else if (Osize==0)
	stx=i;
      else
	break;
    }
  stx++;
  return polFit<Xtype,Ytype>(Aim,Osize,xP+stx,yP+stx);
}


template<typename Xtype,typename Ytype>
Ytype
polFit(const Xtype& Aim,const int Order,
	   typename std::vector<Xtype,std::allocator<Xtype> >::const_iterator X,
	   typename std::vector<Ytype,std::allocator<Ytype> >::const_iterator Y)
  /*!
    Function to carry out interpolation
    \param Aim :: aim point
    \param Order :: value of the interator
    \param X :: X iterator point (at start not middle)
    \param Y :: Y iterator point corresponding to X
   */
{
  
  Xtype testDiff,diff;

  std::vector<Ytype> C(Order);      //  C and D Var
  std::vector<Ytype> D(Order);      // 


  int ns(0);
  
  diff=fabs(Aim-X[0]);
  C[0]=Y[0];
  D[0]=Y[0];
  for(int i=1;i<Order;i++)
    {
      testDiff=fabs(Aim-X[i]);
      if (diff>testDiff)
        {
          ns=i;
          diff=testDiff;
        }
      C[i]=Y[i];
      D[i]=Y[i];
    }

  Ytype out=Y[ns];
  ns--;                   // Now can be -1 !!!! 

  Ytype den;
  Ytype w;
  Ytype outSigma;
  Xtype ho,hp;           // intermediate variables 

  for(int m=1;m<Order;m++)
    {
      for(int i=0;i<Order-m;i++)
        {
          ho=X[i]-Aim;
          hp=X[i+m]-Aim;
          w=C[i+1]-D[i];
          /*      den=ho-hp;  -- test !=0.0 */
          den=w/static_cast<Ytype>(ho-hp);
          D[i]=static_cast<Ytype>(hp)*den;
          C[i]=static_cast<Ytype>(ho)*den;
        }
      outSigma= (2*(ns+1)<(Order-m)) ? C[ns+1] : D[ns--];
      out+=outSigma;
    }
  return out;
}

template<typename T>
T
intQuadratic(const typename std::vector<T>::const_iterator& Xpts,
	     const typename std::vector<T>::const_iterator& Ypts) 
  /*!
    This function carries out a quadratic polynominal integration
    for the three points Xpts,Ypts. It assumes that Xpts are random.
    It is designed to be used in 
    
  */
{
  const T C =  Ypts[0];
  const T x1= Xpts[0];
  const T x2= Xpts[1]-x1;
  const T x3= Xpts[2]-x1;
  const T y2= Ypts[1]-C;
  const T y3= Ypts[2]-C;

  const T FracD=x2*x3*x3-x2*x2*x3;
  const T B= (x3*x3*y2-x2*x2*y3)/FracD;
  const T A= -(x3*y2-x2*y3)/FracD;

  return x3*(C+x3*B/2.0+x3*x3*A/3.0);
}

template<typename T>
T
derivQuadratic(const typename std::vector<T,std::allocator<T> >::const_iterator& Xpts,
			   const typename std::vector<T,std::allocator<T> >::const_iterator& Ypts) 
  /*!
    This function carries out a quadratic polynominal differentuation
    for the three points Xpts,Ypts. It assumes that Xpts are random.
  */
{
  const T C =  Ypts[0];
  const T x1= Xpts[0];
  const T x2= Xpts[1]-x1;
  const T x3= Xpts[2]-x1;
  const T y2= Ypts[1]-C;
  const T y3= Ypts[2]-C;

  const T FracD=x2*x3*x3-x2*x2*x3;
  const T B= (x3*x3*y2-x2*x2*y3)/FracD;
  const T A= -(x3*y2-x2*y3)/FracD;

  return 2.0*A*x2+B;
}

template<typename T>
int
mathFunc::binSearch(const typename std::vector<T>::const_iterator& pVecB,
		    const typename std::vector<T>::const_iterator& pVecE,
		    const T& V)
  /*!
    Determine a binary search of a component
    \param pVecB :: start point in vector list
    \param pVecE :: end point in vector list
    \param V :: Item to search for
    \return position index
  */
{
  if (*pVecB>=V)
    return 0;
  if (*(pVecE-1)<=V)
    return distance(pVecB,pVecE)-1;
  
  typename std::vector<T>::const_iterator   
    vc=lower_bound(pVecB,pVecE,V,std::less<T>());

  return distance(pVecB,vc);
}

template<typename T,typename U>
void
mathFunc::crossSort(std::vector<T>& pVec,std::vector<U>& Base)
  /*!
    Function to take a vector and sort the vector and
    update Base in the equivalent manor.
    \param pVec :: Vector of variables to index
    \param Base :: 
  */
{
  if (pVec.size()!=Base.size())
    throw ColErr::MisMatch<int>(pVec.size(),Base.size(),"mathFunc::crossSort");

  std::vector<typename std::pair<T,U> > Index(pVec.size());
  transform(pVec.begin(),pVec.end(),Base.begin(),
	    Index.begin(),mathSupport::PCombine<T,U>());
  sort(Index.begin(),Index.end());

  typename std::vector<std::pair<T,U> >::const_iterator vc;
  typename std::vector<T>::iterator ac=pVec.begin();
  typename std::vector<U>::iterator bc=Base.begin();
  for(vc=Index.begin();vc!=Index.end();vc++,ac++,bc++)
    {
      *ac = vc->first;
      *bc = vc->second;
    }
  return;
}


template<typename T>
void 
mathFunc::Order(T& A,T& B)
  /*!
    Function to order A,B
    \param A : set to min(A,B)
    \param B : set to max(A,B)
  */
{
  if (A>B)
    {
      T tmp=B;
      B=A;
      A=tmp;
    }
  return;
}


template<typename T>
void 
mathFunc::Swap(T& A,T& B)
  /*!
    Function to swap A,B
    \param A : set to make B
    \param B : set to make A
  */
{
  const T tmp=B;
  B=A;
  A=tmp;
  return;
}

/// \cond TEMPLATE 

template
int solveQuadratic(const double*,
       std::pair<std::complex<double>,std::complex<double> >&);
template
int solveQuadratic(double*,
       std::pair<std::complex<double>,std::complex<double> >&);
template
int solveQuadratic(const std::vector<double>::const_iterator,
       std::pair<std::complex<double>,std::complex<double> >&);
template
int solveCubic(const double*,std::complex<double>&,
	       std::complex<double>&,std::complex<double>&);
template
int solveCubic(const std::vector<double>::iterator,std::complex<double>&,
	       std::complex<double>&,std::complex<double>&);
template
int solveCubic(const std::vector<double>::const_iterator,std::complex<double>&,
	       std::complex<double>&,std::complex<double>&);


/*
template 
std::complex<double> 
polFit(const double&,const int,
	   std::vector<double,std::allocator<double> >::const_iterator,
	   std::vector<std::complex<double>,std::allocator<std::complex<double> > >::const_iterator);
*/

template 
double 
polFit(const double&,const int,
	   std::vector<double,std::allocator<double> >::const_iterator,
	   std::vector<double,std::allocator<double> >::const_iterator);

template 
std::complex<double> 
polInterp(const double&,const int,const std::vector<double>&,
	  const std::vector<std::complex<double> >&);

template 
std::complex<long double> 
polInterp(const double&,const int,const std::vector<double>&,
	  const std::vector<std::complex<long double> >&);

template 
double 
polInterp(const double&,const int,const std::vector<double>&,
	  const std::vector<double>&);

template 
double
intQuadratic(const std::vector<double>::const_iterator&,
	     const std::vector<double>::const_iterator&);

template 
double
derivQuadratic(const std::vector<double>::const_iterator&,
	       const std::vector<double>::const_iterator&);


template class mathSupport::PIndex<double>;
template class mathSupport::PSep<double>;

template void indexSort(const std::vector<double>&,std::vector<int>&);
template void indexSort(const std::vector<int>&,std::vector<int>&);
template double norm(const std::vector<double>&);

//template void mathFunc::crossSort(std::vector<int>&,
//				  std::vector<RMCbox::Atom*>&);
template void mathFunc::crossSort(std::vector<int>&,std::vector<int>&);
template void mathFunc::crossSort(std::vector<int>&,std::vector<double>&);
template void mathFunc::crossSort(std::vector<double>&,std::vector<double>&);
template void mathFunc::crossSort(std::vector<float>&,std::vector<float>&);
template void mathFunc::Order(int&,int&);
template void mathFunc::Order(double&,double&);
template void mathFunc::Swap(double&,double&);

template int mathFunc::binSearch(const std::vector<double>::const_iterator&,
				 const std::vector<double>::const_iterator&,
				 const double&);

/// \endcond TEMPLATE 

}
