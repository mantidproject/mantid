//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/DynamicKuboToyabe.h"
#include "MantidAPI/Jacobian.h"
#include "MantidAPI/FunctionFactory.h"
#include <cmath>
#include <boost/math/special_functions/fpclassify.hpp> // TODO Remove

#define EPS 1e-6
#define JMAX 14
#define JMAXP (JMAX+1)
#define K 5
#define NR_END 1
#define FREE_ARG char*

namespace Mantid
{
namespace CurveFitting
{

using namespace Kernel;
using namespace API;
using namespace boost::math;

DECLARE_FUNCTION(DynamicKuboToyabe)

// ** MODIFY THIS **
// Here specify/declare the parameters of your Fit Function
// 
// declareParameter takes three arguments:
//
//   1st: The name of the parameter
//   2nd: The default (initial) value of the parameter
//   3rd: A description of the parameter (optional)
//
void DynamicKuboToyabe::init()
{
  declareParameter("Asym",  0.2, "Amplitude at time 0");
  declareParameter("Delta", 0.2, "Local field");
  declareParameter("Field", 0.0, "External field");
  declareParameter("Nu",    0.0, "Hopping rate");
  //declareParameter("EndX",15);
}


//------------------------------------------------------------------------------------------------
double *vector(long nl, long nh)
/* allocate a double vector with subscript range v[nl..nh] */
{
	double *v;

	v=(double *)malloc((size_t) ((nh-nl+1+NR_END)*sizeof(double)));
  if (!v) throw std::runtime_error("allocation failure in dvector()");
	return v-nl+NR_END;
}
void free_vector(double *v, long nl, long nh)
/* free a double vector allocated with dvector() */
{
	free((FREE_ARG) (v+nl-NR_END));
}
double midpnt(double func(const double, const double, const double),
	const double a, const double b, const int n, const double g, const double w0) {
// quote & modified from numerical recipe 2nd edtion (page147)	
	
	static double s;

	if (n==1) { 
    double s1 = 0.5*(b-a)*func(a,g,w0)+func(b,g,w0);
    double s2 = (b-a)*func(0.5*(a+b),g,w0);
    return (s2);
	} else {
		double x, tnm, sum, del, ddel;
		int it, j;
		for (it=1,j=1;j<n-1;j++) it *= 3;
		tnm = it;
		del = (b-a)/(3*tnm);
		ddel=del+del;
		x = a+0.5*del;
		sum =0.0;
		for (j=1;j<=it;j++) {
			sum += func(x,g,w0);
			x += ddel;
			sum += func(x,g,w0);
			x += del;
		}
		s=(s+(b-a)*sum/tnm)/3.0;
		return s;
	}
}

void polint (double xa[], double ya[], int n, double x, double& y, double& dy) {
	int i, m, ns = 1;
  double den, dif, dift, ho, hp, w;

  double *c,*d;

  dif = fabs(x-xa[1]);
  c = vector(1,n);
  d = vector(1,n);
	for (i=1;i<=n;i++){
		if((dift=fabs(x-xa[i]))<dif) {
			ns=i;
			dif=dift;
		}
    c[i]=ya[i];
		d[i]=ya[i];
	}
	y=ya[ns--];
	for (m=1;m<n;m++) {
		for (i=1;i<=n-m;i++) {
			ho=xa[i]-x;
			hp=xa[i+m]-x;
			w=c[i+1]-d[i];
			if((den=ho-hp)==0.0) //error message!!!
        throw std::runtime_error("Error in routine polint\n");
			den=w/den;
			d[i]=hp*den;
			c[i]=ho*den;
		}
		y += (dy=(2*(ns)<(n-m) ? c[ns+1] : d[ns--]));

	}

  free_vector(d,1,n);
  free_vector(c,1,n);
}


double integrate (double func(const double, const double, const double),
				const double a, const double b, const double g, const double w0) {
	int j;
	double ss,dss;
	double h[JMAXP+1], s[JMAXP];
	
	h[0] = 1.0;
	for (j=1; j<= JMAX; j++) {
		s[j]=midpnt(func,a,b,j,g,w0);
		if (j >= K) {
			polint(&h[j-K],&s[j-K],K,0.0,ss,dss);
			if (fabs(dss) <= fabs(ss)) return ss;
		}
		h[j+1]=h[j]/9.0;
	}
  throw std::runtime_error("integrate(): Too many steps in routine integrate\n");
	return 0.0;
}

//--------------------------------------------------------------------------------------------------------------------------------------


double f1(const double x, const double G, const double w0) {
  return( exp(-G*G*x*x/2)*sin(w0*x));
}

double ZFKT (double q){
  // In zero field: 
  // g(t) = 1/3 + 2/3 exp( -q/2 ) ( 1 - q )
  // q = t *sigma
  return (0.3333333333 + 0.6666666667*exp(-0.5*q)*(1-q));
}
double gz (const double x, const double G, const double F) 
{
	double w0 = 2.0*3.1415926536*0.01355342*F;
	const double q = G*G*x*x;
	
	if (w0 == 0.0) {
		return (ZFKT(q)); 
	}
	else {
		
		if (F>2.0*G) { w0 = 2*3.1415926*0.01355342*F ;} else { w0 =2*3.1415926*0.01355342*2.0*G; }
			
		double p = G*G/(w0*w0);
		double HKT = 1.0-2.0*p*(1-exp(-q/2.0)*cos(w0*x))+2.0*p*p*w0*integrate(f1,0.0,x,G,w0);
		if (F>2.0*G) {return (HKT);} 
		else {return (ZFKT(q)+ (F/2.0/G)*(HKT-ZFKT(q)));}	 
		
   }
}

// Original function by mark telling
void DynamicKuboToyabe::function1D(double* out, const double* xValues, const size_t nData)const
{
  const double& A = getParameter("Asym");
  const double& G = fabs(getParameter("Delta"));
  const double& F = fabs(getParameter("Field"));
  const double& v = fabs(getParameter("Nu"));


  // Zero hopping rate
	if (v == 0.0) {
		for (size_t i = 0; i < nData; i++) {
			out[i] = A*gz(xValues[i],G,F);
		}
	} 

  // Non-zero hopping rate
	else {
    const int n = 1000;
    const double stepsize = fabs(xValues[nData-1]/n);
    // do{stepsizeTemp=stepsizeTemp/10;nTemp=nTemp*10; }while (xValues[0]<stepsizeTemp);
    //make sure stepsize is smaller than spacing between xValues.
    std::vector<double> funcG(n);
    for (int i = 0; i < n; i++) {

      double Integral=0.0;
      for (int c = 1; c <= i; c++) {
        Integral= gz(c*stepsize,G,F)*exp(-v*c*stepsize)*funcG[i-c]*(stepsize) + Integral; 
      }
      funcG[i] = (gz(i*stepsize,G,F)*exp(-v*i*stepsize) + v*Integral);
    }


		for (size_t i = 0; i < nData; i++) {
			double a =xValues[i]/stepsize;
      a = a<nData ? a : a-1;
      out[i] = A*(funcG.at(int(a)));
		}

   } // else hopping rate != 0

}



void DynamicKuboToyabe::functionDeriv(const API::FunctionDomain& domain, API::Jacobian& jacobian)
{
  calNumericalDeriv(domain, jacobian);
}

void DynamicKuboToyabe::functionDeriv1D(API::Jacobian* , const double* , const size_t )
{
  throw Mantid::Kernel::Exception::NotImplementedError("functionDerivLocal is not implemented for DynamicKuboToyabe.");
}

void DynamicKuboToyabe::setActiveParameter(size_t i, double value) {

  setParameter( i, fabs(value), false);

}

} // namespace CurveFitting
} // namespace Mantid
