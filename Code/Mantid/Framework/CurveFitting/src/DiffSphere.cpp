//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/DiffSphere.h"
#include <cmath>
<<<<<<< HEAD
=======
#include <boost/math/special_functions/bessel.hpp>
>>>>>>> Refs #4394 Updated main fitting funtion

namespace Mantid
{
namespace CurveFitting
{

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(DiffSphere)

<<<<<<< HEAD
DiffSphere::DiffSphere()
{
  declareParameter("Radius", 1.0);
  declareParameter("Diffusion", 1.0);

}


void DiffSphere::functionMW(double* out, const double* xValues, const size_t nData)const
{
    const double& h = getParameter("Height");
    const double& t = getParameter("Location");
    const double& b = getParameter("Scale");

    for (size_t i = 0; i < nData; i++) 
    {
    	double x = xValues[i];
    	if( x == 0.0 )
    	{
    		out[i] = 0.0; //limit of the distribution as x approaches to zero
    	}
    	else
    	{
    		double c = (log(x)-t)/b;
    		out[i] = h/x*exp( -c*c/2 );
    	}
    }
}

void DiffSphere::functionDerivMW(Jacobian* out, const double* xValues, const size_t nData)
{
    const double& h = getParameter("Height");
    const double& t = getParameter("Location");
    const double& b = getParameter("Scale");

    for (size_t i = 0; i < nData; i++) {
        double x = xValues[i];
        if(x==0.0)
        {
            out->set(i,0, 0.0); //all partial derivatives approach to 0 as x goes to 0
            out->set(i,1, 0.0);
            out->set(i,2, 0.0);
        }else
        {
        	double c = (log(x)-t)/b;
        	double e = exp( -c*c/2 )/x;
        	out->set(i,0, e);           //partial derivative with respect to Height
        	out->set(i,1, h*e*(c/b));    //partial derivative with respect to Location parameter
        	out->set(i,2, h*e*(c*c/b));  //partial derivative with respect to Scale parameter
        }
    }

}


=======
// initialize class attribute xnl with a list of coefficients in string format
void DiffSphere::initXnlCoeff(double *coeffList){
  xnlc coeff;
  for(size_t i=0; i<ncoeff; i+=3){
    coeff.x = coeffList[i];                            //value of the coefficient
    coeff.n = dynamic_cast<unsigned>(coeffList[i+1]);  //corresponding n
    coeff.l = dynamic_cast<unsigned>(coeffList[i+2]);  //corresponding l
    }
  xnl.push_back(coeff);
}

//initialize a set of coefficients that will remain constant during fitting
void DiffSphere::initAlphaCoeff(){
  for(std::vector<xnlc>::const_iterator it=xnl.begin(); it!=xnl.end(); ++it){
    unsigned l = it->l;
    double c = it->x * it->x;
    alpha.push_back( (2*l+1) * ( 6*c/(c-l*(l+1)) ) / M_PI );
  }
}

//initialize linear interpolation of factor J around its numerical divergence point a = it->x
void DiffSphere::initLinJlist(){
  linearJ abJ;
  unsigned l;
  double a,c;
  double J0,J1;
  for(std::vector<xnlc>::const_iterator it=xnl.begin(); it!=xnl.end(); ++it){
    l = it->l;
    c = it->x * it->x;
    a = it->x - divZone; //left of the numerical divergence point
    J0 = ( a*boost::math::sph_bessel(l+1,a)-l*boost::math::sph_bessel(l,a) ) / (a*a - c);
    a = it->x - divZone; //right of the numerical divergence point
    J1 = ( a*boost::math::sph_bessel(l+1,a)-l*boost::math::sph_bessel(l,a) ) / (a*a - c);
    abJ.slope = (J1-J0)/(2*divZone);  //slope of the linear interpolation
    abJ.intercept = J1 - abJ.slope * (it->x - divZone); //intercept of the linear interpolation

    linearJlist.push_back(abJ); //store the parameters of the linear interpolation for this it->x
  }
}

DiffSphere::DiffSphere(){
  declareParameter("Radius", 1.0, "Sphere radius");
  declareParameter("Diffusion", 1.0, "Diffusion coefficient, in units of");
  declareParameter("Q",1.0, "Momentum transfer");

  this->initXnlCoeff(xnlist); // initialize this->xnl with the list of coefficients xnlist
  this->initAlphaCoeff();     // initialize this->alpha, certain factors constant over the fit
  this->initLinJlist();       // initialize this->linJlist, linear interpolation around numerical divergence
}


void DiffSphere::functionMW(double* out, const double* xValues, const size_t nData)const{
  const double& R = getParameter("Radius");
  const double& D = getParameter("Diffusion");
  const double& Q = getParameter("Q");

  double a=Q*R, w=D/(R*R); //active parameters, "scaled" radius and diffusion

  //precompute the 2+lmax spherical bessel functions
  double jl[2+lmax];
  for(size_t l=0; l<=1+lmax; l++) jl[l] = boost::math::sph_bessel(l,a);

  unsigned l;
  double c,cw;   //coefficient related
  double A,J,L;  //factors of the fitting function A_i*J_i(a,x)*L_i(w,x)

  for (size_t i = 0; i < nData; i++){

    double x = xValues[i];

    //loop over all coefficients
    out[i] = 0.0;
    std::vector<double>::const_iterator italpha=alpha.begin();
    std::vector<linearJ>::const_iterator itlinJ=linearJlist.begin();
    for(std::vector<xnlc>::const_iterator it=xnl.begin(); it!=xnl.end(); ++it){

      //only to make expressions for A, J, and L more readable
      c  = it->x * it->x;
      cw = c * w;
      l  = it->l;

      //compute the three factors A, J, and L that go into each term
      A = *italpha; //A is independent of parameters a and w, and independent of data x

      /*J is dependent on parameter a and independent of data x
       * cannot be computed when active parameter a obeys a*a-c=0. Thus for each it->x
       * we stored J(it->x-divZone) and J(it->x_divZone), and use linear interpolation
       * */
      if(a - it->x > divZone ){
        J = ( a*jl[l+1]-l*jl[l] ) / (a*a - c);
      }else{
          J = itlinJ->slope*a + itlinJ->intercept;
      }
      J *= J;

      //Lorentzian. L is dependent on parameter w and data x
      L = cw/(cw*cw+x*x);

      out[i] += A * J * L;

      ++italpha; //retrieve next alpha coefficient
      ++itlinJ;  //retrieve next linear interpolation
    }//end of for(std::vector<xnlc>::const_iterator it....
  }//end of for (size_t i...
} //end of void DiffSphere::functionMW

>>>>>>> Refs #4394 Updated main fitting funtion
} // namespace CurveFitting
} // namespace Mantid
