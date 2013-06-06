//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/DiffSphere.h"
#include "MantidCurveFitting/BoundaryConstraint.h"
#include "MantidAPI/FunctionFactory.h"
#include <cmath>
#include <boost/math/special_functions/bessel.hpp>
#include "MantidAPI/ParameterTie.h"


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Mantid
{
namespace CurveFitting
{

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(InelasticDiffSphere);

// initialize class attribute xnl with a list of coefficients in string format
void InelasticDiffSphere::initXnlCoeff(){
  /* List of 98 coefficients sorted by increasing value (F.Volino,Mol. Phys. 41,271-279,1980)
   * For each coefficient, the triad (coeff, l, n) is defined
   */
  ncoeff = 98;
  double xnlist[]={
    2.081576,  1,  0,  3.342094,  2,  0,  4.493409,  0,  1,  4.514100,  3,  0,
    5.646704,  4,  0,  5.940370,  1,  1,  6.756456,  5,  0,  7.289932,  2,  1,
    7.725252,  0,  2,  7.851078,  6,  0,  8.583755,  3,  1,  8.934839,  7,  0,
    9.205840,  1,  2,  9.840446,  4,  1, 10.010371,  8,  0, 10.613855,  2,  2,
    10.904122,  0,  3, 11.070207,  5,  1, 11.079418,  9,  0, 11.972730,  3,  2,
    12.143204, 10,  0, 12.279334,  6,  1, 12.404445,  1,  3, 13.202620, 11,  0,
    13.295564,  4,  2, 13.472030,  7,  1, 13.846112,  2,  3, 14.066194,  0,  4,
    14.258341, 12,  0, 14.590552,  5,  2, 14.651263,  8,  1, 15.244514,  3,  3,
    15.310887, 13,  0, 15.579236,  1,  4, 15.819216,  9,  1, 15.863222,  6,  2,
    16.360674, 14,  0, 16.609346,  4,  3, 16.977550, 10,  1, 17.042902,  2,  4,
    17.117506,  7,  2, 17.220755,  0,  5, 17.408034, 15,  0, 17.947180,  5,  3,
    18.127564, 11,  1, 18.356318,  8,  2, 18.453241, 16,  0, 18.468148,  3,  4,
    18.742646,  1,  5, 19.262710,  6,  3, 19.270294, 12,  1, 19.496524, 17,  0,
    19.581889,  9,  2, 19.862424,  4,  4, 20.221857,  2,  5, 20.371303,  0,  6,
    20.406581, 13,  1, 20.538074, 18,  0, 20.559428,  7,  3, 20.795967, 10,  2,
    21.231068,  5,  4, 21.537120, 14,  1, 21.578053, 19,  0, 21.666607,  3,  5,
    21.840012,  8,  3, 21.899697,  1,  6, 21.999955, 11,  2, 22.578058,  6,  4,
    22.616601, 20,  0, 22.662493, 15,  1, 23.082796,  4,  5, 23.106568,  9,  3,
    23.194996, 12,  2, 23.390490,  2,  6, 23.519453,  0,  7, 23.653839, 21,  0,
    23.783192, 16,  1, 23.906450,  7,  4, 24.360789, 10,  3, 24.382038, 13,  2,
    24.474825,  5,  5, 24.689873, 22,  0, 24.850085,  3,  6, 24.899636, 17,  1,
    25.052825,  1,  7, 25.218652,  8,  4, 25.561873, 14,  2, 25.604057, 11,  3,
    25.724794, 23,  0, 25.846084,  6,  5, 26.012188, 18,  1, 26.283265,  4,  6,
    26.516603,  9,  4, 26.552589,  2,  7, 26.666054,  0,  8, 26.735177, 15,  2,
    26.758685, 24,  0, 26.837518, 12,  3};

  for(unsigned int i=0; i<ncoeff; i+=3){
    xnlc coeff;
    coeff.x = xnlist[i];                            //value of the coefficient
    coeff.l = (unsigned int)(xnlist[i+1]);  //corresponding n
    coeff.n= (unsigned int)(xnlist[i+2]);  //corresponding l
    xnl.push_back(coeff);
  }
}

//initialize a set of coefficients that will remain constant during fitting
void InelasticDiffSphere::initAlphaCoeff(){
  for(std::vector<xnlc>::const_iterator it=xnl.begin(); it!=xnl.end(); ++it){
    double x = it->x;
    double l = (double)(it->l);
    alpha.push_back( (2.0*l+1) * ( 6.0*x*x/(x*x-l*(l+1)) ) / M_PI );
  }
}

//initialize linear interpolation of factor J around its numerical divergence point a = it->x
void InelasticDiffSphere::initLinJlist(){
  for(std::vector<xnlc>::const_iterator it = xnl.begin(); it != xnl.end(); ++it){
    linearJ abJ;
    double x = it->x;
    unsigned int l = it->l;
    double a = x - m_divZone; //left of the numerical divergence point
    double J0 = ( a * boost::math::sph_bessel(l+1,a) - l * boost::math::sph_bessel(l,a) ) / (a*a - x*x);
    a = x + m_divZone; //right of the numerical divergence point
    double J1 = ( a * boost::math::sph_bessel(l+1,a) - l * boost::math::sph_bessel(l,a) ) / (a*a - x*x);
    abJ.slope = (J1 - J0) / (2 * m_divZone);  //slope of the linear interpolation
    abJ.intercept = J0 - abJ.slope * (x - m_divZone); //intercept of the linear interpolation
    linearJlist.push_back(abJ); //store the parameters of the linear interpolation for this it->x
  }
}

InelasticDiffSphere::InelasticDiffSphere() : lmax(24), m_divZone(0.1) {
  declareParameter("Intensity",1.0, "scaling factor");
  declareParameter("Radius", 1.0, "Sphere radius");
  declareParameter("Diffusion", 1.0, "Diffusion coefficient, in units of");

  declareAttribute( "Q", API::IFunction::Attribute(1.0) );

  // Ensure positive values for Intensity, Radius, and Diffusion coefficient
  BoundaryConstraint* IntensityConstraint = new BoundaryConstraint(this,"Intensity",0,true);
  addConstraint(IntensityConstraint);

  BoundaryConstraint* RadiusConstraint = new BoundaryConstraint(this,"Radius",0,true);
  addConstraint(RadiusConstraint);

  BoundaryConstraint* DiffusionConstraint = new BoundaryConstraint(this,"Diffusion",0,true);
  addConstraint(DiffusionConstraint);

  initXnlCoeff();   // initialize this->xnl with the list of coefficients xnlist
  initAlphaCoeff(); // initialize this->alpha, certain factors constant over the fit
  initLinJlist();   // initialize this->linJlist, linear interpolation around numerical divergence
}

//calculate the coefficients for each Lorentzian
std::vector<double> InelasticDiffSphere::LorentzianCoefficients(double a)const{

  //precompute the 2+lmax spherical bessel functions (26 in total)
  double* jl = new double[2+lmax];
  for(unsigned int l=0; l<=1+lmax; l++){
    jl[l] = boost::math::sph_bessel(l,a);
  }

  //store the coefficient of each Lorentzian in vector YJ(a,w)
  std::vector<double> YJ(ncoeff);
  std::vector<linearJ>::const_iterator itlinJ=linearJlist.begin();
  //loop over all coefficients
  std::vector<double>::const_iterator italpha=alpha.begin();
  std::vector<double>::iterator itYJ=YJ.begin();
  for(std::vector<xnlc>::const_iterator it=xnl.begin(); it!=xnl.end(); ++it){
    //only to make expressions more readable
    double x  = it->x;
    unsigned int l  = it->l;
    //compute  factors Y and J
    double Y = *italpha; //Y is independent of parameters a and w, and independent of data x
    /* J is dependent on parameter a, cannot be computed when active parameter a obeys a*a=c.
     * Thus for each it->x we stored J(it->x-m_divZone) and J(it->x_m_divZone), and use linear
     * interpolation
     */
    double J;
    if(fabs(a-x) > m_divZone ){
      J = ( a*jl[l+1]-l*jl[l] ) / (a*a - x*x);
    }else{
      J = itlinJ->slope*a + itlinJ->intercept; //linear interpolation
    }
    *itYJ = Y*J*J;
    ++itYJ;
    ++italpha;
    ++itlinJ;  //retrieve next linear interpolation
  } // end of for(std::vector<xnlc>::const_iterator it=xnl.begin()

  delete[] jl;
  return YJ;
} // end of LorentzianCoefficients


void InelasticDiffSphere::function1D(double* out, const double* xValues, const size_t nData)const{
  const double I = getParameter("Intensity");
  const double R = getParameter("Radius");
  const double D = getParameter("Diffusion");
  const double Q = getAttribute("Q").asDouble();

  std::vector<double> YJ;
  YJ = LorentzianCoefficients( Q * R );
  for (unsigned int i = 0; i < nData; i++){
    double x = xValues[i];
    //loop over all coefficients
    out[i] = 0.0;
    std::vector<double>::const_iterator itYJ=YJ.begin();
    for(std::vector<xnlc>::const_iterator it=xnl.begin(); it!=xnl.end(); ++it){
      double z = it->x;
      double zw = z*z*D/(R*R);
      double L = zw/(zw*zw+x*x); //Lorentzian
      out[i] += I * (*itYJ) * L;
      ++itYJ;  //retrieve next coefficient
    } // end of for(std::vector<xnlc>::const_iterator it....
  } // end of for (unsigned int i...
} // end of void DiffSphere::functionMW

/** Calculate numerical derivatives.
 * @param domain :: The domain of the function
 * @param jacobian :: A Jacobian matrix. It is expected to have dimensions of domain.size() by nParams().
 */
void InelasticDiffSphere::calNumericalDeriv2(const API::FunctionDomain& domain, API::Jacobian& jacobian)
{
  const double minDouble = std::numeric_limits<double>::min();
  const double epsilon = std::numeric_limits<double>::epsilon() * 100;
  double stepPercentage = 0.001; // step percentage
  double step; // real step
  double cutoff = 100.0*minDouble/stepPercentage;
  size_t nParam = nParams();
  size_t nData = domain.size();

  FunctionValues minusStep(domain);
  FunctionValues plusStep(domain);

  //PARALLEL_CRITICAL(numeric_deriv)
  {
    applyTies(); // just in case
    function(domain,minusStep);
  }

  for (size_t iP = 0; iP < nParam; iP++)
  {
    if ( isActive(iP) )
    {
      const double val = activeParameter(iP);
      if (fabs(val) < cutoff)
      {
        step = epsilon;
      }
      else
      {
        step = val*stepPercentage;
      }

      double paramPstep = val + step;
      //PARALLEL_CRITICAL(numeric_deriv)
      {
        setActiveParameter(iP, paramPstep);
        applyTies();
        function(domain,plusStep);
        setActiveParameter(iP, val);
      }

      step = paramPstep - val;
      for (size_t i = 0; i < nData; i++)
      {
        jacobian.set(i, iP, (plusStep.getCalculated(i) - minusStep.getCalculated(i)) / step);
      }
    }
  }
} // calNumericalDeriv()

/// Using numerical derivative
void InelasticDiffSphere::functionDeriv(const API::FunctionDomain &domain, API::Jacobian &jacobian)
{
  this->calNumericalDeriv(domain, jacobian);
  return;
}

/// Using numerical derivative
void InelasticDiffSphere::functionDeriv1D(Jacobian* jacobian, const double* xValues, const size_t nData)
{
  FunctionDomain1DView domain(xValues,nData);
  this->calNumericalDeriv(domain,*jacobian);
}

} // namespace CurveFitting
} // namespace Mantid
