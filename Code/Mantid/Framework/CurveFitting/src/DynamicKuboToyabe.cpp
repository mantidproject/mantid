//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/DynamicKuboToyabe.h"
#include "MantidAPI/Jacobian.h"
#include "MantidAPI/FunctionFactory.h"
#include <vector>

namespace Mantid
{
namespace CurveFitting
{

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(DynamicKuboToyabe)

void DynamicKuboToyabe::init()
{
  declareParameter("Asym",  0.2, "Amplitude at time 0");
  declareParameter("Delta", 0.2, "Local field");
  declareParameter("Field", 0.0, "External field");
  declareParameter("Nu",    0.0, "Hopping rate");
}


//--------------------------------------------------------------------------------------------------------------------------------------
// From Numerical Recipes

// Midpoint method
double midpnt(double func(const double, const double, const double),
              const double a, const double b, const int n, const double g,
              const double w0) {
  // quote & modified from numerical recipe 2nd edtion (page147)

  static double s;

  if (n == 1) {
    s = (b - a) * func(0.5 * (a + b), g, w0);
    return (s);
  } else {
    double x, tnm, sum, del, ddel;
    int it, j;
    for (it = 1, j = 1; j < n - 1; j++)
      it *= 3;
    tnm = it;
    del = (b - a) / (3 * tnm);
    ddel = del + del;
    x = a + 0.5 * del;
    sum = 0.0;
    for (j = 1; j <= it; j++) {
      sum += func(x, g, w0);
      x += ddel;
      sum += func(x, g, w0);
      x += del;
    }
    s = (s + (b - a) * sum / tnm) / 3.0;
    return s;
  }
}

// Polynomial interpolation
void polint (double xa[], double ya[], int n, double x, double& y, double& dy) {
  int i, m, ns = 1;
  double dif;

  dif = fabs(x - xa[1]);
  std::vector<double> c(n + 1);
  std::vector<double> d(n + 1);
  for (i = 1; i <= n; i++) {
    double dift;
    if ((dift = fabs(x - xa[i])) < dif) {
      ns = i;
      dif = dift;
    }
    c[i] = ya[i];
    d[i] = ya[i];
  }
  y = ya[ns--];
  for (m = 1; m < n; m++) {
    for (i = 1; i <= n - m; i++) {
      double den, ho, hp, w;
      ho = xa[i] - x;
      hp = xa[i + m] - x;
      w = c[i + 1] - d[i];
      if ((den = ho - hp) == 0.0) { // error message!!!
        throw std::runtime_error("Error in routin polint");
      }
      den = w / den;
      d[i] = hp * den;
      c[i] = ho * den;
    }
    y += (dy = (2 * (ns) < (n - m) ? c[ns + 1] : d[ns--]));
  }
}

// Integration
double integral(double func(const double, const double, const double),
                const double a, const double b, const double g,
                const double w0) {

  const int JMAX = 14;
  const int JMAXP = JMAX + 1;
  const int K = 5;

  int j;
  double ss, dss;
  double h[JMAXP + 1], s[JMAXP];

  h[1] = 1.0;
  for (j = 1; j <= JMAX; j++) {
    s[j] = midpnt(func, a, b, j, g, w0);
    if (j >= K) {
      polint(&h[j - K], &s[j - K], K, 0.0, ss, dss);
      if (fabs(dss) <= fabs(ss))
        return ss;
    }
    h[j + 1] = h[j] / 9.0;
  }
  throw std::runtime_error("Too many steps in routine integrate");
}

// End of Numerical Recipes routines
//--------------------------------------------------------------------------------------------------------------------------------------


// f1: function to integrate
double f1(const double x, const double G, const double w0) {
  return( exp(-G*G*x*x/2)*sin(w0*x));
}

// Static Zero Field Kubo Toyabe relaxation function
double ZFKT (const double x, const double G){

  const double q = G*G*x*x;
  return (0.3333333333 + 0.6666666667*exp(-0.5*q)*(1-q));
}

// Static non-zero field Kubo Toyabe relaxation function
double HKT (const double x, const double G, const double F) {

  const double q = G*G*x*x;
  const double gm = 2*M_PI*0.01355342; // Muon gyromagnetic ratio * 2 * PI
  
  double w;
  if (F>2*G) {
    // Use F
    w = gm * F;
  } else {
    // Use G
    w = gm * 2 * G;
  }
  
  const double r = G*G/w/w;
  
  double ig;
  if ( x>0 && r>0 ) {
    // Compute integral
    ig = integral(f1,0.0,x,G,w);
  } else {
    // Integral is 0
    ig = 0;
  }
  
  const double ktb=(1-2*r*(1-exp(-q/2)*cos(w*x))+2*r*r*w*ig);
  
  if ( F>2*G ) {
    return ktb;
  } else {
    const double kz = ZFKT(x,G);
    return kz+F/2/G*(ktb-kz);
  }
  
}

// Dynamic Kubo-Toyabe
double DynamicKuboToyabe::getDKT (double t, double G, double F, double v, double eps) const {

  const int tsmax = static_cast<int>(std::ceil(32.768/eps));

  static double oldG=-1., oldV=-1., oldF=-1., oldEps=-1.;

  const int maxTsmax = static_cast<int>(std::ceil(32.768/m_minEps));
  static std::vector<double> gStat(maxTsmax), gDyn(maxTsmax);

  if ( (G != oldG) || (v != oldV) || (F != oldF) || (eps != oldEps)){

    // If G or v or F or eps have changed with respect to the 
    // previous call, we need to re-do the computations


    if ( G != oldG || (F != oldF) ){

      // But we only need to
      // re-compute gStat if G or F have changed

      // Generate static Kubo-Toyabe
      if (F == 0) {
        for (int k=0; k<tsmax; k++){
          gStat[k]= ZFKT(k*eps,G);
        }
      } else {
        for (int k=0; k<tsmax; k++){
          gStat[k]= HKT(k*eps,G,F);
        }
      }
      // Store new G value
      oldG =G;
      // Store new F value
      oldF =F;
    }

    // Store new v value
    oldV =v;
    // Store new eps value
    oldEps =eps;

    double hop = v*eps;

    // Generate dynamic Kubo Toyabe
    for (int k=0; k<tsmax; k++){
      double y=gStat[k];
      for (int j=k-1; j>0; j--){
        y=y*(1-hop)+hop*gDyn[k-j]*gStat[j];
      }
      gDyn[k]=y;
    }
  }

  // Interpolate table 
  // If beyond end, extrapolate
  int x=int(fabs(t)/eps);
  if (x>tsmax-2)
    x = tsmax-2;
  double xe=(fabs(t)/eps)-x;
  return gDyn[x]*(1-xe)+xe*gDyn[x+1];

}

// Dynamic Kubo Toyabe function
void DynamicKuboToyabe::function1D(double* out, const double* xValues, const size_t nData)const
{
  const double& A = getParameter("Asym");
  const double& G = fabs(getParameter("Delta"));
  const double& F = fabs(getParameter("Field"));
  const double& v = fabs(getParameter("Nu"));


  // Zero hopping rate
  if (v == 0.0) {

    // Zero external field
    if ( F == 0.0 ){
      for (size_t i = 0; i < nData; i++) {
        out[i] = A*ZFKT(xValues[i],G);
      }
    }
    // Non-zero external field
    else{
      for (size_t i = 0; i < nData; i++) {
        out[i] = A*HKT(xValues[i],G,F);
      }
    }
  } 

  // Non-zero hopping rate
  else {

    for (size_t i = 0; i<nData; i++){
      out[i] = A*getDKT(xValues[i],G,F,v,m_eps);
    }
  }


}


//----------------------------------------------------------------------------------------------
/** Constructor
 */
DynamicKuboToyabe::DynamicKuboToyabe() : m_eps(0.05), m_minEps(0.003) {}

//----------------------------------------------------------------------------------------------
/** Function to calculate derivative numerically
 */
void DynamicKuboToyabe::functionDeriv(const API::FunctionDomain& domain, API::Jacobian& jacobian)
{
  calNumericalDeriv(domain, jacobian);
}

//----------------------------------------------------------------------------------------------
/** Function to calculate derivative analytically
 */
void DynamicKuboToyabe::functionDeriv1D(API::Jacobian* , const double* , const size_t )
{
  throw Mantid::Kernel::Exception::NotImplementedError("functionDeriv1D is not implemented for DynamicKuboToyabe.");
}

//----------------------------------------------------------------------------------------------
/** Set new value of the i-th parameter
 * @param i :: parameter index
 * @param value :: new value
 */
void DynamicKuboToyabe::setActiveParameter(size_t i, double value) {

  setParameter( i, fabs(value), false);

}

//----------------------------------------------------------------------------------------------
/** Get Attribute names
 * @return A list of attribute names
 */
std::vector<std::string> DynamicKuboToyabe::getAttributeNames() const {
  std::vector<std::string> res;
  res.push_back("eps");
  return res;
}

//----------------------------------------------------------------------------------------------
/** Get Attribute
 * @param attName :: Attribute name. If it is not "eps" an exception is thrown.
 * @return a value of attribute attName
 */
API::IFunction::Attribute DynamicKuboToyabe::getAttribute(const std::string &attName) const {

  if (attName == "eps") {
    return Attribute(m_eps);
  }
  throw std::invalid_argument("DynamicKuboToyabe: Unknown attribute " + attName);
}

//----------------------------------------------------------------------------------------------
/** Set Attribute
 * @param attName :: The attribute name. If it is not "eps" exception is thrown.
 * @param att :: A double attribute containing a new positive value.
 */
void DynamicKuboToyabe::setAttribute(const std::string &attName,
                              const API::IFunction::Attribute &att) {
  if (attName == "eps") {

    double newVal = att.asDouble();

    if (newVal < 0) {
      throw std::invalid_argument("DynamicKuboToyabe: bin width cannot be negative.");

    } else if (newVal < m_minEps) {
      throw std::invalid_argument("DynamicKuboToyabe: bin width too small.");
    }

    m_eps = newVal;

  } else {
    throw std::invalid_argument("DynamicKuboToyabe: Unknown attribute " + attName);
  }
}

//----------------------------------------------------------------------------------------------
/** Check if attribute attName exists
 * @param attName :: The attribute name.
 */
bool DynamicKuboToyabe::hasAttribute(const std::string &attName) const {
  return attName == "eps";
}


} // namespace CurveFitting
} // namespace Mantid
